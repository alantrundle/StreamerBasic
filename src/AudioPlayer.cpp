#include "AudioPlayer.h"
#include "HttpStreamEngine.h"
#include "AudioCore.h"
#include "A2DPCore.h"

#include "esp_a2dp_api.h"


// --------------------------------------------------
// Playlist
// --------------------------------------------------
static const char* urls[] = {
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

// --------------------------------------------------
// Player state
// --------------------------------------------------
static int  track        = 0;
static bool autoAdvance  = false;
static bool wasPlaying   = false;
static bool isPaused     = false;

// --------------------------------------------------
// INTERNAL helper
// --------------------------------------------------
static void startTrack() {
  Serial.printf("[PLAYER] ▶ Start track %d: %s\n", track, urls[track]);

  // ✅ If player has ever been put into Play mode,
  // autoAdvance must stay enabled
  if (!isPaused)
    autoAdvance = true;

  HttpStreamEngine::open(urls[track]);
  HttpStreamEngine::play();
}

// --------------------------------------------------
// Public API (LVGL buttons)
// --------------------------------------------------
void AudioPlayer_Play() {

  Serial.println("[PLAYER] Play pressed");

  autoAdvance = true;

  // -------- RESUME ----------
  if (isPaused) {

    Serial.println("[PLAYER] Resume requested");

    if (HttpStreamEngine::isAlive()) {
      // Soft resume
      AudioCore::decoder_paused = false;
      AudioCore::StartI2S();
      isPaused = false;
      return;
    }

    // HTTP session died → restart same track
    Serial.println("[PLAYER] Resume failed → restarting track");

    AudioCore::clearPCM();
    HttpStreamEngine::stop();
    startTrack();
    return;
  }

  // -------- FRESH PLAY ----------
  track = 0;
  HttpStreamEngine::stop();
  startTrack();
}

void AudioPlayer_Pause() {

  Serial.println("[PLAYER] Pause pressed");

  if (!HttpStreamEngine::isPlaying())
    return;

  AudioCore::decoder_paused = true;
  AudioCore::StopI2S();
  isPaused = true;
}

void AudioPlayer_Stop() {

  Serial.println("[PLAYER] Stop pressed");

  autoAdvance = false;
  isPaused    = false;

  AudioCore::decoder_paused = false;
  HttpStreamEngine::stop();
}

void AudioPlayer_Next() {
  autoAdvance = false;
  isPaused    = false;

  if (track < PLAYLIST_COUNT - 1)
    track++;

  HttpStreamEngine::stop();
  startTrack();
}


void AudioPlayer_Prev() {
  autoAdvance = false;
  isPaused    = false;

  if (track > 0)
    track--;

  HttpStreamEngine::stop();
  startTrack();
}

// --------------------------------------------------
// Call from main loop()
// --------------------------------------------------
void AudioPlayer_Loop() {

  static bool wasPlaying = false;

  const bool playing = HttpStreamEngine::isPlaying();

  // Detect natural EOF
  if (wasPlaying && !playing) {

    Serial.println("[PLAYER] ▶ Playback stopped");

    if (autoAdvance && !isPaused) {

      track++;
      if (track >= PLAYLIST_COUNT)
        track = 0;

      Serial.printf("[PLAYER] ✅ Auto-advance → track %d\n", track);

      AudioCore::clearPCM();
      HttpStreamEngine::net_ring_clear();
      startTrack();
    }
    else {
      Serial.println("[PLAYER] ⏹ No auto-advance (manual or paused)");
    }
  }

  wasPlaying = playing;
}

int32_t pcm_data_callback(uint8_t* data, int32_t len) {
  return AudioCore::get_pcm_data_a2dp(data, len);
}

void onA2DPConnectionState(esp_a2d_connection_state_t state, void* user) {
  Serial.printf("[APP] A2DP connection state = %d\n", state);

  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    Serial.println("[APP] ✅ Sink Connected — requesting media start");
  }
}


void onA2DPAudioState(esp_a2d_audio_state_t state, void* user) {
  Serial.printf("[APP] A2DP audio state = %d\n", state);

  if (state == ESP_A2D_AUDIO_STATE_STARTED) {
    Serial.println("[APP] ✅ Sink is pulling audio");
    AudioCore::set_a2dp_audio_ready(true);
  } else {
    AudioCore::set_a2dp_audio_ready(false);
  }
}



