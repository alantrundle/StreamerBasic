#pragma once
#include <Arduino.h>

#include "gfxDriver.h"

#include <lvgl.h>
#include "ui/ui.h"
#include "ui/bindings.h"

#include "HttpStreamEngine.h"
#include "AudioCore.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "A2DPCore.h"


static uint32_t last_session = 0;

void lvgl_init();
void macToStr(const uint8_t mac[6], char* out, size_t out_len);
static void ui_update_timer_cb(lv_timer_t *t);