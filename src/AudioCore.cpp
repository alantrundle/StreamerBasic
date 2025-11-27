
#include "AudioCore.h"
#include "HttpStreamEngine.h"
#include "GraphicsEQ.h"

#include "MP3DecoderHelix.h"
#include "AACDecoderHelix.h"      // <-- ADD

using namespace libhelix;

// ======================================================
// Decoder instances
// ======================================================

MP3DecoderHelix mp3;
AACDecoderHelix aac;

// ======================================================
// Global state
// ======================================================

volatile MP3StatusInfo currentMP3Info = {};
volatile CodecKind     feed_codec     = CODEC_UNKNOWN;

volatile bool decoder_paused = false;
volatile bool i2s_output     = true;
volatile bool a2dp_audio_ready = false;

// ======================================================
// SHARED PCM RING BUFFER
// ======================================================

EXT_RAM_ATTR static uint8_t* a2dp_buffer = nullptr;

static volatile size_t a2dp_write_index    = 0;
static volatile size_t a2dp_read_index     = 0;
static volatile size_t a2dp_read_index_i2s = 0;
static volatile size_t a2dp_buffer_fill    = 0;

static portMUX_TYPE a2dp_mux = portMUX_INITIALIZER_UNLOCKED;

// ======================================================
// Ring helpers
// ======================================================

static inline size_t ring_dist(size_t r, size_t w, size_t cap) {
  return (w >= r) ? (w - r) : (cap - (r - w));
}

// Same but assume caller already holds a2dp_mux
static inline size_t rmin_active_locked() {
  if (a2dp_audio_ready && i2s_output)
    return (a2dp_read_index <= a2dp_read_index_i2s)
           ? a2dp_read_index
           : a2dp_read_index_i2s;
  if (a2dp_audio_ready) return a2dp_read_index;
  if (i2s_output)       return a2dp_read_index_i2s;
  return a2dp_write_index;  // no readers active
}


// ======================================================
// PCM write — ORIGINAL BEHAVIOUR
// ======================================================

static inline void ring_write_pcm_shared(const uint8_t* src, size_t nbytes) {
  if (!src || !nbytes) return;

  portENTER_CRITICAL(&a2dp_mux);

  size_t rmin = rmin_active_locked();
  size_t used = ring_dist(rmin, a2dp_write_index, PCM_BUFFER_BYTES);
  size_t free_bytes = PCM_BUFFER_BYTES - used;

  if (free_bytes >= nbytes) {
    size_t first = min(nbytes, PCM_BUFFER_BYTES - a2dp_write_index);
    memcpy(a2dp_buffer + a2dp_write_index, src, first);
    if (first < nbytes)
      memcpy(a2dp_buffer, src + first, nbytes - first);

    a2dp_write_index =
      (a2dp_write_index + nbytes) % PCM_BUFFER_BYTES;

    a2dp_buffer_fill =
      ring_dist(rmin_active_locked(), a2dp_write_index, PCM_BUFFER_BYTES);
  }

  portEXIT_CRITICAL(&a2dp_mux);
}

// ======================================================
// Public helpers
// ======================================================

int AudioCore::pcm_buffer_percent() {
  portENTER_CRITICAL(&a2dp_mux);
  size_t fill = a2dp_buffer_fill;
  portEXIT_CRITICAL(&a2dp_mux);
  return (int)(fill * 100 / PCM_BUFFER_BYTES);
}

int AudioCore::net_filled_slots() {
  int n = 0;
  for (int i = 0; i < NUM_BUFFERS; ++i)
    if (HttpStreamEngine::netBufFilled[i]) n++;
  return n;
}

// ======================================================
// I2S setup
// ======================================================

static void setupI2S() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = I2S_DMA_BUF_COUNT,
    .dma_buf_len = I2S_DMA_BUF_LEN,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_BCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL) != ESP_OK) {
    i2s_output = false;
    return;
  }

  if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) {
    i2s_driver_uninstall(I2S_NUM_0);
    i2s_output = false;
  }
}

// ======================================================
// Helix callbacks — EXACT
// ======================================================

void mp3dataCallback(MP3FrameInfo& info, int16_t* pcm, size_t len, void*) {
  if (!pcm || len == 0) return;

  size_t total_samples = len;
  if (info.nChans == 2 && (total_samples & 1)) total_samples--;

  static uint16_t last_eq_sr = 0;
  if (last_eq_sr != info.samprate) {
    eq_set_samplerate(info.samprate);
    last_eq_sr = info.samprate;
  }

  currentMP3Info.samplerate = info.samprate;
  currentMP3Info.channels  = info.nChans;
  currentMP3Info.layer     = info.layer;
  currentMP3Info.kbps      = info.bitrate / 1000;

  eq_process_block(pcm, total_samples);
  ring_write_pcm_shared((uint8_t*)pcm,
                        total_samples * sizeof(int16_t));
}

int32_t AudioCore::get_data(uint8_t *data, int32_t bytes) {
    // fill the channel silence data
    memset(data, 0, bytes);
    return bytes;
}

void aacDataCallback(AACFrameInfo& info, int16_t* pcm, size_t len, void*) {
  if (!pcm || len == 0) return;

  size_t total_samples = len;
  if (info.nChans == 2 && (total_samples & 1)) total_samples--;

  static uint16_t last_eq_sr = 0;
  if (last_eq_sr != info.sampRateOut) {
    eq_set_samplerate(info.sampRateOut);
    last_eq_sr = info.sampRateOut;
  }

  currentMP3Info.samplerate = info.sampRateOut;
  currentMP3Info.channels  = info.nChans;

  eq_process_block(pcm, total_samples);
  ring_write_pcm_shared((uint8_t*)pcm,
                        total_samples * sizeof(int16_t));
}

bool AudioCore::init() {

  a2dp_buffer = (uint8_t*)heap_caps_malloc(
      PCM_BUFFER_BYTES,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!a2dp_buffer) {
    Serial.println("[Audio] ❌ PCM PSRAM alloc failed");
    return false;
  }

  memset(a2dp_buffer, 0, PCM_BUFFER_BYTES);

  a2dp_write_index    = 0;
  a2dp_read_index_i2s = 0;
  a2dp_buffer_fill   = 0;

  setupI2S();

  // Helix callbacks
  mp3.setDataCallback(mp3dataCallback);
  aac.setDataCallback(aacDataCallback);

  // Decoder task (core 1, higher prio)
  xTaskCreatePinnedToCore(
      decodeTask,
      "DECODE",
      8192,
      nullptr,
      2,
      nullptr,
      1);

  // I2S output task (core 1, lower prio)
  xTaskCreatePinnedToCore(
      i2sPlaybackTask,
      "I2S",
      4096,
      nullptr,
      1,
      nullptr,
      1);

  Serial.println("[Audio] ✅ AudioCore initialised (shared ring)");
  return true;
}

void AudioCore::i2sPlaybackTask(void* /*param*/) {
  // Match your I²S DMA frame length (keep this = i2s_config.dma_buf_len)
  constexpr size_t FRAMES_PER_CHUNK = 256;
  constexpr size_t BYTES_PER_FRAME  = 4;                  // L16 + R16
  constexpr size_t CHUNK_BYTES      = FRAMES_PER_CHUNK * BYTES_PER_FRAME;

  // Small staging buffer on stack (1 KB)
  uint8_t tempBuf[CHUNK_BYTES];

  for (;;) {
    bool output_any = false;  // did we actually push audio this iteration?

    // Fast exit if output is gated
    if (!i2s_output) {
      // Not allowed to output right now → tiny nap to avoid busy spin
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    // Snapshot indices once; compute availability without holding the lock lon
    size_t read_idx_snapshot, write_idx_snapshot, avail;
    portENTER_CRITICAL(&a2dp_mux);
    read_idx_snapshot  = a2dp_read_index_i2s;
    write_idx_snapshot = a2dp_write_index;
    avail = ring_dist(read_idx_snapshot, write_idx_snapshot, A2DP_BUFFER_SIZE);
    portEXIT_CRITICAL(&a2dp_mux);

    if (avail < CHUNK_BYTES) {
      // Not enough PCM yet; short, cooperative nap
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    // Copy CHUNK_BYTES from ring → tempBuf, handling wrap — outside the lock
    const size_t first = std::min(CHUNK_BYTES, A2DP_BUFFER_SIZE - read_idx_snapshot);
    memcpy(tempBuf, &a2dp_buffer[read_idx_snapshot], first);
    if (first < CHUNK_BYTES) {
      memcpy(tempBuf + first, &a2dp_buffer[0], CHUNK_BYTES - first);
    }

    // Now advance the I²S reader atomically and refresh shared fill
    portENTER_CRITICAL(&a2dp_mux);
    a2dp_read_index_i2s = (read_idx_snapshot + CHUNK_BYTES) % A2DP_BUFFER_SIZE;
    a2dp_buffer_fill    = ring_dist(rmin_active_locked(), a2dp_write_index, A2DP_BUFFER_SIZE);
    portEXIT_CRITICAL(&a2dp_mux);

    // Push exactly one DMA chunk; short timeout to stay responsive
    size_t written = 0;
    esp_err_t res = i2s_write(I2S_NUM_0, tempBuf, CHUNK_BYTES, &written, pdMS_TO_TICKS(5));
    if (res == ESP_OK && written > 0) {
      output_any = true;
    }

    // If the driver returned early, top off without blocking the system
    while (written < CHUNK_BYTES) {
      size_t w2 = 0;
      res = i2s_write(I2S_NUM_0,
                      tempBuf + written,
                      CHUNK_BYTES - written,
                      &w2,
                      pdMS_TO_TICKS(5));
      written += w2;

      if (res == ESP_OK && w2 > 0) {
        output_any = true;
      }

      // DMA saturated / nothing more written → brief backoff
      if (w2 == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        break;
      }
    }

    // Only back off if we didn’t really output anything this loop
    if (!output_any) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    // else: no explicit delay — time spent in i2s_write is enough,
    // scheduler can preempt us as needed.
  }

}


// ======================================================
// Decode task — UNCHANGED
// ======================================================
void AudioCore::decodeTask(void*) {

  constexpr int HI_PCT    = 90;
  constexpr int LO_PCT    = 70;
  constexpr int PRIME_PCT = 20;

  const size_t HI_BYTES   = (PCM_BUFFER_BYTES * HI_PCT) / 100;
  const size_t LO_BYTES   = (PCM_BUFFER_BYTES * LO_PCT) / 100;
  const size_t PRIME_SLOTS = (NUM_BUFFERS * PRIME_PCT) / 100;

  CodecKind active_codec = CODEC_UNKNOWN;
  uint32_t  last_session = 0xFFFFFFFF;
  bool      priming      = true;
  bool      drained      = false;   // ✅ NEW: EOS guard

  for (;;) {

    // --------------------------------------------------
    // Stream inactive → DRAIN, NOT ABORT  ✅ FIXED
    // --------------------------------------------------
    if (!HttpStreamEngine::stream_running) {

      // ✅ If NET still has data, KEEP decoding
      if (net_filled_slots() > 0) {
        drained = false;
        // fall through to normal decode path
      }
      else {
        // ✅ NET empty → flush decoder ONCE
        if (!drained) {
          if (active_codec == CODEC_MP3)
            mp3.write(nullptr, 0);
          else if (active_codec == CODEC_AAC)
            aac.write(nullptr, 0);
          drained = true;
        }

        priming        = true;
        decoder_paused = false;
        vTaskDelay(pdMS_TO_TICKS(5));
        continue;
      }
    }

    // --------------------------------------------------
    // NET priming phase (unchanged)
    // --------------------------------------------------
    if (priming) {
      if (net_filled_slots() < (int)PRIME_SLOTS) {
        vTaskDelay(5);
        continue;
      }
      priming = false;
      drained = false;
    }

    // --------------------------------------------------
    // PCM buffer hysteresis (unchanged)
    // --------------------------------------------------
    portENTER_CRITICAL(&a2dp_mux);
    size_t fill = a2dp_buffer_fill;
    portEXIT_CRITICAL(&a2dp_mux);

    if (!decoder_paused && fill >= HI_BYTES)
      decoder_paused = true;
    else if (decoder_paused && fill <= LO_BYTES)
      decoder_paused = false;

    if (decoder_paused) {
      vTaskDelay(5);
      continue;
    }

    // --------------------------------------------------
    // Wait for NET slot (unchanged)
    // --------------------------------------------------
    if (!HttpStreamEngine::netBufFilled[HttpStreamEngine::netRead]) {
      vTaskDelay(1);
      continue;
    }

    const int slot = HttpStreamEngine::netRead;

    uint8_t* ptr    = HttpStreamEngine::netBuffers[slot];
    uint16_t bytes  = HttpStreamEngine::netSize[slot];
    uint16_t offset = HttpStreamEngine::netOffset[slot];
    uint32_t sess   = HttpStreamEngine::netSess[slot];
    uint8_t  tag    = HttpStreamEngine::netTag[slot];

    // --------------------------------------------------
    // New session → hard reset (unchanged)
    // --------------------------------------------------
    if (sess != last_session) {
      if (active_codec == CODEC_MP3) mp3.end();
      if (active_codec == CODEC_AAC) aac.end();
      active_codec = CODEC_UNKNOWN;
      last_session = sess;
      priming      = true;
      drained      = false;
      continue;
    }

    // --------------------------------------------------
    // Codec selection (unchanged)
    // --------------------------------------------------
    if (tag == SLOT_MP3 && active_codec != CODEC_MP3) {
      mp3.begin();
      active_codec = CODEC_MP3;
      feed_codec   = CODEC_MP3;
    }
    else if (tag == SLOT_AAC && active_codec != CODEC_AAC) {
      aac.begin();
      active_codec = CODEC_AAC;
      feed_codec   = CODEC_AAC;
    }

    // --------------------------------------------------
    // Apply offset (unchanged)
    // --------------------------------------------------
    if (offset < bytes) {
      ptr   += offset;
      bytes -= offset;
    }

    // --------------------------------------------------
    // Feed decoder (unchanged)
    // --------------------------------------------------
    if (bytes) {
      if (active_codec == CODEC_MP3)
        mp3.write(ptr, bytes);
      else if (active_codec == CODEC_AAC)
        aac.write(ptr, bytes);
    }

    // --------------------------------------------------
    // Release NET slot (unchanged)
    // --------------------------------------------------
    HttpStreamEngine::netBufFilled[slot] = false;
    HttpStreamEngine::netSize[slot]      = 0;
    HttpStreamEngine::netRead =
      (HttpStreamEngine::netRead + 1) % NUM_BUFFERS;

    vTaskDelay(5);   // ✅ correct placement preserved
  }
}

