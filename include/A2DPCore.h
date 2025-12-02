#pragma once

#include <Arduino.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_bt_device.h"
#include "esp_coexist.h"

typedef struct {
    bool     connected;
    bool     auto_reconnect;
    uint8_t  mac[6];
    char     name[32];     // resolved / EIR
  } A2DPConnectedDetails;

class A2DPCore {
public:
  static constexpr int MAX_SCAN = 10;

  using A2DPConnectionStateCallback =
    void (*)(esp_a2d_connection_state_t, void*);

  using A2DPAudioStateCallback =
    void (*)(esp_a2d_audio_state_t, void*);

  using A2DPScanCallback =
    void (*)(int count,
             const char* const* names,
             const char* const* macs,
             const int8_t* rssi);

  

  A2DPCore();

  void set_device_name(const char* name);
  void set_connectionstate_callback(A2DPConnectionStateCallback cb);
  void set_audiostate_callback(A2DPAudioStateCallback cb);
  void set_scan_callback(A2DPScanCallback cb);
  void set_pcm_callback(esp_a2d_source_data_cb_t cb);
  void set_autoreconnection(bool enable);

  void start();
  void start_scan(uint32_t duration_seconds);
  void stop_scan();
  void connect_by_index(int index);

  // ✅ Auto-reconnect table management
  void autoreconnect_delete(int index);

  bool scan_blocked() const { return block_manual_scan_; }
  bool isConnected() const { return connected_; }
  bool isScanning() const { return scanning_; }

  static void erase_autoreconnect_table();

  bool connected_details(A2DPConnectedDetails& out);

  /* ✅ REQUIRED: call this from your main loop */
  void loop();

private:
  static void gap_cb(esp_bt_gap_cb_event_t,
                     esp_bt_gap_cb_param_t*);
  static void a2dp_cb(esp_a2d_cb_event_t,
                      esp_a2d_cb_param_t*);
  static void avrc_tg_cb(esp_avrc_tg_cb_event_t,
                         esp_avrc_tg_cb_param_t*);

  static A2DPCore* self_;

  static esp_bd_addr_t scan_bda_[MAX_SCAN];
  static char          scan_name_[MAX_SCAN][32];
  static int8_t        scan_rssi_[MAX_SCAN];
  static int           scan_count_;

  char device_name_[32] = {};

  A2DPConnectionStateCallback conn_cb_  = nullptr;
  A2DPAudioStateCallback      audio_cb_ = nullptr;
  A2DPScanCallback            scan_cb_  = nullptr;
  esp_a2d_source_data_cb_t    pcm_cb_   = nullptr;

  bool auto_reconnect_ = true;
  bool scanning_ = false;
  bool connected_ = false;

  // ✅ NEW (minimal)
  bool     block_manual_scan_ = false;
  uint32_t autoreconnect_start_ms_ = 0;

  bool connecting_via_autoreconnect_ = false;
  A2DPConnectedDetails connected_details_ {};
};
