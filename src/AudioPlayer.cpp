#include "AudioPlayer.h"
#include "HttpStreamEngine.h"
#include "AudioCore.h"

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
  HttpStreamEngine::open(urls[track]);
  HttpStreamEngine::play();
}

// --------------------------------------------------
// Public API (LVGL buttons)
// --------------------------------------------------
void AudioPlayer_Play() {

  Serial.println("[PLAYER] Play pressed");

  autoAdvance = true;

  // ✅ If paused, try to resume
  if (isPaused) {

    Serial.println("[PLAYER] Resume requested");

    if (HttpStreamEngine::isAlive()) {
      // Soft resume
      AudioCore::decoder_paused = false;
      AudioCore::StartI2S();
      isPaused = false;
      return;
    }

    // Session died → restart same track
    AudioCore::clearPCM();
    HttpStreamEngine::net_ring_clear();
    startTrack();

    AudioCore::decoder_paused  = false;
    isPaused = false;
    return;
  }

  // ✅ Not paused → fresh Play always starts from track 0
  track = 0;

  HttpStreamEngine::stop();
  startTrack();
}

void AudioPlayer_Pause() {

  Serial.printf("[PLAYER] Pause pressed, DEBUG=%d", HttpStreamEngine::isPlaying());

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

  HttpStreamEngine::stop();
}

void AudioPlayer_Next() {

  Serial.println("[PLAYER] Next pressed");

  autoAdvance = false;
  isPaused    = false;

  if (track < PLAYLIST_COUNT - 1)
    track++;

  Serial.printf("[PLAYER] Next → track %d\n", track);

  HttpStreamEngine::stop();
  startTrack();
}

void AudioPlayer_Prev() {

  Serial.println("[PLAYER] Prev pressed");

  autoAdvance = false;
  isPaused    = false;

  if (track > 0)
    track--;

  Serial.printf("[PLAYER] Prev → track %d\n", track);

  HttpStreamEngine::stop();
  startTrack();
}

// --------------------------------------------------
// Call from main loop()
// --------------------------------------------------
void AudioPlayer_Loop() {

  bool playing = HttpStreamEngine::isPlaying();

  // Auto-advance only on natural EOF
  if (wasPlaying && !playing && autoAdvance && !isPaused) {

    track++;
    if (track >= PLAYLIST_COUNT)
      track = 0;

    Serial.printf("[PLAYER] Auto-advance → track %d\n", track);
    startTrack();
  }

  wasPlaying = playing;
}
