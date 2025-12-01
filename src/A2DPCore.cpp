#include "A2DPCore.h"
#include <string.h>

#include "AudioCore.h"

// ------------------------------------------------------------
// Static storage
// ------------------------------------------------------------

A2DPCore* A2DPCore::self_ = nullptr;

esp_bd_addr_t A2DPCore::scan_bda_[MAX_SCAN];
char          A2DPCore::scan_name_[MAX_SCAN][32];
int8_t        A2DPCore::scan_rssi_[MAX_SCAN];
int           A2DPCore::scan_count_ = 0;

// ------------------------------------------------------------
// Ctor
// ------------------------------------------------------------

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

void A2DPCore::set_sleepmode(bool enable) {

  if(enable && connected_)
    esp_bt_sleep_disable();
  else 
    esp_bt_sleep_enable();
    
    Serial.printf("[A2DP] 🔁 BT Sleep %s\n",
                enable ? "ENABLED" : "DISABLED");
}

// ------------------------------------------------------------
// Start Bluetooth stack (⚠️ DO NOT CHANGE ORDER ⚠️)
// ------------------------------------------------------------

void A2DPCore::start() {

  esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

  if (!btStarted()) {
    if (!btStart()) {
      Serial.println("[BT] ❌ btStart failed");
      return;
    }
  }

  if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
    esp_bluedroid_init();
  }
  if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
    esp_bluedroid_enable();
  }

  esp_coex_preference_set(ESP_COEX_PREFER_BALANCE);

  esp_bt_gap_register_callback(gap_cb);
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE,
                           ESP_BT_GENERAL_DISCOVERABLE);

  if (device_name_[0]) {
    esp_bt_dev_set_device_name(device_name_);
    Serial.printf("[BT] Device name: %s\n", device_name_);
  }

  esp_a2d_register_callback(a2dp_cb);

  if (pcm_cb_) {
    esp_a2d_source_register_data_callback(pcm_cb_);
    Serial.println("[A2DP] PCM callback registered");
  }

  esp_avrc_tg_register_callback(avrc_tg_cb);
  esp_avrc_tg_init();

  esp_a2d_source_init();

  // prevent sleep
  //esp_wifi_set_ps(WIFI_PS_NONE);
  //esp_bt_sleep_disable();

  // ✅ NEW: block manual scans during auto-reconnect window ONLY
  if (auto_reconnect_) {
    block_manual_scan_ = true;
    autoreconnect_start_ms_ = millis();
  }

  Serial.println("[A2DP] ✅ Source ready");
}

// ------------------------------------------------------------
// Loop (AUTO-RECONNECT SCAN UNBLOCK ONLY)
// ------------------------------------------------------------

void A2DPCore::loop() {
  if (!block_manual_scan_) return;

  if (millis() - autoreconnect_start_ms_ >= 5000) {
    block_manual_scan_ = false;
    Serial.println("[A2DP] 🔓 Manual scan unblocked (auto-reconnect done)");
  }
}

// ------------------------------------------------------------
// Scan / connect
// ------------------------------------------------------------

void A2DPCore::start_scan(uint32_t duration_seconds) {

  // ✅ ONLY behavior change (as requested)
  if (block_manual_scan_) {
    Serial.println("[A2DP] 🚫 Scan blocked (auto-reconnect running)");
    return;
  }

  scan_count_ = 0;
  scanning_   = true;    // ✅ ADD THIS
  Serial.printf("[BT] 🔍 Scanning (%u sec)...\n", duration_seconds);

  esp_bt_gap_start_discovery(
    ESP_BT_INQ_MODE_GENERAL_INQUIRY,
    duration_seconds,
    0
  );
}

void A2DPCore::stop_scan() {

  Serial.println("[BT] ⏹ Stopping discovery");

  esp_bt_gap_cancel_discovery();
}

void A2DPCore::connect_by_index(int index) {
  if (index < 0 || index >= scan_count_) return;

  Serial.printf(
    "[BT] 🔗 Connecting to %s\n",
    scan_name_[index][0] ? scan_name_[index] : "Unknown"
  );

  esp_a2d_source_connect(scan_bda_[index]);
}

// ------------------------------------------------------------
// GAP callback (UNCHANGED)
// ------------------------------------------------------------

void A2DPCore::gap_cb(esp_bt_gap_cb_event_t event,
                      esp_bt_gap_cb_param_t* param) {

  if (!self_) return;

  switch (event) {

    case ESP_BT_GAP_DISC_RES_EVT: {
      if (scan_count_ >= MAX_SCAN) break;

      memcpy(scan_bda_[scan_count_],
             param->disc_res.bda,
             sizeof(esp_bd_addr_t));

      scan_name_[scan_count_][0] = 0;
      scan_rssi_[scan_count_] = -127;

      for (int i = 0; i < param->disc_res.num_prop; i++) {
        esp_bt_gap_dev_prop_t* p = &param->disc_res.prop[i];

        if (p->type == ESP_BT_GAP_DEV_PROP_BDNAME && p->val) {
          strncpy(scan_name_[scan_count_],
                  (const char*)p->val,
                  sizeof(scan_name_[scan_count_]) - 1);
        }
        else if (p->type == ESP_BT_GAP_DEV_PROP_RSSI && p->val) {
          scan_rssi_[scan_count_] = *(int8_t*)p->val;
        }
      }

      Serial.printf(
        "[BT] 📡 Found: %s RSSI=%d\n",
        scan_name_[scan_count_][0] ?
          scan_name_[scan_count_] : "Unknown",
        scan_rssi_[scan_count_]
      );

      scan_count_++;
      break;
    }

    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
      if (param->disc_st_chg.state ==
          ESP_BT_GAP_DISCOVERY_STOPPED) {

        Serial.printf(
          "[BT] ✅ Scan complete (%d device%s)\n",
          scan_count_,
          scan_count_ == 1 ? "" : "s");
          self_->scanning_ = false;   // ✅ ADD THIS

        if (self_->scan_cb_) {
          static const char* names[MAX_SCAN];
          static const char* macs[MAX_SCAN];
          static char macbuf[MAX_SCAN][18];

          for (int i = 0; i < scan_count_; i++) {
            names[i] = scan_name_[i][0] ?
                        scan_name_[i] : "Unknown";
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

    default:
      break;
  }
}

// ------------------------------------------------------------
// A2DP callback
// ------------------------------------------------------------
static esp_timer_handle_t s_a2dp_kick_timer = nullptr;

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
                       esp_a2d_cb_param_t* param) {
  if (!self_) return;

  switch (event) {

    // --------------------------------------------------
    // CONNECTION STATE
    // --------------------------------------------------
    case ESP_A2D_CONNECTION_STATE_EVT: {
      esp_a2d_connection_state_t s = param->conn_stat.state;

      Serial.printf("[A2DP] 🔗 Connection state = %d\n", s);

      if (s == ESP_A2D_CONNECTION_STATE_CONNECTED) {

        self_->connected_         = true;
        self_->block_manual_scan_ = false;

        // ✅ Defer CHECK_SRC_RDY by 100 ms (SAFE)
        if (!s_a2dp_kick_timer) {
          esp_timer_create_args_t args = {};
          args.callback = &a2dp_kick_cb;
          args.name = "a2dp_kick";

          esp_timer_create(&args, &s_a2dp_kick_timer);
        }

        esp_timer_start_once(s_a2dp_kick_timer, 100000); // 100ms
      }
      else if (s == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {

        self_->connected_ = false;
        AudioCore::set_a2dp_audio_ready(false);
      }

      if (self_->conn_cb_) {
        self_->conn_cb_(s, nullptr);
      }
      break;
    }

    // --------------------------------------------------
    // MEDIA CONTROL ACKS
    // --------------------------------------------------
    case ESP_A2D_MEDIA_CTRL_ACK_EVT: {

      auto cmd = param->media_ctrl_stat.cmd;
      auto st  = param->media_ctrl_stat.status;

      if (cmd == ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY) {

        if (st == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS) {

          Serial.println("[A2DP] ▶ Source ready — starting stream");

          esp_err_t r = esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);
          if (r != ESP_OK) {
            Serial.printf("[A2DP] ❌ START failed: %s\n",
                          esp_err_to_name(r));
          }

        } else {
          Serial.printf("[A2DP] SRC not ready (ack=%d)\n", (int)st);
        }
      }
      else if (cmd == ESP_A2D_MEDIA_CTRL_START) {
        Serial.printf("[A2DP] ▶ START ack=%d\n", (int)st);
      }
      break;
    }

    // --------------------------------------------------
    // AUDIO STREAM STATE
    // --------------------------------------------------
    case ESP_A2D_AUDIO_STATE_EVT: {
      esp_a2d_audio_state_t s = param->audio_stat.state;

      AudioCore::set_a2dp_audio_ready(
        s == ESP_A2D_AUDIO_STATE_STARTED
      );

      if (self_->audio_cb_) {
        self_->audio_cb_(s, nullptr);
      }
      break;
    }

    default:
      break;
  }
}



// ------------------------------------------------------------
// AVRCP TG (unchanged)
// ------------------------------------------------------------

void A2DPCore::avrc_tg_cb(esp_avrc_tg_cb_event_t, esp_avrc_tg_cb_param_t*) {

  
}
