#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include <string.h>

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



static int g_bt_scan_count = 0;

void btScanCallback(int count, const char* const* names, const char* const* macs, const int8_t* rssis)
{
    g_bt_scan_count = count;

    static char opts[512];   // bump if you expect long names / many devices
    opts[0] = '\0';

    for (int i = 0; i < count; i++) {

        const char* nm = (names && names[i] && names[i][0]) ? names[i] : "Unknown";

        if (opts[0] != '\0') strlcat(opts, "\n", sizeof(opts));
        strlcat(opts, nm, sizeof(opts));
    }

    // Update dropdown (assumes this callback is safe to touch LVGL)
    if (objects.bt_devicelist) {
        lv_dropdown_clear_options(objects.bt_devicelist);

        if (count > 0) {
            lv_dropdown_set_options(objects.bt_devicelist, opts);
        } else {
            lv_dropdown_set_options(objects.bt_devicelist, "No devices");
        }
    }
}

void memUsage() {

size_t dma_free = heap_caps_get_largest_free_block(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
size_t int_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

Serial.printf("DMA largest free: %u bytes\n", dma_free);
Serial.printf("Internal free:   %u bytes\n", int_free);

}

static const char* wifi_ps_str(wifi_ps_type_t ps) {
  switch (ps) {
    case WIFI_PS_NONE:      return "NONE";
    case WIFI_PS_MIN_MODEM: return "MIN";
    case WIFI_PS_MAX_MODEM: return "MAX";
    default:               return "?";
  }
}


void health_log() {
  static uint32_t last = 0;
  uint32_t now = millis();

  if (now - last < 5UL * 60UL * 1000UL)
    return;
  last = now;

  // ---- Heap ----
  uint32_t int_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  uint32_t int_min  = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);

  uint32_t dma_free = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
  uint32_t dma_min  = heap_caps_get_minimum_free_size(MALLOC_CAP_DMA);

  // ---- WiFi info ----
  wifi_ps_type_t ps;
  esp_wifi_get_ps(&ps);

  wifi_ap_record_t ap{};
  bool ap_ok = (esp_wifi_sta_get_ap_info(&ap) == ESP_OK);

  // ---- Log ----
  Serial.printf(
    "[HEALTH] up=%lus "
    "Int=%u(min=%u) "
    "DMA=%u(min=%u) "
    "WiFi=%d PS=%s "
    "RSSI=%d Ch=%d "
    "BT=%d "
    "PCM=%d%% NET=%d%%\n",
    now / 1000,

    int_free, int_min,
    dma_free, dma_min,

    WiFi.status(),
    wifi_ps_str(ps),

    ap_ok ? ap.rssi : 0,
    ap_ok ? ap.primary : 0,

    a2dp.isConnected(),
    AudioCore::pcm_buffer_percent(),
    HttpStreamEngine::net_fill_percent()
  );
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
  //esp_wifi_set_ps(WIFI_PS_NONE);
  
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

  connectWiFi();

  // enable Player controls button now were up!
  lv_obj_clear_state(objects.btn_menu_player, LV_STATE_DISABLED);

  memUsage();
}

// ------------------------------------------------------------
// Arduino loop (control plane only)
// ------------------------------------------------------------
static bool scan_started = false;

void loop() {

    a2dp.loop();
    AudioPlayer_Loop();

    health_log();

    //if (!a2dp.isConnected() && !a2dp.scan_blocked() && !scan_started) {
    //    a2dp.start_scan(10);
    //    scan_started = true;
    //}

    delay(100);

  }