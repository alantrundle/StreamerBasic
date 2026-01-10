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
volatile bool HttpStreamEngine::g_id3_done = false;
volatile bool     HttpStreamEngine::stream_running = false;
volatile bool     HttpStreamEngine::g_force_next   = false;

volatile uint32_t HttpStreamEngine::g_lastNetWriteMs = 0;
volatile uint32_t HttpStreamEngine::g_httpBytesTick  = 0;

const char*   HttpStreamEngine::g_open_url  = nullptr;
volatile bool HttpStreamEngine::g_url_open  = false;
volatile bool HttpStreamEngine::g_isPlaying = false;

volatile bool HttpStreamEngine::stream_eof = false;


ID3v2Meta HttpStreamEngine::id3m{};

bool id3_done = false;

portMUX_TYPE net_mux = portMUX_INITIALIZER_UNLOCKED;

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

// WiFi Helpers
void HttpStreamEngine::wifi_quick_reconnect()
{
    Serial.println("[WIFI] Quick reconnect");

    size_t dma_free = heap_caps_get_largest_free_block(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    size_t int_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

  Serial.printf("[DEBUG] DMA largest free: %u bytes\n", dma_free);
  Serial.printf("[DEBUG] Internal free:   %u bytes\n", int_free);

    WiFi.disconnect(false);   // do NOT erase config
    delay(50);

    WiFi.reconnect();         // uses stored credentials

    while(!WiFi.isConnected()) {
      delay(100);
    }
}

bool HttpStreamEngine::wifi_check_and_recover()
{
    wl_status_t st = WiFi.status();

    if (st == WL_CONNECTED) {
        return true;
    }

    Serial.printf("[WIFI] ⛔ Not connected (status=%d), recovering…\n", st);
    wifi_quick_reconnect();
    return false;
}

uint32_t HttpStreamEngine::getPlaySession() {
  return g_play_session;
}

bool HttpStreamEngine::isID3Done() {
  return g_id3_done;
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
      3072,
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
  g_id3_done = false;
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
void HttpStreamEngine::httpFillTask(void*)
{
  HTTPClient http;
  WiFiClient sock;
  sock.setNoDelay(true);

  ID3v2Collector id3c{};
  ID3v2Meta&     id3 = HttpStreamEngine::id3m;

  for (;;) {

    const uint32_t my_session = g_play_session;

    // ================= IDLE =================
    if (!stream_running || !g_url_open) {
      vTaskDelay(pdMS_TO_TICKS(2));
      continue;
    }

    // ---------- clean close ----------
    http.end();
    sock.stop();
    vTaskDelay(pdMS_TO_TICKS(1));

    if (!http.begin(sock, g_open_url)) {
      vTaskDelay(pdMS_TO_TICKS(2));
      continue;
    }

    http.setReuse(false);
    http.setTimeout(800);
    http.setConnectTimeout(3000);

    const int code = http.GET();
    if (code != HTTP_CODE_OK && code != HTTP_CODE_PARTIAL_CONTENT) {
      http.end();
      sock.stop();
      vTaskDelay(pdMS_TO_TICKS(2));
      continue;
    }

    // ================= STREAM INIT =================
    int64_t content_len = http.getSize();
    int64_t bytes_seen  = 0;

    SlotCodec session_tag = SLOT_UNKNOWN;
    bool session_locked  = false;
    bool id3_done        = false;

    id3v2_reset_collector(&id3c);
    id3v2_reset_meta(&id3);

    Serial.printf("[HTTP] Connected: %s (len=%lld)\n",
                  g_open_url, (long long)content_len);

    // ================= STREAM =================
    while (stream_running && g_play_session == my_session) {

      // ---------- NET ring full ----------
      if (netBufFilled[netWrite]) {
        vTaskDelay(pdMS_TO_TICKS(1));
        continue;
      }

      const bool sock_ok = sock.connected();
      const int  avail   = sock.available();

      if (avail <= 0) {
        if (!sock_ok) break;
        vTaskDelay(pdMS_TO_TICKS(2));
        continue;
      }

      int toRead = min(avail, MAX_CHUNK_SIZE);
      if (content_len > 0) {
        int64_t remain = content_len - bytes_seen;
        if (remain <= 0) break;
        if (toRead > remain) toRead = remain;
      }

      uint8_t* dst = netBuffers[netWrite];
      const int readn = sock.read(dst, toRead);
      if (readn <= 0)
        continue;

      bytes_seen += readn;

      uint8_t* cur = dst;
      size_t   len = readn;

      // ---------- ID3 peel (DESTRUCTIVE, SAFE) ----------
      if (!id3_done && len) {

        id3v2_try_begin(cur, len, bytes_seen - len,
                        MAX_CHUNK_SIZE, &id3c);

        const size_t taken =
          id3v2_consume(cur, len, &id3c, &id3);

        cur += taken;
        len -= taken;

        if (!id3c.collecting && !id3_done) {
          id3_done   = true;
          g_id3_done = true;
          Serial.println("[HTTP] ✅ ID3 phase complete");
        }

        if (!len)
          continue;
      }

      // ---------- HEADER DETECTION (ONE-SHOT, NON-DESTRUCTIVE) ----------
      if (!session_locked && id3_done && len) {

        audetect::DetectResult dr{};
        if (audetect::detect_audio_format_strict(cur, len, &dr)) {

          if (dr.format == audetect::AF_MP3 ||
              dr.format == audetect::AF_AAC_ADTS) {

            session_tag =
              (dr.format == audetect::AF_MP3)
                ? SLOT_MP3
                : SLOT_AAC;

            int off = dr.offset;
            if (off < 0) off = 0;
            if (off > (int)len) off = len;

            cur += off;
            len -= off;

            session_locked = true;

            Serial.printf(
              "[HTTP] 🔒 Codec locked (%s), offset=%d\n",
              (session_tag == SLOT_MP3 ? "MP3" : "AAC"), off);

            if (!len)
              continue;
          }
        }

        // ❗ IMPORTANT:
        // If detection fails, DO NOT SKIP BYTES.
        // Treat data as audio and continue.
      }

      // ---------- publish NET slot (ATOMIC) ----------
      if (session_locked && len) {

        portENTER_CRITICAL(&net_mux);

        netSize[netWrite]       = len;
        netTag[netWrite]        = session_tag;
        netOffset[netWrite]     = 0;
        netSess[netWrite]       = my_session;
        netBufFilled[netWrite]  = true;

        netWrite = (netWrite + 1) % NUM_BUFFERS;

        portEXIT_CRITICAL(&net_mux);

        g_lastNetWriteMs = millis();
        g_httpBytesTick  += len;
      }

      if (content_len > 0 && bytes_seen >= content_len)
        break;
    }

    // ================= TEARDOWN =================
    http.end();
    sock.stop();

    stream_running = false;
    stream_eof     = true;

    Serial.println("[HTTP] ⏳ Network EOF, draining PCM...");

    // ---------- PCM drain wait ----------
    uint32_t last_change = millis();
    int last_pcm = AudioCore::pcm_buffer_percent();

    while (g_play_session == my_session) {

      int pcm = AudioCore::pcm_buffer_percent();

      if (pcm <= 0) {
        Serial.println("[HTTP] ✅ PCM drained");
        break;
      }

      if (pcm != last_pcm) {
        last_pcm = pcm;
        last_change = millis();
      }

      if (millis() - last_change > PCM_STALL_TIMEOUT_MS) {
        Serial.println("[HTTP] ⚠ PCM drain stalled, forcing end");
        break;
      }

      vTaskDelay(pdMS_TO_TICKS(2));
    }

    // ---------- final ----------
    if (g_play_session == my_session) {

      g_isPlaying = false;
      g_url_open  = false;
      stream_eof  = false;

      Serial.println("[HTTP] ✅ Playback finished");
    }

    vTaskDelay(pdMS_TO_TICKS(10));
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