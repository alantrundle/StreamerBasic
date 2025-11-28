// TFT inludes
#include "LVGLCore.h"
#include "gfxDriver.h"

#include "freertos/FreeRTOS.h"  // needed for ESP Arduino < 2.0


#include <lvgl.h>
#include "gfxDriver.h"   // provides: extern LGFX_ST7796 display;
#include "ui/ui.h"        // your UI (bars + keyboard)
#include "ui/bindings.h"

#include "HttpStreamEngine.h"
#include "AudioCore.h"


// LVGL display + draw buffers
static lv_display_t *lv_disp = nullptr;
static lv_color_t *lvbuf1 = nullptr;
static lv_color_t *lvbuf2 = nullptr;
// End LVGL

static void my_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static void my_touch_read(lv_indev_t *indev, lv_indev_data_t *data);
void lvgl_start_task();



// Millis as LVGL tick source
static uint32_t my_tick(void) {
  return (uint32_t)millis();
}

void lvgl_init() {

  // --- Display init ---
  display.init();
  display.setRotation(PANEL_ROTATION);
  display.setBrightness(200);
  display.fillScreen(0x0000);

  // --- LVGL init ---
  lv_init();
  lv_tick_set_cb(my_tick);

  // Display driver for LVGL
  lv_disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
  lv_display_set_flush_cb(lv_disp, my_flush);

  // Allocate small INTERNAL DMA buffers (partial rendering)
  int lines = 20;                                   // tune (20..40 works w
  size_t bytes = (size_t)TFT_HOR_RES * lines * sizeof(lv_color_t);
  lvbuf1 = (lv_color_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  lvbuf2 = (lv_color_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
 

  if (!lvbuf1 || !lvbuf2) {
    // graceful fallback → single buffer
    if (lvbuf2) { heap_caps_free(lvbuf2); lvbuf2 = nullptr; }
    if (!lvbuf1) {
      lines = 20;
      bytes = (size_t)TFT_HOR_RES * lines * sizeof(lv_color_t);
      lvbuf1 = (lv_color_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    lv_display_set_buffers(lv_disp, lvbuf1, nullptr, bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
  } else {
    lv_display_set_buffers(lv_disp, lvbuf1, lvbuf2, bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
  }

  // Touch input device
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touch_read);
  lv_indev_set_display(indev, lv_disp);

  // --- Build your UI (bars + keyboard, maps in FLASH) ---
 
  ui_init();
  lvgl_start_task();

  Serial.println("[LVGL] TFT setup done");
}

// Call once after lv_init() + driver registration
void lvgl_start_task() {
  xTaskCreatePinnedToCore(
    [](void*) {

      TickType_t last = xTaskGetTickCount();
      const TickType_t period = pdMS_TO_TICKS(20); // 60Hz

      for (;;) {
        const int net_pct = HttpStreamEngine::net_fill_percent();
        const int pcm_pct  = AudioCore::pcm_buffer_percent();

        ID3v2Meta meta;

        lv_timer_handler();
        
        ui_update_stats_bars(net_pct , pcm_pct);
        //ui_update_stats_outputs(i2s_output, a2dp_connected, "none");
        //ui_update_stats_decoder(codec_name_from_enum(feed_codec), currentMP3Info.samplerate, currentMP3Info.channels, currentMP3Info.kbps);
        //ui_update_stats_outputs(i2s_output, a2dp_connected, a2dp_connected_name);
        //ui_update_stats_wifi(WiFi.status(), WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());


        if (HttpStreamEngine::getID3(meta)) {
          ui_update_player_id3(true, meta.artist, meta.title, meta.album, (int)meta.track);
        }
        else {
          ui_update_player_id3(false, "-", "-", "-", -1);
        }

        ui_tick();

        vTaskDelayUntil(&last, period);  
      }
    },
    "LVGL", 6144, nullptr, 1, nullptr, 1
  );
}



// LVGL -> panel flush
static void my_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  (void)disp;
  const uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
  const uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);

  display.startWrite();
  display.setAddrWindow(area->x1, area->y1, w, h);
  display.writePixels((lgfx::rgb565_t*)px_map, w * h);  // px_map is RGB565
  display.endWrite();

  lv_display_flush_ready(disp);
}

// LVGL touch read -> use driver’s readTouch()
static void my_touch_read(lv_indev_t *indev, lv_indev_data_t *data) {
  (void)indev;
  if (!data) return;

  uint16_t x=0, y=0; bool pressed=false;
  bool ok = display.readTouch(x, y, pressed);
  data->state   = (ok && pressed) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  if (ok && pressed) {
    data->point.x = (lv_coord_t)x;
    data->point.y = (lv_coord_t)y;
  }
}