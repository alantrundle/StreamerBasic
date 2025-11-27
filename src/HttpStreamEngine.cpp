#include "HttpStreamEngine.h"

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
      1,
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

void HttpStreamEngine::stop() {

  stream_running = false;
  g_isPlaying    = false;
  net_ring_clear();

  Serial.println("[HTTP] ⏹ Stop");
}

// =======================================================
// HTTP Producer Task (PATCHED TO MATCH ORIGINAL MAIN.CPP)
// =======================================================

void HttpStreamEngine::httpFillTask(void*) {

  HTTPClient http;
  WiFiClient sock;
  sock.setNoDelay(true);

  ID3v2Collector id3c{};
  ID3v2Meta      id3m{};

  uint8_t tail6[6] = {0};
  bool tail_valid  = false;

  // ---------- DEBUG ----------
  uint32_t dbg_pkts = 0;
  uint32_t dbg_bytes = 0;
  uint32_t dbg_last = 0;
  // --------------------------

  for (;;) {

    // 🔑 DO NOT SELF-SUSPEND (idle instead)
    if (!stream_running || !g_open_url) {
      vTaskDelay(10);
      continue;
    }

    const uint32_t my_session = g_play_session;

    http.end();
    sock.stop();

    if (!http.begin(sock, g_open_url)) {
      vTaskDelay(100);
      continue;
    }

    http.setReuse(true);
    http.setTimeout(2500);
    http.setConnectTimeout(4000);

    int code = http.GET();
    if (code != HTTP_CODE_OK && code != HTTP_CODE_PARTIAL_CONTENT) {
      http.end();
      vTaskDelay(200);
      continue;
    }

    id3v2_reset_collector(&id3c);
    id3v2_reset_meta(&id3m);

    SlotCodec session_tag = SLOT_UNKNOWN;
    bool session_locked = false;
    tail_valid = false;

    int64_t bytes_seen = 0;

    Serial.printf("[HTTP] Connected: %s\n", g_open_url);

    while (stream_running && g_play_session == my_session) {

      if (netBufFilled[netWrite]) {
        vTaskDelay(1);
        continue;
      }

      uint8_t* dst = netBuffers[netWrite];
      int readn = sock.read(dst, MAX_CHUNK_SIZE);

      // 🔑 IMPORTANT: zero read ≠ EOF
      if (readn <= 0) {
        vTaskDelay(2);
        continue;
      }

      size_t wrote = (size_t)readn;
      bytes_seen += wrote;

      uint8_t* cur = dst;
      size_t len   = wrote;

      if (!session_locked && !id3m.header_found) {
        id3v2_try_begin(cur, len, bytes_seen - len, MAX_CHUNK_SIZE, &id3c);
        size_t taken = id3v2_consume(cur, len, &id3c, &id3m);
        cur += taken;
        len -= taken;
      }

      SlotCodec tag = session_tag;
      uint16_t offset = 0;

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
          if (dr.format == audetect::AF_MP3) tag = SLOT_MP3;
          else if (dr.format == audetect::AF_AAC_ADTS) tag = SLOT_AAC;

          offset = dr.offset - (tail_valid ? 6 : 0);
          if ((int)offset < 0 || offset >= len) offset = 0;

          session_tag = tag;
          session_locked = true;
          tail_valid = false;
        } else if (len >= 6) {
          memcpy(tail6, cur + len - 6, 6);
          tail_valid = true;
        }
      }

      netSize[netWrite]      = len;
      netTag[netWrite]       = tag;
      netOffset[netWrite]    = offset;
      netSess[netWrite]      = my_session;
      netBufFilled[netWrite] = true;

      netWrite = (netWrite + 1) % NUM_BUFFERS;

      g_lastNetWriteMs = millis();
      g_httpBytesTick  += len;

      // ---------- DEBUG ----------
      dbg_pkts++;
      dbg_bytes += len;
      uint32_t now = millis();
      if (now - dbg_last > 1000) {
        dbg_last = now;
      }
      // --------------------------
    }

    http.end();
    sock.stop();

    // 🔑 DO NOT FORCE STOP HERE — decoder drains NET
    vTaskDelay(1);
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

