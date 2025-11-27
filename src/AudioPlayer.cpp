#include "AudioPlayer.h"
#include "HttpStreamEngine.h"

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
// State
// --------------------------------------------------
static int  track = 0;
static bool autoAdvance = false;
static bool wasPlaying  = false;

// --------------------------------------------------
static void startTrack() {
  Serial.printf("[PLAYER] START track=%d\n", track);
  HttpStreamEngine::open(urls[track]);
  HttpStreamEngine::play();
}

// --------------------------------------------------
// Controls
// --------------------------------------------------
void AudioPlayer_Play() {

  Serial.println("[PLAYER] PLAY");

  autoAdvance = true;
  track = 0;

  HttpStreamEngine::stop();
  startTrack();
}

void AudioPlayer_Stop() {

  Serial.println("[PLAYER] STOP");

  autoAdvance = false;
  HttpStreamEngine::stop();
}

void AudioPlayer_Next() {

  Serial.println("[PLAYER] NEXT");

  autoAdvance = false;

  if (track < PLAYLIST_COUNT - 1) {
    track++;
  }

  Serial.printf("[PLAYER] track now %d\n", track);

  HttpStreamEngine::close();
  startTrack();
}

void AudioPlayer_Prev() {

  Serial.println("[PLAYER] PREV");

  autoAdvance = false;

  if (track > 0) {
    track--;
  }

  Serial.printf("[PLAYER] track now %d\n", track);

  HttpStreamEngine::close();
  startTrack();
}

// --------------------------------------------------
// Call from loop()
// --------------------------------------------------
void AudioPlayer_Loop() {

  bool playing = HttpStreamEngine::isPlaying();

  // Detect natural end (falling edge)
  if (wasPlaying && !playing && autoAdvance) {

    track++;
    if (track >= PLAYLIST_COUNT)
      track = 0;

    Serial.printf("[PLAYER] AUTO ADVANCE → %d\n", track);
    startTrack();
  }

  wasPlaying = playing;
}
