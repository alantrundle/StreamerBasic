 #pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "Config.h"
#include "idv3Parser.h"
#include "headerDetection.h"

// =======================================================
// Slot codec tagging (EXACT)
// =======================================================
enum SlotCodec : uint8_t {
  SLOT_UNKNOWN = 0,
  SLOT_MP3     = 1,
  SLOT_AAC     = 2
};

class HttpStreamEngine {
public:
  // Lifecycle
  static void begin();
  static void open(const char* url);
  static void play();
  static void close();
  static void stop();

  static void wifi_quick_reconnect();
  static bool wifi_check_and_recover();

  static void net_ring_clear();

  static bool isPlaying();
  static bool isAlive();

  static uint32_t getPlaySession();
  static bool isID3Done();


  // ================= NET ring =================
  static EXT_RAM_ATTR uint8_t* net_pool;
  static EXT_RAM_ATTR uint8_t* netBuffers[NUM_BUFFERS];

  static volatile bool     netBufFilled[NUM_BUFFERS];
  static volatile uint16_t netSize[NUM_BUFFERS];
  static volatile uint8_t  netTag[NUM_BUFFERS];
  static volatile uint16_t netOffset[NUM_BUFFERS];
  static volatile uint32_t netSess[NUM_BUFFERS];

  static volatile bool stream_eof;

  static volatile int netWrite;
  static volatile int netRead;

  // ================= Stream state =================
  static volatile uint32_t g_play_session;
  static volatile bool     stream_running;
  static volatile bool     g_force_next;

  static int net_fill_percent();

  static ID3v2Meta id3m;
  static bool getID3(ID3v2Meta& out);

private:
  // Task
  static TaskHandle_t httpTaskHandle;
  static void httpFillTask(void* arg);

  // URL / state
  static const char*   g_open_url;
  static volatile bool g_url_open;
  static volatile bool g_isPlaying;

  // Stats / debug
  static volatile uint32_t g_lastNetWriteMs;
  static volatile uint32_t g_httpBytesTick;

  // id3
  static volatile bool g_id3_done;

};
