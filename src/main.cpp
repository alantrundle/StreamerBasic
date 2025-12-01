#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"

#include "Config.h"
#include "AudioCore.h"
#include "HttpStreamEngine.h"
#include "A2DPCore.h"

#include "LVGLCore.h"
#include "ui/ui.h"

#include "AudioPlayer.h"

#include "sdkconfig.h"

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



  Serial.printf("\n[WIFI] ✅ Connected, IP=%s\n",
                WiFi.localIP().toString().c_str());
}

void btScanCallback(int count, const char* const* names, const char* const* macs, const int8_t* rssis) {

  static bool connect = false;

  if (count > 0 && !connect) {
    Serial.printf("Connecting to %s\r\n", names[0]);
    a2dp.connect_by_index(0);
    connect = true;
  }

}

void memUsage() {

size_t dma_free = heap_caps_get_largest_free_block(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
size_t int_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

Serial.printf("DMA largest free: %u bytes\n", dma_free);
Serial.printf("Internal free:   %u bytes\n", int_free);

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
  esp_wifi_set_ps(WIFI_PS_NONE);
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

  esp_bt_sleep_disable();

  // enable Player controls button now were up!
  lv_obj_clear_state(objects.start_btn, LV_STATE_DISABLED);

  memUsage();
}

// ------------------------------------------------------------
// Arduino loop (control plane only)
// ------------------------------------------------------------
static bool scan_started = false;

void loop() {

    a2dp.loop();
    AudioPlayer_Loop();

    //if (!a2dp.isConnected() && !a2dp.scan_blocked() && !scan_started) {
    //    a2dp.start_scan(10);
    //    scan_started = true;
    //}

  }