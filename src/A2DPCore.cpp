#include "A2DPCore.h"
#include <string.h>

#include "AudioCore.h"

// === AUTO-RECONNECT (NVS)
#include "nvs.h"
#include "nvs_flash.h"

#define AR_NAMESPACE "a2dp_ar"
#define AR_KEY       "table"
#define AR_MAX       1

typedef struct {
  uint8_t mac[6];
  char    name[32];
} AutoReconnectEntry;

volatile A2DPNvState A2DPCore::nv_state_ = A2DPNvState::NONE;
char A2DPCore::nv_saved_name_[32] = {0};


// ------------------------------------------------------------
// Static storage
// ------------------------------------------------------------

A2DPCore* A2DPCore::self_ = nullptr;

esp_bd_addr_t A2DPCore::scan_bda_[MAX_SCAN];
char          A2DPCore::scan_name_[MAX_SCAN][32];
int8_t        A2DPCore::scan_rssi_[MAX_SCAN];
int           A2DPCore::scan_count_ = 0;

// === AUTO-RECONNECT (NVS)
static AutoReconnectEntry ar_table[AR_MAX];

static esp_timer_handle_t s_a2dp_kick_timer = nullptr;
static void a2dp_kick_cb(void* arg);


// ------------------------------------------------------------
// NVS helpers
// ------------------------------------------------------------

static void ar_init()
{
  nvs_handle_t nvs;
  if (nvs_open(AR_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK) return;

  size_t len = 0;
  esp_err_t r = nvs_get_blob(nvs, AR_KEY, nullptr, &len);

  // Create new blob if missing OR wrong size (e.g. AR_MAX changed)
  if (r == ESP_ERR_NVS_NOT_FOUND || len != sizeof(ar_table)) {

    memset(ar_table, 0, sizeof(ar_table));

    // If an old key exists with different size, erase then recreate
    nvs_erase_key(nvs, AR_KEY);
    nvs_set_blob(nvs, AR_KEY, ar_table, sizeof(ar_table));
    nvs_commit(nvs);

    Serial.printf("[A2DP] 🧹 NV table initialized (size=%u)\n",
                  (unsigned)sizeof(ar_table));
  }

  nvs_close(nvs);
}

static void ar_load()
{
  memset(ar_table, 0, sizeof(ar_table));

  nvs_handle_t nvs;
  size_t len = sizeof(ar_table);

  if (nvs_open(AR_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) {
    // Treat as empty
    A2DPCore::nv_saved_name_[0] = '\0';
    A2DPCore::nv_state_ = A2DPNvState::NONE;
    return;
  }

  esp_err_t r = nvs_get_blob(nvs, AR_KEY, ar_table, &len);

  // Wrong size or missing key -> keep cleared
  if (r != ESP_OK || len != sizeof(ar_table)) {
    memset(ar_table, 0, sizeof(ar_table));
  }

  nvs_close(nvs);

  // Update cached UI name + state (no NVS reads needed later)
  if (ar_table[0].mac[0]) {
    strlcpy(A2DPCore::nv_saved_name_,
            ar_table[0].name[0] ? ar_table[0].name : "Unknown",
            sizeof(A2DPCore::nv_saved_name_));
    A2DPCore::nv_state_ = A2DPNvState::LOADED;
  } else {
    A2DPCore::nv_saved_name_[0] = '\0';
    A2DPCore::nv_state_ = A2DPNvState::NONE;
  }
}

static void ar_store()
{
  nvs_handle_t nvs;
  if (nvs_open(AR_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK) return;

  nvs_set_blob(nvs, AR_KEY, ar_table, sizeof(ar_table));
  nvs_commit(nvs);
  nvs_close(nvs);

  // ✅ Update cached name + NV state (NO LVGL calls here)
  if (ar_table[0].mac[0]) {
    strlcpy(A2DPCore::nv_saved_name_,
            ar_table[0].name[0] ? ar_table[0].name : "Unknown",
            sizeof(A2DPCore::nv_saved_name_));
    A2DPCore::nv_state_ = A2DPNvState::SAVED;
  } else {
    A2DPCore::nv_saved_name_[0] = '\0';
    A2DPCore::nv_state_ = A2DPNvState::ERASED;
  }
}


void A2DPCore::erase_autoreconnect_table()
{
  memset(ar_table, 0, sizeof(ar_table));

  nvs_handle_t nvs;
  if (nvs_open(AR_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {

    nvs_erase_key(nvs, AR_KEY);
    nvs_commit(nvs);
    nvs_close(nvs);

    // ✅ Update cached UI name + state
    nv_saved_name_[0] = '\0';
    nv_state_ = A2DPNvState::ERASED;

    Serial.println("[A2DP] 🗑 Auto-reconnect table erased");
  } else {
    Serial.println("[A2DP] ❌ Failed to erase NV table");
  }
}

A2DPCore::A2DPCore() {
  self_ = this;
}

// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------

void A2DPCore::set_device_name(const char* name) {
  if (name) {
    strlcpy(device_name_, name, sizeof(device_name_));
  }
}

void A2DPCore::set_connectionstate_callback(A2DPConnectionStateCallback cb) {
  conn_cb_ = cb;
}

void A2DPCore::set_audiostate_callback(A2DPAudioStateCallback cb) {
  audio_cb_ = cb;
}

void A2DPCore::set_scan_callback(A2DPScanCallback cb) {
  scan_cb_ = cb;
}

void A2DPCore::set_pcm_callback(esp_a2d_source_data_cb_t cb) {
  pcm_cb_ = cb;
}

void A2DPCore::set_autoreconnection(bool enable) {
  auto_reconnect_ = enable;
  Serial.printf("[A2DP] 🔁 Auto-reconnect %s\n",
                enable ? "ENABLED" : "DISABLED");
}

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
static bool mac_is_zero(const uint8_t* mac) {
  for (int i = 0; i < 6; i++) if (mac[i] != 0x00) return false;
  return true;
}

static bool mac_equal(const uint8_t* a, const uint8_t* b) {
  return memcmp(a, b, 6) == 0;
}

bool A2DPCore::poll_nv_state(A2DPNvState& state, char* name, size_t name_len)
{
  static A2DPNvState last = A2DPNvState::NONE;

  A2DPNvState now = nv_state_;
  if (now == last) return false;

  last = now;
  state = now;

  if (name && name_len) {
    if (nv_saved_name_[0]) strlcpy(name, nv_saved_name_, name_len);
    else name[0] = '\0';
  }

  return true;
}


// Overwrite slot 0 (AR_MAX=1) with MAC+name and commit.
// If same MAC already stored, it will only update the name if different.
static void ar_store_last_device(const uint8_t* mac, const char* name)
{
  if (!mac || mac_is_zero(mac)) return;

  // Build new entry
  AutoReconnectEntry e;
  memset(&e, 0, sizeof(e));
  memcpy(e.mac, mac, 6);

  if (name && name[0]) {
    strlcpy(e.name, name, sizeof(e.name));
  } else {
    strlcpy(e.name, "Unknown", sizeof(e.name));
  }

  // Decide whether we actually need to write
  bool need_write = true;

  if (ar_table[0].mac[0] && mac_equal(ar_table[0].mac, e.mac)) {
    if (strncmp(ar_table[0].name, e.name, sizeof(ar_table[0].name)) == 0) {
      need_write = false; // identical
    }
  }

  // Overwrite RAM table always
  ar_table[0] = e;

  if (!need_write) return;

  // ✅ Single path: commit + update enum/name
  ar_store();

  Serial.printf("[A2DP] 💾 NV overwritten: %s (%02X:%02X:%02X:%02X:%02X:%02X)\n",
                ar_table[0].name,
                ar_table[0].mac[0], ar_table[0].mac[1], ar_table[0].mac[2],
                ar_table[0].mac[3], ar_table[0].mac[4], ar_table[0].mac[5]);
}

// ------------------------------------------------------------
// Start Bluetooth stack
// ------------------------------------------------------------
void A2DPCore::start()
{
  ar_init();
  ar_load();

  if (!btStarted()) {
    if (!btStart()) {
      Serial.println("[BT] ❌ btStart failed");
      return;
    }
  }

  if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED)
    esp_bluedroid_init();
  if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED)
    esp_bluedroid_enable();

  esp_bt_gap_register_callback(gap_cb);
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE,
                           ESP_BT_GENERAL_DISCOVERABLE);

  if (device_name_[0]) {
    esp_bt_dev_set_device_name(device_name_);
    Serial.printf("[BT] Device name: %s\n", device_name_);
  }

  esp_a2d_register_callback(a2dp_cb);
  if (pcm_cb_) esp_a2d_source_register_data_callback(pcm_cb_);
  esp_avrc_tg_register_callback(avrc_tg_cb);
  esp_avrc_tg_init();
  esp_a2d_source_init();

  // ===== AUTO-RECONNECT =====
    // ===== AUTO-RECONNECT =====
  if (auto_reconnect_) {

    block_manual_scan_ = true;
    autoreconnect_start_ms_ = millis();

    // If we have a saved device, schedule a delayed + retried connect.
    for (int i = 0; i < AR_MAX; i++) {

      if (ar_table[i].mac[0] == 0x00)
        continue;

      Serial.printf("[A2DP] 🔁 Auto-connect scheduled: %s\n",
        ar_table[i].name[0] ? ar_table[i].name : "Unknown");

      // Mark as pending; loop() will attempt after a delay.
      memcpy(auto_conn_mac_, ar_table[i].mac, 6);
      auto_conn_pending_ = true;

      auto_conn_attempts_left_ = AUTO_CONN_MAX_ATTEMPTS;
      auto_conn_next_ms_ = millis() + AUTO_CONN_FIRST_DELAY_MS;

      break;
    }
  }

  Serial.println("[A2DP] ✅ Source ready");
}

// ------------------------------------------------------------

void A2DPCore::loop()
{
  // Unblock manual scan after your existing timeout
  if (block_manual_scan_) {
    if (millis() - autoreconnect_start_ms_ >= 5000) {
      block_manual_scan_ = false;
      Serial.println("[A2DP] 🔓 Manual scan unblocked");
    }
  }

  // ------------------------------------------------------------
  // Auto-connect retry engine (non-blocking)
  // ------------------------------------------------------------
  if (!auto_conn_pending_)
    return;

  // If already connected, stop retrying
  if (connected_) {
    auto_conn_pending_ = false;
    auto_conn_attempts_left_ = 0;
    return;
  }

  // Nothing to do yet
  uint32_t now = millis();
  if ((int32_t)(now - auto_conn_next_ms_) < 0)
    return;

  // Out of attempts
  if (auto_conn_attempts_left_ == 0) {
    Serial.println("[A2DP] ❌ Auto-connect gave up (no response)");
    auto_conn_pending_ = false;
    return;
  }

  // Attempt connect
  auto_conn_attempts_left_--;

  Serial.printf("[A2DP] 🔁 Auto-connect attempt (%u left)\n",
                (unsigned)auto_conn_attempts_left_);

  // This is informational now; your CONNECTED handler uses ar_contains_mac() anyway
  connecting_via_autoreconnect_ = true;

  esp_err_t r = esp_a2d_source_connect(auto_conn_mac_);
  if (r != ESP_OK) {
    Serial.printf("[A2DP] ❌ esp_a2d_source_connect failed: %s\n",
                  esp_err_to_name(r));
  }

  // Schedule next retry
  auto_conn_next_ms_ = now + AUTO_CONN_RETRY_MS;
}

static bool ar_contains_mac(const uint8_t* mac)
{
    for (int i = 0; i < AR_MAX; i++) {
        if (ar_table[i].mac[0] &&
            memcmp(ar_table[i].mac, mac, 6) == 0) {
            return true;
        }
    }
    return false;
}

// ------------------------------------------------------------
// Scan / connect
// ------------------------------------------------------------

void A2DPCore::start_scan(uint32_t duration_seconds)
{
  if (block_manual_scan_) {
    Serial.println("[A2DP] 🚫 Scan blocked");
    scanning_ = false;
    return;
  }

  scan_count_ = 0;

  esp_err_t r = esp_bt_gap_start_discovery(
      ESP_BT_INQ_MODE_GENERAL_INQUIRY,
      duration_seconds,
      0
  );

  if (r != ESP_OK) {
    scanning_ = false;
    Serial.printf("[BT] ❌ start_discovery failed: %s\n", esp_err_to_name(r));
    return;
  }

  scanning_ = true;
  Serial.printf("[BT] 🔍 Scanning (%u sec)\n", duration_seconds);
}

void A2DPCore::stop_scan()
{
  esp_err_t r = esp_bt_gap_cancel_discovery();

  // We consider "stop requested" as not scanning from the UI POV.
  // GAP callback should also come in and keep it consistent.
  scanning_ = false;

  if (r != ESP_OK) {
    Serial.printf("[BT] ❌ cancel_discovery failed: %s\n", esp_err_to_name(r));
  } else {
    Serial.println("[BT] 🛑 Scan cancel requested");
  }
}

void A2DPCore::connect_by_index(int index)
{
  if (index < 0 || index >= scan_count_)
    return;

  // Manual connection overrides auto-reconnect
  connecting_via_autoreconnect_ = false;

  const char* nm = scan_name_[index][0] ? scan_name_[index] : "Unknown";

  // ✅ ALWAYS overwrite slot 0 (AR_MAX=1)
  ar_store_last_device(scan_bda_[index], nm);

  esp_a2d_source_connect(scan_bda_[index]);
}

void A2DPCore::disconnect()
{
  if (!connected_)
    return;

  Serial.println("[A2DP] 🔌 Disconnect requested");

  esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_STOP);

  esp_a2d_source_disconnect(connected_details_.mac);
}


// ------------------------------------------------------------
// Delete NV entry
// ------------------------------------------------------------

void A2DPCore::autoreconnect_delete(int index) {
  if (index < 0 || index >= AR_MAX) return;
  memset(&ar_table[index], 0, sizeof(AutoReconnectEntry));
  ar_store();
  Serial.printf("[A2DP] 🗑 Deleted NV[%d]\n", index);
}

bool A2DPCore::connected_details(A2DPConnectedDetails& out)
{
  if (!connected_details_.connected)
    return false;

  out = connected_details_;
  return true;
}

// ------------------------------------------------------------
// GAP callback (EIR name support)
// ------------------------------------------------------------
void A2DPCore::gap_cb(esp_bt_gap_cb_event_t event,
                      esp_bt_gap_cb_param_t* param)
{
  if (!self_) return;

  // Helper: find existing scan index by MAC
  auto find_scan_index = [](const esp_bd_addr_t bda) -> int {
    for (int i = 0; i < scan_count_; i++) {
      if (memcmp(scan_bda_[i], bda, sizeof(esp_bd_addr_t)) == 0) {
        return i;
      }
    }
    return -1;
  };

  switch (event) {

    // --------------------------------------------------
    // DEVICE FOUND
    // --------------------------------------------------
    case ESP_BT_GAP_DISC_RES_EVT: {

      // Extract bda early
      const esp_bd_addr_t& bda = param->disc_res.bda;

      // --- De-dupe by MAC ---
      int idx = find_scan_index(bda);
      bool is_new = (idx < 0);

      if (is_new) {
        if (scan_count_ >= MAX_SCAN) break;
        idx = scan_count_;

        memcpy(scan_bda_[idx], bda, sizeof(esp_bd_addr_t));

        // Default placeholders
        scan_name_[idx][0] = '\0';
        scan_rssi_[idx]    = -127;
      }

      uint8_t* eir = nullptr;

      // ------------------------------
      // Collect EIR + RSSI
      // ------------------------------
      for (int i = 0; i < param->disc_res.num_prop; i++) {

        esp_bt_gap_dev_prop_t* p = &param->disc_res.prop[i];

        if (p->type == ESP_BT_GAP_DEV_PROP_EIR && p->val) {
          eir = (uint8_t*)p->val;
        }
        else if (p->type == ESP_BT_GAP_DEV_PROP_RSSI && p->val) {
          scan_rssi_[idx] = *(int8_t*)p->val;
        }
      }

      // ------------------------------
      // Resolve name from EIR (if any)
      // ------------------------------
      char resolved_name[32] = {0};

      if (eir) {
        uint8_t name_len = 0;
        uint8_t* name = esp_bt_gap_resolve_eir_data(
            eir,
            ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME,
            &name_len);

        if (!name) {
          name = esp_bt_gap_resolve_eir_data(
              eir,
              ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME,
              &name_len);
        }

        if (name && name_len > 0) {
          size_t copy_len = name_len;
          if (copy_len >= sizeof(resolved_name))
            copy_len = sizeof(resolved_name) - 1;

          memcpy(resolved_name, name, copy_len);
          resolved_name[copy_len] = '\0';

          // ✅ Trim trailing non-printable
          for (int j = (int)copy_len - 1; j >= 0; j--) {
            uint8_t c = (uint8_t)resolved_name[j];
            if (c >= 32 && c <= 126) break;
            resolved_name[j] = '\0';
          }
        }
      }

      // ------------------------------
      // Ensure we always have a usable name
      // ------------------------------
      const bool got_new_name = (resolved_name[0] != '\0');
      const bool existing_blank = (scan_name_[idx][0] == '\0');
      const bool existing_unknown =
          (strncmp(scan_name_[idx], "Unknown", sizeof(scan_name_[idx])) == 0);

      // If it’s a new device, set name immediately.
      if (is_new) {
        if (got_new_name) {
          strlcpy(scan_name_[idx], resolved_name, sizeof(scan_name_[idx]));
        } else {
          strlcpy(scan_name_[idx], "Unknown", sizeof(scan_name_[idx]));
        }

        scan_count_++; // ✅ only increment for NEW unique MAC
      }
      // If it’s an existing MAC, only upgrade the name if we’ve learned better
      else {
        if (got_new_name && (existing_blank || existing_unknown)) {
          strlcpy(scan_name_[idx], resolved_name, sizeof(scan_name_[idx]));
        }
        // else: keep the existing name (don’t downgrade)
      }

      Serial.printf(
        "[BT] 📡 Found: %s RSSI=%d %s\n",
        scan_name_[idx][0] ? scan_name_[idx] : "Unknown",
        scan_rssi_[idx],
        is_new ? "(new)" : "(dup)"
      );

      break;
    }

    // --------------------------------------------------
    // SCAN STATE CHANGED
    // --------------------------------------------------
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {

      if (param->disc_st_chg.state ==
          ESP_BT_GAP_DISCOVERY_STOPPED) {

        self_->scanning_ = false;

        Serial.printf(
          "[BT] ✅ Scan complete (%d unique device%s)\n",
          scan_count_,
          scan_count_ == 1 ? "" : "s"
        );

        if (self_->scan_cb_) {

          static const char* names[MAX_SCAN];
          static const char* macs[MAX_SCAN];
          static char macbuf[MAX_SCAN][18];

          for (int i = 0; i < scan_count_; i++) {

            // ✅ Always return a name string
            names[i] = (scan_name_[i][0] ? scan_name_[i] : "Unknown");

            snprintf(macbuf[i], sizeof(macbuf[i]),
                     "%02X:%02X:%02X:%02X:%02X:%02X",
                     scan_bda_[i][0], scan_bda_[i][1],
                     scan_bda_[i][2], scan_bda_[i][3],
                     scan_bda_[i][4], scan_bda_[i][5]);

            macs[i] = macbuf[i];
          }

          self_->scan_cb_(scan_count_, names, macs, scan_rssi_);
        }
      }
      break;
    }

    default:
      break;
  }
}

// ------------------------------------------------------------
// A2DP callback (UNCHANGED)
// ------------------------------------------------------------

static esp_timer_handle_t s_kick = nullptr;

static void kick_cb(void*) {
  esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY);
}

static void a2dp_kick_cb(void* arg)
{
  esp_err_t r = esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY);
  if (r != ESP_OK) {
    Serial.printf("[A2DP] ❌ CHECK_SRC_RDY failed: %s\n",
                  esp_err_to_name(r));
  } else {
    Serial.println("[A2DP] ✅ CHECK_SRC_RDY sent (deferred)");
  }
}

void A2DPCore::a2dp_cb(esp_a2d_cb_event_t event,
                       esp_a2d_cb_param_t* param)
{
  if (!self_)
    return;

  switch (event) {

  // ==========================================================
  // CONNECTION STATE
  // ==========================================================
  case ESP_A2D_CONNECTION_STATE_EVT: {

    esp_a2d_connection_state_t s = param->conn_stat.state;
    Serial.printf("[A2DP] 🔗 Connection state = %d\n", s);

    // ===================== CONNECTED =====================
    if (s == ESP_A2D_CONNECTION_STATE_CONNECTED) {

      esp_bt_sleep_disable();

      self_->connected_ = true;
      self_->block_manual_scan_ = false;

      // Audio not ready until START ACK
      AudioCore::set_a2dp_audio_ready(false);

      // Clear + populate connected details
      memset(&self_->connected_details_, 0,
             sizeof(self_->connected_details_));

      self_->connected_details_.connected = true;

      // ✅ Treat "AUTO" as: device matches the saved NV entry
      // (robust even if timing flips connecting_via_autoreconnect_)
      self_->connected_details_.auto_reconnect = ar_contains_mac(param->conn_stat.remote_bda);

      // One-shot flag (still clear it, but no longer authoritative)
      self_->connecting_via_autoreconnect_ = false;

      // Store MAC
      memcpy(self_->connected_details_.mac,
             param->conn_stat.remote_bda, 6);

      // -------- Resolve name (priority order) --------
      // 1) Auto-reconnect table
      for (int i = 0; i < AR_MAX; i++) {
        if (ar_table[i].mac[0] &&
            memcmp(ar_table[i].mac,
                   param->conn_stat.remote_bda, 6) == 0) {

          strncpy(self_->connected_details_.name,
                  ar_table[i].name,
                  sizeof(self_->connected_details_.name) - 1);
          break;
        }
      }

      // 2) Last scan results
      if (!self_->connected_details_.name[0]) {
        for (int i = 0; i < scan_count_; i++) {
          if (memcmp(scan_bda_[i],
                     param->conn_stat.remote_bda, 6) == 0) {

            strncpy(self_->connected_details_.name,
                    scan_name_[i][0]
                      ? scan_name_[i]
                      : "Unknown",
                    sizeof(self_->connected_details_.name) - 1);
            break;
          }
        }
      }

      // 3) Fallback
      if (!self_->connected_details_.name[0]) {
        strncpy(self_->connected_details_.name,
                "Unknown",
                sizeof(self_->connected_details_.name) - 1);
      }

      Serial.printf(
        "[A2DP] ✅ Connected: %s (%02X:%02X:%02X:%02X:%02X:%02X) [%s]\n",
        self_->connected_details_.name,
        self_->connected_details_.mac[0],
        self_->connected_details_.mac[1],
        self_->connected_details_.mac[2],
        self_->connected_details_.mac[3],
        self_->connected_details_.mac[4],
        self_->connected_details_.mac[5],
        self_->connected_details_.auto_reconnect ? "AUTO" : "MANUAL"
      );

      // ---- Defer CHECK_SRC_RDY (required) ----
      if (!s_a2dp_kick_timer) {
        esp_timer_create_args_t args = {};
        args.callback = &a2dp_kick_cb;
        args.name     = "a2dp_kick";
        esp_timer_create(&args, &s_a2dp_kick_timer);
      }

      esp_timer_start_once(s_a2dp_kick_timer, 100000); // 100 ms
    }

    // ===================== DISCONNECTED =====================
    else if (s == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {

      esp_bt_sleep_enable();

      self_->connected_ = false;
      self_->connecting_via_autoreconnect_ = false;

      AudioCore::set_a2dp_audio_ready(false);

      memset(&self_->connected_details_, 0,
             sizeof(self_->connected_details_));

      Serial.println("[A2DP] ❌ Device disconnected");
    }

    // Notify application
    if (self_->conn_cb_) {
      self_->conn_cb_(s, nullptr);
    }
    break;
  }

  // ==========================================================
  // MEDIA CONTROL ACKS (AUTHORITATIVE STREAM CONTROL)
  // ==========================================================
  case ESP_A2D_MEDIA_CTRL_ACK_EVT: {

    auto cmd = param->media_ctrl_stat.cmd;
    auto st  = param->media_ctrl_stat.status;

    if (cmd == ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY &&
        st  == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS) {

      Serial.println("[A2DP] ▶ Source ready — starting stream");

      esp_err_t r =
        esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);

      if (r != ESP_OK) {
        Serial.printf("[A2DP] ❌ START failed: %s\n",
                      esp_err_to_name(r));
      }
    }

    if (cmd == ESP_A2D_MEDIA_CTRL_START &&
        st  == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS) {

      Serial.println("[A2DP] ✅ Media START confirmed");
      AudioCore::set_a2dp_audio_ready(true);
    } else {
      AudioCore::set_a2dp_audio_ready(false);
    }

    if (cmd == ESP_A2D_MEDIA_CTRL_STOP ||
        cmd == ESP_A2D_MEDIA_CTRL_SUSPEND) {

        Serial.println("[A2DP] ⏹ Media stopped");
      
    }

    break;
  }

  // ==========================================================
  // AUDIO STATE (informational only)
  // ==========================================================
  case ESP_A2D_AUDIO_STATE_EVT: {

    if (self_->audio_cb_) {
      self_->audio_cb_(param->audio_stat.state, nullptr);
    }
    break;
  }

  default:
    break;
  }
}


// ------------------------------------------------------------

void A2DPCore::avrc_tg_cb(esp_avrc_tg_cb_event_t,
                          esp_avrc_tg_cb_param_t*) {}
