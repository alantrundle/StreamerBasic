#pragma once

#include <stdint.h>
#include "esp_a2dp_api.h"
#include "esp_gap_bt_api.h"

// Thin wrapper around ESP-IDF Classic A2DP Source + GAP scan
class A2DPCore {
public:
    using pcm_callback_t  = int32_t (*)(uint8_t* data, int32_t len);
    using scan_callback_t = void (*)(int count,
                                     const char* const* names,
                                     const char* const* macs,
                                     const int8_t*     rssis);
    using a2dp_conn_cb_t  = void (*)(esp_a2d_connection_state_t state, void* user);
    using a2dp_audio_cb_t = void (*)(esp_a2d_audio_state_t      state, void* user);

    A2DPCore();

    // Configuration
    void set_device_name(const char* name);
    void set_pcm_callback(pcm_callback_t cb);
    void set_scan_callback(scan_callback_t cb);
    void set_connectstate_callback(a2dp_conn_cb_t cb);
    void set_audiostate_callback(a2dp_audio_cb_t cb);

    // Lifecycle
    bool start();              // bring up Classic BT + A2DP source
    void stop();               // stop A2DP source (does NOT tear down controller)
    void start_scan(uint32_t seconds);
    bool connect_by_index(int index);
    void disconnect();

private:
    // Internal GAP + A2DP callbacks
    static void raw_gap_cb(esp_bt_gap_cb_event_t event,
                           esp_bt_gap_cb_param_t* param);
    static void a2dp_cb(esp_a2d_cb_event_t event,
                        esp_a2d_cb_param_t* param);

    // Classic BT bring-up (controller + bluedroid)
    static void ensure_bt_ready();

    // Static config
    static const char*  device_name;
    static pcm_callback_t  pcm_cb_;
    static scan_callback_t scan_cb_;
    static a2dp_conn_cb_t  conn_cb_;
    static a2dp_audio_cb_t audio_cb_;
};

