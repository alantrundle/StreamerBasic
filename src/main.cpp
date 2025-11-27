#include <Arduino.h>
#include <WiFi.h>

#include "Config.h"
#include "AudioCore.h"
#include "HttpStreamEngine.h"

// ------------------------------------------------------------
// Playlists / URLs
// ------------------------------------------------------------

const char* urls[] = {
  "http://81.2.125.100/codec_board/01%20-%20Sultans%20Of%20Swing.mp3",
  "http://81.2.125.100/codec_board/10-Spin%20Doctors%20-%20Two%20princes.mp3",
  "http://81.2.125.100/codec_board/10-Sweet%20Little%20Mystery.mp3",
  "http://81.2.125.100/codec_board/09-Private%20investigations.mp3",
  "http://81.2.125.100/codec_board/Cheek%20to%20Cheek/15-Lady%20In%20Red.mp3",
  "http://81.2.125.100/codec_board/Cheek%20to%20Cheek/01-I%20Want%20To%20Know%20What%20Love%20Is.mp3",
  "http://81.2.125.100/codec_board/Cheek%20to%20Cheek/12-Baby%20I%20Love%20Your%20Way%20_%20Freebird.mp3",
  "http://81.2.125.100/codec_board/09%20-%20Without%20Me.mp3",
  "http://81.2.125.100/codec_board/Steel%20Bars.mp3",
  "http://81.2.125.100/codec_board/sample4.aac"
};

static constexpr int PLAYLIST_COUNT =
    sizeof(urls) / sizeof(urls[0]);

static int currentTrack = 0;

void playPlayList();

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
  playPlayList();


  Serial.println("[MAIN] ▶ Playback started");
}

static int track = 0;

void playPlayList() {

  if (HttpStreamEngine::isPlaying()) 
    return;

  HttpStreamEngine::stop();

  HttpStreamEngine::open(urls[track]);
  HttpStreamEngine::play();

  track = (track + 1) % PLAYLIST_COUNT;
}


// ------------------------------------------------------------
// Arduino loop (control plane only)
// ------------------------------------------------------------

void loop() {
  // Nothing time-critical lives here anymore.
  // This is where future controls will go:
  //
  // - next / previous track
  // - stop / play
  // - EQ updates
  // - OLED updates

  //playNext();
  //
  delay(1000);
}
