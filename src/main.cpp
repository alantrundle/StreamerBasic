#include <Arduino.h>
#include <WiFi.h>

#include "Config.h"
#include "AudioCore.h"
#include "HttpStreamEngine.h"
#include "A2DPCore.h"


#include "LVGLCore.h"
#include "ui/ui.h"

#include "AudioPlayer.h"

A2DPCore a2dp;


// ------------------------------------------------------------
// Wi-Fi helper
// ------------------------------------------------------------

static void connectWiFi() {
  Serial.printf("[WIFI] Connecting to %s\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
    if (millis() - start > 30000) {
      Serial.println("\n[WIFI] ❌ Connection timeout");
      return;
    }
  }

  // enable Player controls button
  lv_obj_clear_state(objects.start_btn, LV_STATE_DISABLED);

  Serial.printf("\n[WIFI] ✅ Connected, IP=%s\n",
                WiFi.localIP().toString().c_str());
}

void btScanCallback(int count, const char* const* names, const char* const* macs, const int8_t* rssis) {

  if (count > 0) {
    a2dp.connect_by_index(0);
  }

}

// ------------------------------------------------------------
// Arduino setup
// ------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("\n=== StreamerBasic ===");

  lvgl_init();

  // 1. Wi-Fi
  connectWiFi();

  // 2. Audio core (PCM, EQ, I2S, decoder tasks)
  if (!AudioCore::init()) {
    Serial.println("[MAIN] ❌ AudioCore init failed");
    while (true) delay(1000);
  }


  // 3. HTTP streamer (NET buffers + task)
  HttpStreamEngine::begin();

  a2dp.set_device_name("AT1053-Source");
  a2dp.set_autoreconnection(true);
  a2dp.set_pcm_callback(pcm_data_callback);
  a2dp.set_audiostate_callback(onA2DPAudioState);
  a2dp.set_connectionstate_callback(onA2DPConnectionState);
  a2dp.set_scan_callback(btScanCallback);
  a2dp.start();

  a2dp.start_scan(10);
  
  //HttpStreamEngine::open(urls[0]);
  //HttpStreamEngine::play();
}

// ------------------------------------------------------------
// Arduino loop (control plane only)
// ------------------------------------------------------------

void loop() {

  AudioPlayer_Loop();

  delay(100);
}