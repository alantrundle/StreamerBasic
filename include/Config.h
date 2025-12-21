#pragma once
#include <Arduino.h>

// --- Wi-Fi + Stream config ---
static const char* WIFI_SSID = "WyseNet";
static const char* WIFI_PASS = "cdf45424e4";

// ------------------------------
// NET / HTTP buffering
// ------------------------------
#define MAX_CHUNK_SIZE 1024
#define NUM_BUFFERS   512

// ✅ NEW robust stall handling
constexpr uint32_t STALL_RETRY_TIMEOUT_MS = 800;
constexpr int      STALL_MAX_RETRIES      = 10;

// ------------------------------
// PCM buffering
// ------------------------------
#define PCM_BUFFER_SIZE_KB 1024
#define PCM_BUFFER_BYTES   (1024 * PCM_BUFFER_SIZE_KB)
#define A2DP_BUFFER_SIZE PCM_BUFFER_BYTES

// ------------------------------
// I2S pins & Config
// ------------------------------ 
#define I2S_BCK   14
#define I2S_WS    12
#define I2S_DATA  13

#define I2S_DMA_BUF_COUNT 8
#define I2S_DMA_BUF_LEN 256

// ------------------------------
// Decode cadence
// ------------------------------
constexpr int HI_PCT    = 90;
constexpr int LO_PCT    = 80;
constexpr int PRIME_PCT = 30;
constexpr int MIN_A2DP_PCT = 10;   // wait until PCN > 10%

 constexpr uint32_t PCM_STALL_TIMEOUT_MS = 4000; // detect end of track

// xTask Priorities
constexpr int DECODER_TASK_PRIORITY = 4;
constexpr int HTTP_TASK_PRIORITY    = 8;
constexpr int LVGL_TASK_PRIORITY    = 1;
constexpr int I2S_TASK_PRIORITY     = 2;


constexpr uint32_t UI_UPDATE_PERIOD_MS = 80;

// Album Art - IDv3
#define TJPGD_WORKSPACE_SIZE 3100
#define THUMB_W 120 // output size
#define THUMB_H 120 // output size