#pragma once
#include <Arduino.h>



void lvgl_init();
void macToStr(const uint8_t mac[6], char* out, size_t out_len);