#pragma once

#include <Arduino.h>
#include <driver/i2s.h>
#include "Config.h"

enum CodecKind : uint8_t {
  CODEC_UNKNOWN = 0,
  CODEC_MP3,
  CODEC_AAC
};

struct MP3StatusInfo {
  uint32_t samplerate;
  uint16_t kbps;
  uint8_t  channels;
  uint8_t  layer;
};

class AudioCore {
public:
  static bool init();
  static void decodeTask(void*);
  static int32_t get_data(uint8_t *data, int32_t bytes);
  static void i2sPlaybackTask(void*);
  static int  pcm_buffer_percent();
  static int  net_filled_slots();

  static void clearPCM();

  static void StartI2S();
  static void StopI2S();

  static bool decoder_paused;

private:

  static bool decoder_auto_paused;   // AUTO pause (buffer)
};
