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

bool AudioCore::decoder_paused = false;
bool AudioCore::decoder_auto_paused = false;

static volatile bool i2s_output     = true;
static volatile bool a2dp_output     = false;
static volatile bool a2dp_audio_ready = false;


CodecKind active_codec = CODEC_UNKNOWN;

// ======================================================
// SHARED PCM RING BUFFER=
// =====================================================

EXT_RAM_ATTR static uint8_t* a2dp_buffer = nullptr;

static volatile size_t a2dp_write_index    = 0;
static volatile size_t a2dp_read_index     = 0;
static volatile size_t a2dp_read_index_i2s = 0;
static volatile size_t a2dp_buffer_fill    = 0;

static portMUX_TYPE a2dp_mux = portMUX_INITIALIZER_UNLOCKED;

TaskHandle_t i2STask;

void AudioCore::set_i2s_output(bool enabled) {
  portENTER_CRITICAL(&a2dp_mux);
  i2s_output = enabled;
  portEXIT_CRITICAL(&a2dp_mux);
}

void AudioCore::set_a2dp_output(bool enabled) {
  portENTER_CRITICAL(&a2dp_mux);
  a2dp_output = enabled;
  portEXIT_CRITICAL(&a2dp_mux);
}

void AudioCore::set_a2dp_audio_ready(bool ready) {
  portENTER_CRITICAL(&a2dp_mux);
  a2dp_audio_ready = ready;
  portEXIT_CRITICAL(&a2dp_mux);
}

bool AudioCore::is_i2s_output_enabled() {
  return i2s_output;
}

bool AudioCore::is_a2dp_output_enabled() {
  return a2dp_output;
}

bool AudioCore::is_a2dp_audio_ready() {
  return a2dp_audio_ready;
}

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

int32_t AudioCore::get_pcm_data(PCMConsumer consumer,
                                uint8_t* data,
                                int32_t len) {
  if (!data || len <= 0)
    return 0;

  // Gate by consumer readiness
  if (consumer == PCM_CONSUMER_A2DP && !a2dp_audio_ready)
    return 0;

  if (consumer == PCM_CONSUMER_I2S && !i2s_output)
    return 0;

  portENTER_CRITICAL(&a2dp_mux);

  // Re-check under lock (disconnect / stop can race)
  if (consumer == PCM_CONSUMER_A2DP && !a2dp_audio_ready) {
    portEXIT_CRITICAL(&a2dp_mux);
    return 0;
  }
  if (consumer == PCM_CONSUMER_I2S && !i2s_output) {
    portEXIT_CRITICAL(&a2dp_mux);
    return 0;
  }

  const size_t cap = PCM_BUFFER_BYTES;
  if (!a2dp_buffer || cap == 0) {
    portEXIT_CRITICAL(&a2dp_mux);
    return 0;
  }

  // Select correct read pointer
  size_t* rd =
    (consumer == PCM_CONSUMER_A2DP)
      ? (size_t*)&a2dp_read_index
      : (size_t*)&a2dp_read_index_i2s;

  const size_t r0 = *rd;
  const size_t w0 = a2dp_write_index;

  size_t avail = ring_dist(r0, w0, cap);
  if (avail == 0) {
    portEXIT_CRITICAL(&a2dp_mux);
    return 0;
  }

  size_t to_copy = (size_t)len;
  if (to_copy > avail)
    to_copy = avail;

  // Optional: keep 16-bit stereo alignment happy
  // to_copy &= ~0x03;

  const size_t space_to_end = cap - r0;
  const size_t first = (to_copy < space_to_end)
                       ? to_copy
                       : space_to_end;

  memcpy(data, a2dp_buffer + r0, first);
  if (first < to_copy) {
    memcpy(data + first, a2dp_buffer, to_copy - first);
  }

  // Advance ONLY this consumer
  *rd = (r0 + to_copy) % cap;

  // Update shared fill based on slowest active reader
  a2dp_buffer_fill =
    ring_dist(rmin_active_locked(), a2dp_write_index, cap);

  portEXIT_CRITICAL(&a2dp_mux);
  return (int32_t)to_copy;
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
// Clear shared PCM ring buffer safely
// ======================================================
void AudioCore::clearPCM() {

  // 1️⃣ Clear indices atomically (fast, safe)
  portENTER_CRITICAL(&a2dp_mux);

  a2dp_write_index    = 0;
  a2dp_read_index     = 0;
  a2dp_read_index_i2s = 0;
  a2dp_buffer_fill    = 0;

  portEXIT_CRITICAL(&a2dp_mux);

  // 2️⃣ Validate buffer
  if (!a2dp_buffer) {
    Serial.println("[Audio] ❌ PCM buffer NULL — cannot clear");
    return;
  }

  if (a2dp_buffer) {
    memset(a2dp_buffer, 0, PCM_BUFFER_BYTES);
  }

  Serial.println("[Audio] 🧹 PCM buffer cleared");
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
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
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

bool AudioCore::getMP3Info(MP3StatusInfo& out)
{
    // No valid decoded info yet
    if (currentMP3Info.samplerate == 0) {
        return false;
    }

    // Atomic snapshot (volatile-safe)
    portENTER_CRITICAL(&a2dp_mux);

    switch(active_codec) {
      case 0:
        out.codec = "UNKNOWN";
        break;
      case 1:
        out.codec = "MP3";
        break;
      case 2:
        out.codec = "AAC";
        break;
    }

    out.samplerate = currentMP3Info.samplerate;
    out.kbps       = currentMP3Info.kbps;
    out.channels   = currentMP3Info.channels;
    out.layer      = currentMP3Info.layer;

    portEXIT_CRITICAL(&a2dp_mux);

    return true;
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
      3072,
      nullptr,
      DECODER_TASK_PRIORITY,
      nullptr,
      1);

  // I2S output task (core 1, lower prio)
  xTaskCreatePinnedToCore(
      i2sPlaybackTask,
      "I2S",
      2048,
      nullptr,
      I2S_TASK_PRIORITY,
      &i2STask,
      1);

      // if i2s output is off then stop it.
      if (!i2s_output) {
        vTaskSuspend(i2STask);
      }

     

  Serial.println("[Audio] ✅ AudioCore initialised (shared ring)");
  return true;
}

void AudioCore::StartI2S() {
  vTaskResume(i2STask);
}

void AudioCore::StopI2S() {
  vTaskSuspend(i2STask);
}

void AudioCore::i2sPlaybackTask(void* /*param*/) {

  constexpr size_t FRAMES_PER_CHUNK = I2S_DMA_BUF_LEN;
  constexpr size_t BYTES_PER_FRAME  = 4;   // L16 + R16
  constexpr size_t CHUNK_BYTES      = FRAMES_PER_CHUNK * BYTES_PER_FRAME;

  uint8_t tempBuf[CHUNK_BYTES];

  for (;;) {

    // Output gated
    if (!i2s_output) {
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    // Pull PCM (exact chunk)
    int32_t got = AudioCore::get_pcm_data(
                    PCM_CONSUMER_I2S,
                    tempBuf,
                    CHUNK_BYTES);

    if (got < (int32_t)CHUNK_BYTES) {
      // Not enough PCM yet → wait
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    size_t written = 0;

    // ✅ BLOCK until DMA has actually consumed time
    while (written < CHUNK_BYTES) {
      size_t w2 = 0;
      esp_err_t res = i2s_write(
          I2S_NUM_0,
          tempBuf + written,
          CHUNK_BYTES - written,
          &w2,
          portMAX_DELAY);   // <-- THIS IS THE FIX

      if (res != ESP_OK || w2 == 0) {
        // Hardware stalled → short backoff
        vTaskDelay(pdMS_TO_TICKS(1));
        break;
      }

      written += w2;
    }
  }
}

// ======================================================
// Decode task — UNCHANGED
// ======================================================
void AudioCore::decodeTask(void*) {

  const size_t HI_BYTES   = (PCM_BUFFER_BYTES * HI_PCT) / 100;
  const size_t LO_BYTES   = (PCM_BUFFER_BYTES * LO_PCT) / 100;
  const size_t PRIME_SLOTS = (NUM_BUFFERS * PRIME_PCT) / 100;
  const size_t MIN_A2DP_BYTES = (PCM_BUFFER_BYTES * MIN_A2DP_PCT) / 100;

  uint32_t last_session = 0xFFFFFFFF;
  bool priming = true;
  bool drained = false;
  bool a2dp_enabled_this_session = false;

  static UBaseType_t base_prio = uxTaskPriorityGet(NULL);
  static bool prio_boosted = false;

  for (;;) {

    const uint32_t cur_session = HttpStreamEngine::getPlaySession();

    // ==================================================
    // 🔒 SESSION FENCE
    // ==================================================
    if (cur_session != last_session) {

      if (active_codec == CODEC_MP3) mp3.end();
      if (active_codec == CODEC_AAC) aac.end();

      active_codec = CODEC_UNKNOWN;
      last_session = cur_session;

      priming = true;
      drained = false;

      decoder_auto_paused = false;
      set_a2dp_output(false);
      a2dp_enabled_this_session = false;

      vTaskDelay(pdMS_TO_TICKS(3));
      continue;
    }

    // OPT: cache these
    const bool stream_running = HttpStreamEngine::stream_running;
    const int  net_slots      = net_filled_slots();

    // ==================================================
    // DRAIN WHEN STREAM STOPS
    // ==================================================
    if (!stream_running) {

      if (net_slots > 0) {
        drained = false;
      }
      else {
        if (!drained) {
          if (active_codec == CODEC_MP3) mp3.write(nullptr, 0);
          else if (active_codec == CODEC_AAC) aac.write(nullptr, 0);
          drained = true;
        }

        priming = true;
        decoder_auto_paused = false;
        vTaskDelay(pdMS_TO_TICKS(5));
        continue;
      }
    }

    // ==================================================
    // NET PRIMING
    // ==================================================
    if (priming) {
      if (net_slots < (int)PRIME_SLOTS) {
        vTaskDelay(pdMS_TO_TICKS(2));
        continue;
      }
      priming = false;
      drained = false;
    }

    // ==================================================
    // PCM FILL
    // ==================================================
    portENTER_CRITICAL(&a2dp_mux);
    const size_t fill = a2dp_buffer_fill;
    portEXIT_CRITICAL(&a2dp_mux);

    const bool pcm_starving = (fill <= LO_BYTES);
    const bool net_starving = (net_slots <= 1);
    const bool starving    = pcm_starving || net_starving;

    // OPT: edge-triggered priority change
    if (starving && !prio_boosted) {
      vTaskPrioritySet(NULL, base_prio + 1);
      prio_boosted = true;
    }
    else if (!starving && prio_boosted) {
      vTaskPrioritySet(NULL, base_prio);
      prio_boosted = false;
    }

    // ==================================================
    // A2DP ENABLE / STICKY DISABLE
    // ==================================================
    if (!a2dp_enabled_this_session) {
      if (fill >= MIN_A2DP_BYTES) {
        a2dp_output = true;
        a2dp_enabled_this_session = true;
        Serial.println("[Decoder] Enabling A2DP output.");
      }
    }
    else {
      static uint32_t zero_since = 0;

      if (!stream_running && fill == 0) {
        if (!zero_since) zero_since = millis();
        if (millis() - zero_since > 600) {
          set_a2dp_output(false);
          a2dp_enabled_this_session = false;
        }
      }
      else {
        zero_since = 0;
      }
    }

    // ==================================================
    // AUTO PAUSE
    // ==================================================
    if (!decoder_auto_paused && fill >= HI_BYTES)
      decoder_auto_paused = true;
    else if (decoder_auto_paused && fill <= LO_BYTES)
      decoder_auto_paused = false;

    if (decoder_paused || decoder_auto_paused) {
      vTaskDelay(pdMS_TO_TICKS(4));
      continue;
    }

    // ==================================================
    // NET → DECODER
    // ==================================================
    int decode_loops = starving ? 3 : 1;

    while (decode_loops--) {

      const int slot = HttpStreamEngine::netRead;
      if (!HttpStreamEngine::netBufFilled[slot]) break;

      if (HttpStreamEngine::netSess[slot] != cur_session) {
        HttpStreamEngine::netBufFilled[slot] = false;
        HttpStreamEngine::netRead = (HttpStreamEngine::netRead + 1) % NUM_BUFFERS;
        continue;
      }

      uint8_t* ptr   = HttpStreamEngine::netBuffers[slot];
      uint16_t bytes = HttpStreamEngine::netSize[slot];
      uint16_t off   = HttpStreamEngine::netOffset[slot];
      uint8_t  tag   = HttpStreamEngine::netTag[slot];

      if (tag == SLOT_MP3 && active_codec != CODEC_MP3) {
        mp3.begin(); active_codec = feed_codec = CODEC_MP3;
      }
      else if (tag == SLOT_AAC && active_codec != CODEC_AAC) {
        aac.begin(); active_codec = feed_codec = CODEC_AAC;
      }

      if (off < bytes) { ptr += off; bytes -= off; }

      if (bytes) {
        if (active_codec == CODEC_MP3) mp3.write(ptr, bytes);
        else if (active_codec == CODEC_AAC) aac.write(ptr, bytes);
      }

      HttpStreamEngine::netBufFilled[slot] = false;
      HttpStreamEngine::netRead = (HttpStreamEngine::netRead + 1) % NUM_BUFFERS;
    }

    // ==================================================
    // COOPERATIVE THROTTLE (single exit)
    // ==================================================
    if (starving)               vTaskDelay(1);
    else if (decoder_auto_paused) vTaskDelay(pdMS_TO_TICKS(6));
    else                         vTaskDelay(pdMS_TO_TICKS(3));
  }
}