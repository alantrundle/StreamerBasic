// include/header_detection.h
// Exact, scan-anywhere audio header detector for ESP32/Arduino (PlatformIO)
// Order: Containers (WAV/FLAC/OGG/ASF) → AAC (ADTS/LOAS/ADIF) → MP3 → MIDI.
// Returns enum + offset to first decodable frame/page. No heuristics.
//
// Public API:
//   bool detect_audio_format_strict(const uint8_t* buf, int len, DetectResult* out);
//
// MIT-licensed snippet

#ifndef HEADER_DETECTION_H
#define HEADER_DETECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

namespace audetect {

typedef enum {
  AF_UNKNOWN=100, AF_WAV, AF_FLAC, AF_OGG_VORBIS, AF_WMA_ASF,
  AF_AAC_ADTS, AF_AAC_LOAS, AF_AAC_ADIF, AF_MP3, AF_MIDI
} AudioFormat;

typedef struct {
  AudioFormat format;
  int offset;             // byte index to first frame/page to feed decoder

  // Parsed fields (optional)
  // AAC (ADTS)
  bool     aac_is_mpeg2;
  uint8_t  aac_aot;       // Audio Object Type (ADTS stores AOT-1)
  int      aac_samplerate;// Hz
  int      aac_channels;  // 0 => PCE

  // MP3
  int mp3_version;        // 10,20,25
  int mp3_layer;          // 1..3
  int mp3_bitrate_kbps;
  int mp3_samplerate;     // Hz
  int mp3_channels;       // 1/2

  // WAV
  uint16_t wav_fmt;       // 1=PCM, 0x11=IMA-ADPCM
  uint16_t wav_channels;
  uint32_t wav_samplerate;
  uint16_t wav_bits;
} DetectResult;

// Returns true if a format was detected and fills out; false otherwise.
bool detect_audio_format_strict(const uint8_t* buf, int len, DetectResult* out);

} // namespace audetect

#endif // HEADER_DETECTION_H
