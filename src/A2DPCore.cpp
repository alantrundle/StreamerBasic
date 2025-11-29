#include "A2DPCore.h"

#include <string.h>
#include <Arduino.h>

#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp32-hal-bt.h"
#include "esp_coexist.h"

// =============================================================
// Static state
// =============================================================
static constexpr int MAX_SCAN = 16;

struct BTDev {
    char           name[32];
    esp_bd_addr_t  addr;
    int8_t         rssi;
};

static BTDev found[MAX_SCAN];
static int   found_count   = 0;

static bool bt_ready       = false;
static bool gap_cb_set     = false;
static bool a2dp_started   = false;

// Static members
const char*              A2DPCore::device_name = "A2DP-Source";
A2DPCore::pcm_callback_t A2DPCore::pcm_cb_   = nullptr;
A2DPCore::scan_callback_t A2DPCore::scan_cb_ = nullptr;
A2DPCore::a2dp_conn_cb_t  A2DPCore::conn_cb_ = nullptr;
A2DPCore::a2dp_audio_cb_t A2DPCore::audio_cb_ = nullptr;

// =============================================================
// Helpers
// =============================================================
static void mac_to_str(const uint8_t* b, char* out)
{
    sprintf(out, "%02X:%02X:%02X:%02X:%02X:%02X",
            b[0], b[1], b[2], b[3], b[4], b[5]);
}

static void resolve_name(const uint8_t* eir, char* out, size_t len)
{
    if (!eir || !out || len == 0) {
        return;
    }

    uint8_t name_len = 0;
    uint8_t* name = esp_bt_gap_resolve_eir_data(
        (uint8_t*)eir,
        ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME,
        &name_len
    );

    if (!name) {
        name = esp_bt_gap_resolve_eir_data(
            (uint8_t*)eir,
            ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME,
            &name_len
        );
    }

    if (name && name_len > 0) {
        if (name_len >= len) name_len = len - 1;
        memcpy(out, name, name_len);
        out[name_len] = '\0';
    }
}

// =============================================================
// Classic BT bring-up (controller + bluedroid)
// =============================================================
void A2DPCore::ensure_bt_ready()
{
    if (bt_ready) return;

    Serial.println("[BT] Initialising Classic stack");

    // Use Arduino's btStart() helper – this wraps controller init/enable
    if (!btStarted()) {
        if (!btStart()) {
            Serial.println("[BT] btStart() failed");
            return;
        }
    }

    // Make sure Bluedroid is up
    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
        ESP_ERROR_CHECK(esp_bluedroid_init());
    }
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bluedroid_enable());
    }

    // Prefer BT over Wi-Fi when they contend
    esp_coex_preference_set(ESP_COEX_PREFER_BT);

    bt_ready = true;
    Serial.println("[BT] Classic stack READY");
}

// =============================================================
// Lifecycle
// =============================================================
A2DPCore::A2DPCore() {}

void A2DPCore::set_device_name(const char* name) {
    device_name = name;
}

void A2DPCore::set_pcm_callback(pcm_callback_t cb) {
    pcm_cb_ = cb;
}

void A2DPCore::set_scan_callback(scan_callback_t cb) {
    scan_cb_ = cb;
}

void A2DPCore::set_connectstate_callback(a2dp_conn_cb_t cb) {
    conn_cb_ = cb;
}

void A2DPCore::set_audiostate_callback(a2dp_audio_cb_t cb) {
    audio_cb_ = cb;
}

// =============================================================
// Start — Classic BT + A2DP Source
// =============================================================
bool A2DPCore::start()
{
    Serial.println("[BT] A2DPCore start");

    ensure_bt_ready();
    if (!bt_ready) {
        Serial.println("[BT] ❌ Classic stack not ready");
        return false;
    }

    esp_bt_dev_set_device_name(device_name);
    Serial.printf("[BT] Device name: %s\n", device_name);

    if (!gap_cb_set) {
        esp_bt_gap_register_callback(raw_gap_cb);
        gap_cb_set = true;
        Serial.println("[BT] GAP callback registered");
    }

    if (!a2dp_started) {
        // Register A2DP callbacks, then init source
        ESP_ERROR_CHECK(esp_a2d_register_callback(a2dp_cb));
        ESP_ERROR_CHECK(
            esp_a2d_source_register_data_callback(
                [](uint8_t* data, int32_t len) -> int32_t {
                    return pcm_cb_ ? pcm_cb_(data, len) : 0;
                })
        );
        ESP_ERROR_CHECK(esp_a2d_source_init());
        a2dp_started = true;
        Serial.println("[A2DP] Source initialised");
    }

    return true;
}

void A2DPCore::stop()
{
    if (a2dp_started) {
        esp_a2d_source_deinit();
        a2dp_started = false;
        Serial.println("[A2DP] Source deinitialised");
    }
}

// =============================================================
// Scan
// =============================================================
void A2DPCore::start_scan(uint32_t seconds)
{
    // Only GAP discovery – no A2DP deinit/re-init games
    found_count = 0;

    Serial.printf("[BT] Scan started (%lus)\n", seconds);

    esp_bt_gap_start_discovery(
        ESP_BT_INQ_MODE_GENERAL_INQUIRY,
        seconds,
        0   // 0 = standard scan interval
    );
}

// =============================================================
// Connect
// =============================================================
bool A2DPCore::connect_by_index(int index)
{
    if (index < 0 || index >= found_count) {
        Serial.println("[A2DP] Invalid device index");
        return false;
    }

    if (!a2dp_started) {
        Serial.println("[A2DP] Source is not started, cannot connect");
        return false;
    }

    char mac[18];
    mac_to_str(found[index].addr, mac);

    Serial.printf("[A2DP] Connecting to %s (%s)\n",
                  found[index].name, mac);

    esp_err_t err = esp_a2d_source_connect(found[index].addr);
    if (err != ESP_OK) {
        Serial.printf("[A2DP] connect error: %s\n", esp_err_to_name(err));
        return false;
    }
    return true;
}

void A2DPCore::disconnect()
{
    esp_a2d_source_disconnect(nullptr);
}

// =============================================================
// GAP callback
// =============================================================
void A2DPCore::raw_gap_cb(
    esp_bt_gap_cb_event_t event,
    esp_bt_gap_cb_param_t* param)
{
    switch (event) {

    case ESP_BT_GAP_DISC_RES_EVT: {
        if (found_count >= MAX_SCAN) return;

        BTDev& d = found[found_count++];
        strcpy(d.name, "Unknown");
        memcpy(d.addr, param->disc_res.bda, sizeof(esp_bd_addr_t));
        d.rssi = -127;

        for (int i = 0; i < param->disc_res.num_prop; ++i) {
            auto* p = &param->disc_res.prop[i];

            if (p->type == ESP_BT_GAP_DEV_PROP_EIR && p->val) {
                resolve_name((const uint8_t*)p->val, d.name, sizeof(d.name));
            } else if (p->type == ESP_BT_GAP_DEV_PROP_RSSI && p->val && p->len == 1) {
                d.rssi = *(int8_t*)p->val;
            }
        }

        Serial.printf("[BT] Found: %s RSSI=%d\n", d.name, d.rssi);
        break;
    }

    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            Serial.printf("[BT] Scan finished (%d devices)\n", found_count);

            if (scan_cb_) {
                static const char* names[MAX_SCAN];
                static const char* macs[MAX_SCAN];
                static int8_t      rssis[MAX_SCAN];
                static char        macbuf[MAX_SCAN][18];

                for (int i = 0; i < found_count; ++i) {
                    names[i] = found[i].name;
                    rssis[i] = found[i].rssi;
                    mac_to_str(found[i].addr, macbuf[i]);
                    macs[i] = macbuf[i];
                }

                scan_cb_(found_count, names, macs, rssis);
            }
        }
        break;
    }

    default:
        break;
    }
}

// =============================================================
// A2DP callback (connection + audio state)
// =============================================================
void A2DPCore::a2dp_cb(
    esp_a2d_cb_event_t event,
    esp_a2d_cb_param_t* param)
{
    switch (event) {

    case ESP_A2D_CONNECTION_STATE_EVT: {
        esp_a2d_connection_state_t st = param->conn_stat.state;
        Serial.printf("[A2DP] Connection state=%d\n", st);

        if (conn_cb_) {
            conn_cb_(st, nullptr);
        }
        break;
    }

    case ESP_A2D_AUDIO_STATE_EVT: {
        esp_a2d_audio_state_t st = param->audio_stat.state;
        Serial.printf("[A2DP] Audio state=%d\n", st);

        if (audio_cb_) {
            audio_cb_(st, nullptr);
        }
        break;
    }

    default:
        break;
    }
}

