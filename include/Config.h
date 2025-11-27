#pragma once
#include <Arduino.h>

// --- Wi-Fi + Stream config ---
static const char* WIFI_SSID = "WyseNet";
static const char* WIFI_PASS = "cdf45424e4";

// ------------------------------
// NET / HTTP buffering
// ------------------------------
#define MAX_CHUNK_SIZE 1024
#define NUM_BUFFERS    512

// ------------------------------
// PCM buffering
// ------------------------------
#define PCM_BUFFER_SIZE_KB 256
#define PCM_BUFFER_BYTES   (1024 * PCM_BUFFER_SIZE_KB)
#define A2DP_BUFFER_SIZE PCM_BUFFER_BYTES


// ------------------------------
// I2S pins
// ------------------------------ 
#define I2S_BCK   14
#define I2S_WS    12
#define I2S_DATA  13

#define I2S_DMA_BUF_COUNT 12
#define I2S_DMA_BUF_LEN 256

// ------------------------------
// Decode cadence
// ------------------------------
constexpr int BUDGET_FAST = 3;
constexpr int BUDGET_NORM = 2;
constexpr int BUDGET_SLOW = 1;

