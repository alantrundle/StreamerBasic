#include "AudioPlayer.h"
#include "HttpStreamEngine.h"
#include "AudioCore.h"
#include "A2DPCore.h"

#include "esp_a2dp_api.h"


// --------------------------------------------------
// Playlist
// --------------------------------------------------
static const char* urls[] = {
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/01%20-%20Intro%20(Curtain%20Call).mp3",
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/02%20-%20Fack.mp3",
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/03%20-%20The%20Way%20I%20Am.mp3",
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/04%20-%20My%20Name%20Is.mp3",
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/05%20-%20Stan%20(Feat-%20Dido).mp3",
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/06%20-%20Lose%20Yourself.mp3",
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/07%20-%20Shake%20That%20(Feat-%20Nate%20Dogg).mp3",
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/08%20-%20Sing%20For%20The%20Moment.mp3",
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/09%20-%20Without%20Me.mp3",
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/10%20-%20Like%20Toy%20Soldiers.mp3",
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/11%20-%20The%20Real%20Slim%20Shady.mp3",
 "http://81.2.125.100/codec_board/Eminem/Curtain%20Call-%20The%20Hits%20(2005)/12%20-%20Mockingbird.mp3"
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
  AudioCore::set_a2dp_output(false);
  AudioCore::StopI2S();
  isPaused = true;
}

void AudioPlayer_Stop() {

  Serial.println("[PLAYER] Stop pressed");

  autoAdvance = false;
  isPaused    = false;

  AudioCore::decoder_paused = false;
  AudioCore::set_a2dp_output(false);
  AudioCore::set_a2dp_output(false);

  HttpStreamEngine::stop();
}

void AudioPlayer_Next() {
  autoAdvance = false;
  isPaused    = false;

  if (track < PLAYLIST_COUNT - 1)
    track++;

  HttpStreamEngine::stop();
  delay(100);
  startTrack();
}

void AudioPlayer_Prev() {
  autoAdvance = false;
  isPaused    = false;

  if (track > 0)
    track--;

  HttpStreamEngine::stop();
  delay(100);
  startTrack();
}

// --------------------------------------------------
// Call from main loop()
// --------------------------------------------------
void AudioPlayer_Loop()
{
  const bool playing =
    HttpStreamEngine::stream_running ||
    AudioCore::pcm_buffer_percent() > 0;

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


// A2DP output
int32_t pcm_data_callback(uint8_t* data, int32_t len)
{
    if (!data || len <= 0) {
        return 0;   // only invalid case allowed
    }

    // --------------------------------------------------
    // A2DP not ready → silence
    // --------------------------------------------------
    if (!AudioCore::is_a2dp_audio_ready() ||
        !AudioCore::is_a2dp_output_enabled()) {
        memset(data, 0, len);
        return len;
    }

    // --------------------------------------------------
    // Encoder warm-up (BT codec stability)
    // --------------------------------------------------
    static uint8_t warmup = 0;
    constexpr uint8_t WARMUP_FRAMES = 3;

    if (warmup < WARMUP_FRAMES) {
        memset(data, 0, len);
        warmup++;
        return len;
    }

    // --------------------------------------------------
    // PCM supply with SILENCE FALLBACK
    // --------------------------------------------------
    int32_t got = AudioCore::get_pcm_data_a2dp(data, len);

    if (got < len) {
        // PCM underrun → pad with zeros
        memset(data + got, 0, len - got);
        return len;
    }

    return len;
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
    AudioCore::set_a2dp_output(true);
    AudioCore::set_a2dp_audio_ready(true);
  } else {
    AudioCore::set_a2dp_audio_ready(false);
  }
}



