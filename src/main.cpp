#include <Arduino.h>
#include <WiFi.h>

#include "Config.h"
#include "AudioCore.h"
#include "HttpStreamEngine.h"

#include "LVGLCore.h"

#include "AudioPlayer.h"


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
    if (millis() - start > 15000) {
      Serial.println("\n[WIFI] ❌ Connection timeout");
      return;
    }
  }

  Serial.printf("\n[WIFI] ✅ Connected, IP=%s\n",
                WiFi.localIP().toString().c_str());
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