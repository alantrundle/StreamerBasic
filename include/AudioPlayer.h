#pragma once

#include <stdint.h>

// ✅ REQUIRED for esp_a2d_audio_state_t
#include "esp_a2dp_api.h"

// (optional but recommended)
#include "esp_avrc_api.h"

#include "Config.h"
#include "AudioCore.h"
#include "HttpStreamEngine.h"

// ------------------------------------------------
// A2DP callbacks used by AudioPlayer
// ------------------------------------------------

int32_t pcm_data_callback(uint8_t* data, int32_t len);

// ✅ NOW compiles correctly
void onA2DPAudioState(esp_a2d_audio_state_t state, void*);
void onA2DPConnectionState(esp_a2d_connection_state_t state, void* user);
//void onA2DPConnectState(esp_a2d_connection_state_t state, void*);

// ------------------------------------------------
// Audio control API
// ------------------------------------------------

void AudioPlayer_Play();
void AudioPlayer_Pause();
void AudioPlayer_Stop();
void AudioPlayer_Next();
void AudioPlayer_Prev();
void AudioPlayer_Loop();
