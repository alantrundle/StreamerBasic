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
  char* codec;
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

  static bool getMP3Info(MP3StatusInfo& out);


  static bool decoder_paused;

  enum PCMConsumer : uint8_t {
  PCM_CONSUMER_A2DP = 0,
  PCM_CONSUMER_I2S
  };

  static int32_t get_pcm_data(PCMConsumer consumer,
                              uint8_t* data,
                              int32_t bytes);

  // Optional wrappers for convenience
  static inline int32_t get_pcm_data_a2dp(uint8_t* d, int32_t b) {
    return get_pcm_data(PCM_CONSUMER_A2DP, d, b);
  }

  static inline int32_t get_pcm_data_i2s(uint8_t* d, int32_t b) {
    return get_pcm_data(PCM_CONSUMER_I2S, d, b);
  }

  // Output enable / readiness
  static void set_i2s_output(bool enabled);
  static void set_a2dp_output(bool a2dp_output);
  static void set_a2dp_audio_ready(bool ready);

  static bool is_i2s_output_enabled();
  static bool is_a2dp_output_enabled();
  static bool is_a2dp_audio_ready();


private:

  static bool decoder_auto_paused;   // AUTO pause (buffer)
};
