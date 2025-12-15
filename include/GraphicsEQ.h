#pragma once
#include <stdint.h>
#include <cstddef>  

void eq_set_samplerate(int samplerate);
void eq_process_block(int16_t* pcm, size_t samples);
