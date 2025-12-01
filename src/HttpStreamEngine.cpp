#include "HttpStreamEngine.h"
#include "AudioCore.h"

// =======================================================
// Static storage
// =======================================================

EXT_RAM_ATTR uint8_t* HttpStreamEngine::net_pool = nullptr;
EXT_RAM_ATTR uint8_t* HttpStreamEngine::netBuffers[NUM_BUFFERS] = { nullptr };

volatile bool     HttpStreamEngine::netBufFilled[NUM_BUFFERS] = { false };
volatile uint16_t HttpStreamEngine::netSize[NUM_BUFFERS]      = { 0 };
volatile uint8_t  HttpStreamEngine::netTag[NUM_BUFFERS]       = { 0 };
volatile uint16_t HttpStreamEngine::netOffset[NUM_BUFFERS]    = { 0 };
volatile uint32_t HttpStreamEngine::netSess[NUM_BUFFERS]      = { 0 };

volatile int HttpStreamEngine::netWrite = 0;
volatile int HttpStreamEngine::netRead  = 0;

volatile uint32_t HttpStreamEngine::g_play_session = 0;
volatile bool     HttpStreamEngine::stream_running = false;
volatile bool     HttpStreamEngine::g_force_next   = false;

volatile uint32_t HttpStreamEngine::g_lastNetWriteMs = 0;
volatile uint32_t HttpStreamEngine::g_httpBytesTick  = 0;

const char*   HttpStreamEngine::g_open_url  = nullptr;
volatile bool HttpStreamEngine::g_url_open  = false;
volatile bool HttpStreamEngine::g_isPlaying = false;

volatile bool HttpStreamEngine::stream_eof = false;

ID3v2Meta HttpStreamEngine::id3m{};


TaskHandle_t HttpStreamEngine::httpTaskHandle = nullptr;

// =======================================================
// Helpers
// =======================================================

void HttpStreamEngine::net_ring_clear() {
  for (int i = 0; i < NUM_BUFFERS; ++i) {
    netBufFilled[i] = false;
    netSize[i]      = 0;
    netTag[i]       = SLOT_UNKNOWN;
    netOffset[i]    = 0;
    netSess[i]      = 0;
  }
  netWrite = 0;
  netRead  = 0;
}

bool HttpStreamEngine::isPlaying() {
  return g_isPlaying;
}

bool HttpStreamEngine::isAlive() {
    return g_isPlaying &&
           stream_running &&
           g_url_open &&
           !stream_eof;
}

bool HttpStreamEngine::getID3(ID3v2Meta& out) {

  if (!id3m.header_found)
    return false;

  out = id3m;
  return true;
}


// =======================================================
// Lifecycle
// =======================================================

void HttpStreamEngine::begin() {

  if (httpTaskHandle)
    return;

  net_pool = (uint8_t*)heap_caps_malloc(
      NUM_BUFFERS * MAX_CHUNK_SIZE,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!net_pool) {
    Serial.println("[HTTP] ❌ Failed to allocate net_pool");
    abort();
  }

  for (int i = 0; i < NUM_BUFFERS; ++i)
    netBuffers[i] = net_pool + (i * MAX_CHUNK_SIZE);

  net_ring_clear();

  xTaskCreatePinnedToCore(
      httpFillTask,
      "HTTPFill",
      8192,
      nullptr,
      HTTP_TASK_PRIORITY,
      &httpTaskHandle,
      1);

  Serial.println("[HTTP] Engine initialized");
}

void HttpStreamEngine::open(const char* url) {

  if (!url || g_isPlaying)
    return;

  g_open_url = url;
  g_url_open = true;

  g_play_session++;
  stream_running = false;

  net_ring_clear();
  AudioCore::clearPCM();

  Serial.printf("[HTTP] Opened URL: %s\n", g_open_url);
}

void HttpStreamEngine::play() {

  if (!g_url_open || g_isPlaying)
    return;

  g_force_next   = false;
  stream_running = true;
  g_isPlaying    = true;

  if (httpTaskHandle)
    vTaskResume(httpTaskHandle);

  Serial.println("[HTTP] ▶ Play");
}

void HttpStreamEngine::close() {

  // Graceful stop request
  stream_running = false;

  // Invalidate current session
  g_play_session++;

  // Mark URL inactive
  g_url_open  = false;

  // Mark playback stopped
  g_isPlaying = false;

  Serial.println("[HTTP] ⏹ Close (graceful)");
}


void HttpStreamEngine::stop() {

  // Hard stop: abort everything immediately
  stream_running = false;
  g_url_open     = false;
  g_isPlaying    = false;

  // Kill any in-flight session
  g_play_session++;

  // Drop all buffered data immediately
  net_ring_clear();

  Serial.println("[HTTP] ⏹ Stop (hard)");
}

// =======================================================
// HTTP Producer Task (PATCHED TO MATCH ORIGINAL MAIN.CPP)
// =======================================================
void HttpStreamEngine::httpFillTask(void*) {

  HTTPClient http;
  WiFiClient sock;
  sock.setNoDelay(true);

  ID3v2Collector id3c{};
  ID3v2Meta&     id3 = HttpStreamEngine::id3m;

  uint8_t tail6[6] = {0};
  bool    tail_valid = false;

  int64_t  content_len     = -1;
  int64_t  bytes_seen      = 0;
  int64_t  bytes_committed = 0;


  uint32_t stall_deadline = 0;
  int      stall_retries  = 0;

  for (;;) {

    // ---------------- IDLE ----------------
    if (!stream_running || !g_url_open) {
      vTaskDelay(30); // idle or not streaming
      continue;
    }

    const uint32_t my_session = g_play_session;

    http.end();
    sock.stop();

    if (!http.begin(sock, g_open_url)) {
      vTaskDelay(200); // HTTP begin failure
      continue;
    }

    http.setReuse(false);            // ✅ IMPORTANT
    http.setTimeout(500);
    http.setConnectTimeout(4000);

    int code = http.GET();
    if (code != HTTP_CODE_OK && code != HTTP_CODE_PARTIAL_CONTENT) {
      Serial.printf("[HTTP] ❌ GET failed: %d\n", code);
      http.end();
      sock.stop();
      vTaskDelay(500); // HTTP GET failure
      continue;
    }

    // -------- init per stream --------
    content_len     = http.getSize();
    bytes_seen      = 0;
    bytes_committed = 0;
    stall_deadline  = millis() + STALL_RETRY_TIMEOUT_MS;
    stall_retries   = 0;

    stream_eof = false;

    id3v2_reset_collector(&id3c);
    id3v2_reset_meta(&id3m);

    SlotCodec session_tag = SLOT_UNKNOWN;
    bool session_locked  = false;
    tail_valid = false;

    Serial.printf("[HTTP] Connected: %s (len=%lld)\n",
                  g_open_url, (long long)content_len);

    // ================= STREAM =================
    while (stream_running && g_play_session == my_session) {

      if (netBufFilled[netWrite]) {
        vTaskDelay(net_fill_percent() > 70 ? 20 : 5); // net fill. 20 if over 70%, and 1 when under (throttle)
        continue;
      }

      int avail = sock.available();

      // ✅ ROBUST stall handling
      if (avail <= 0) {

        if (!sock.connected()) {
          Serial.println("[HTTP] ❌ Socket disconnected");
          break;
        }

        if (millis() > stall_deadline) {

          stall_retries++;

          Serial.printf("[HTTP] ⚠ Stall retry %d/%d\n",
                        stall_retries, STALL_MAX_RETRIES);

          if (stall_retries >= STALL_MAX_RETRIES) {
            Serial.println("[HTTP] ❌ Stall limit reached");
            break;
          }

          stall_deadline = millis() + STALL_RETRY_TIMEOUT_MS;
        }

        vTaskDelay(100); // socket has no data (normal TCP wait)
        continue;
      }

      int toRead = min(avail, MAX_CHUNK_SIZE);

      if (content_len > 0) {
        int64_t remain = content_len - bytes_seen;
        if (remain <= 0) break;
        if (toRead > (int)remain) toRead = (int)remain;
      }

      uint8_t* dst = netBuffers[netWrite];

      int readn = sock.read(dst, toRead);
      if (readn <= 0) {
        vTaskDelay(1);
        continue;
      }

      // ✅ GOOD DATA → reset stall state
      bytes_seen += readn;
      stall_deadline = millis() + STALL_RETRY_TIMEOUT_MS;
      stall_retries  = 0;

      uint8_t* cur = dst;
      size_t   len = readn;

      // ---------- ID3 peel ----------
      if (!session_locked && !id3.header_found) {

        id3v2_try_begin(cur, len, bytes_seen - len, MAX_CHUNK_SIZE, &id3c);

        size_t taken = id3v2_consume(cur, len, &id3c, &id3);
        cur += taken;
        len -= taken;
      }

      SlotCodec tag = session_tag;
      uint16_t  offset = 0;

      if (!session_locked && len > 0) {

        audetect::DetectResult dr{};
        uint8_t view[MAX_CHUNK_SIZE + 6];
        int vlen;

        if (tail_valid) {
          memcpy(view, tail6, 6);
          memcpy(view + 6, cur, len);
          vlen = len + 6;
        } else {
          memcpy(view, cur, len);
          vlen = len;
        }

        if (audetect::detect_audio_format_strict(view, vlen, &dr)) {

          if (dr.format == audetect::AF_MP3)
            tag = SLOT_MP3;
          else if (dr.format == audetect::AF_AAC_ADTS)
            tag = SLOT_AAC;

          int off = dr.offset - (tail_valid ? 6 : 0);
          if (off < 0) off = 0;
          if (off > (int)len) off = len;

          offset = (uint16_t)off;
          session_tag    = tag;
          session_locked = true;
          tail_valid = false;
        }
        else if (len >= 6) {
          memcpy(tail6, cur + len - 6, 6);
          tail_valid = true;
        }
      }

      // -------- publish NET slot --------
      netSize[netWrite]      = len;
      netTag[netWrite]       = tag;
      netOffset[netWrite]    = offset;
      netSess[netWrite]     = my_session;
      netBufFilled[netWrite] = true;
      netWrite = (netWrite + 1) % NUM_BUFFERS;

      bytes_committed += len;
      g_lastNetWriteMs = millis();
      g_httpBytesTick  += len;

      if (content_len > 0 && bytes_seen >= content_len) {
        Serial.println("[HTTP] ✅ EOF (Content-Length)");
        break;
      }
    }

    // ================= HTTP ENDED =================
    http.end();
    sock.stop();

    if (g_play_session == my_session) {

      stream_running = false;
      stream_eof     = true;

      Serial.printf(
        "[HTTP] Final totals: seen=%lld committed=%lld content_len=%lld\n",
        (long long)bytes_seen,
        (long long)bytes_committed,
        (long long)content_len
      );

      Serial.println("[HTTP] ⏹ HTTP EOF — draining decoder");
    }

    uint32_t last_change_ms = millis();
    int      last_pcm_pct   = AudioCore::pcm_buffer_percent();

    while (AudioCore::pcm_buffer_percent() > 0) {

    // ✅ Do NOT interfere with pause
    if (AudioCore::decoder_paused) {
      vTaskDelay(10);
      continue;
    }

    int cur_pct = AudioCore::pcm_buffer_percent();

    // ✅ Detect forward progress
    if (cur_pct != last_pcm_pct) {
      last_pcm_pct   = cur_pct;
      last_change_ms = millis();
    }

    // ❌ PCM stuck → exit drain safely
    if (millis() - last_change_ms > PCM_STALL_TIMEOUT_MS) {
      Serial.printf("[HTTP] ⚠ PCM drain stalled at %d%% — forcing end\n", cur_pct);
      break;
    }

  }

  if (stream_eof && g_play_session == my_session) {

    g_isPlaying = false;
    g_url_open  = false;
    stream_eof  = false;

    Serial.println("[HTTP] ✅ Decoder drained — playback finished");
  }

  // prevents CPU starvation, but throttle if necesarry
  vTaskDelay(net_fill_percent() > 70 ? 20 : 5); // net fill. 20 if over 70%, and 1 when under (throttle)

  }
}

int HttpStreamEngine::net_fill_percent() {

  int filled = 0;

  // No mutex needed: slots are single-writer (HTTP)
  // and single-reader (decode), bool write is atomic
  for (int i = 0; i < NUM_BUFFERS; ++i) {
    if (netBufFilled[i]) filled++;
  }

  return (filled * 100) / NUM_BUFFERS;
}