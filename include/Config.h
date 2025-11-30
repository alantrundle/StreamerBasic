#pragma once
#include <Arduino.h>

// --- Wi-Fi + Stream config ---
static const char* WIFI_SSID = "WyseNet";
static const char* WIFI_PASS = "cdf45424e4";

// ------------------------------
// NET / HTTP buffering
// ------------------------------
#define MAX_CHUNK_SIZE 2048
#define NUM_BUFFERS   128

// ✅ NEW robust stall handling
constexpr uint32_t STALL_RETRY_TIMEOUT_MS = 800;
constexpr int      STALL_MAX_RETRIES      = 10;

// ------------------------------
// PCM buffering
// ------------------------------
#define PCM_BUFFER_SIZE_KB 512
#define PCM_BUFFER_BYTES   (1024 * PCM_BUFFER_SIZE_KB)
#define A2DP_BUFFER_SIZE PCM_BUFFER_BYTES

// ------------------------------
// I2S pins & Config
// ------------------------------ 
#define I2S_BCK   14
#define I2S_WS    12
#define I2S_DATA  13

#define I2S_DMA_BUF_COUNT 12
#define I2S_DMA_BUF_LEN 256

// ------------------------------
// Decode cadence
// ------------------------------
constexpr int HI_PCT    = 90;
constexpr int LO_PCT    = 60;
constexpr int PRIME_PCT = 75;

// xTask Priorities
constexpr int DECODER_TASK_PRIORITY = 2;
constexpr int HTTP_TASK_PRIORITY    = 1;
constexpr int LVGL_TASK_PRIORITY    = 1;
constexpr int I2S_TASK_PRIORITY     = 2;