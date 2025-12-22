#include "AudioPlayer.h"
#include "HttpStreamEngine.h"
#include "AudioCore.h"
#include "A2DPCore.h"

#include "esp_a2dp_api.h"


// --------------------------------------------------
// Playlist
// --------------------------------------------------
static const char* urls[] = {
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/01.That's%20What%20Love%20Is%20All%20About.mp3",
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/02.(Sittin'%20On)%20The%20Dock%20Of%20The%20Bay.mp3",
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/03.Soul%20Provider.mp3",
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/04.How%20Am%20I%20Supposed%20To%20Live%20Without%20You.mp3",
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/05.How%20Can%20We%20Be%20Lovers.mp3",
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/06.When%20I'm%20Back%20On%20My%20Feet%20Again.mp3,"
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/07.Georgia%20On%20My%20Mind.mp3",
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/08.Time,%20Love%20And%20Tenderness.mp3",
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/09.When%20A%20Man%20Loves%20A%20Woman.mp3",
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/10.Missing%20You%20Now.mp3",
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/11.Steel%20Bars.mp3",
  "http://81.2.125.100/codec_board/Michael%20Bolton/Greatest%20Hits%201985-1995%20(2003)/12.Said%20I%20Loved%20You...But%20I%20Lied.mp3"
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



