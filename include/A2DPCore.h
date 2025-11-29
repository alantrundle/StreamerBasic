#pragma once

#include <Arduino.h>

#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_coexist.h"

// ------------------------------------------------------------
// Public callback types
// ------------------------------------------------------------

typedef void (*A2DPConnectionStateCallback)(
    esp_a2d_connection_state_t state,
    void* user
);

typedef void (*A2DPAudioStateCallback)(
    esp_a2d_audio_state_t state,
    void* user
);

typedef void (*A2DPScanCallback)(
    int count,
    const char* const* names,
    const char* const* macs,
    const int8_t* rssis
);

// ------------------------------------------------------------

class A2DPCore {
public:
  A2DPCore();

  // Configuration (set BEFORE start)
  void set_device_name(const char* name);

  void set_connectionstate_callback(A2DPConnectionStateCallback cb);
  void set_audiostate_callback(A2DPAudioStateCallback cb);
  void set_scan_callback(A2DPScanCallback cb);
  void set_pcm_callback(esp_a2d_source_data_cb_t cb);

  // 🔁 Auto reconnect (new, minimal)
  void set_autoreconnection(bool enable);

  // Control
  void start();
  void start_scan(uint32_t duration_seconds);
  void connect_by_index(int index);

private:
  // ESP-IDF callbacks
  static void a2dp_cb(esp_a2d_cb_event_t event,
                      esp_a2d_cb_param_t* param);

  static void gap_cb(esp_bt_gap_cb_event_t event,
                     esp_bt_gap_cb_param_t* param);

  static void avrc_tg_cb(esp_avrc_tg_cb_event_t event,
                         esp_avrc_tg_cb_param_t* param);

  // Singleton instance
  static A2DPCore* self_;

  // Registered callbacks
  A2DPConnectionStateCallback conn_cb_  = nullptr;
  A2DPAudioStateCallback      audio_cb_ = nullptr;
  A2DPScanCallback            scan_cb_  = nullptr;
  esp_a2d_source_data_cb_t    pcm_cb_   = nullptr;

  // Device name
  char device_name_[32] = {0};

  // 🔁 Auto reconnect flag
  bool auto_reconnect_ = true;

  // ----------------------------------------------------------
  // Scan storage — MUST match cpp exactly
  // ----------------------------------------------------------

  static constexpr int MAX_SCAN = 8;

  static esp_bd_addr_t scan_bda_[MAX_SCAN];
  static char          scan_name_[MAX_SCAN][32];
  static int8_t        scan_rssi_[MAX_SCAN];
  static int           scan_count_;
};
