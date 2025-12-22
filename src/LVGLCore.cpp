// TFT includes
#include "LVGLCore.h"
#include "WiFi.h"

extern A2DPCore a2dp;

// Helpers
void macToStr(const uint8_t mac[6], char *out, size_t out_len)
{
  if (!out || out_len < 18)
  { // "AA:BB:CC:DD:EE:FF" + '\0'
    if (out && out_len > 0)
      out[0] = '\0';
    return;
  }

  snprintf(out, out_len,
           "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

// -------------------------------------------------
// LVGL display + buffers
// -------------------------------------------------

static lv_display_t *lv_disp = nullptr;
static lv_color_t *lvbuf1 = nullptr;
static lv_color_t *lvbuf2 = nullptr;

static void my_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static void my_touch_read(lv_indev_t *indev, lv_indev_data_t *data);

// -------------------------------------------------
// LVGL tick source
// -------------------------------------------------

static uint32_t my_tick(void)
{
  return (uint32_t)millis();
}

// -------------------------------------------------
// Init
// -------------------------------------------------

void lvgl_init()
{

  // --- Panel ---
  display.init();
  display.setRotation(PANEL_ROTATION);
  display.setBrightness(180);
  display.fillScreen(0x0000);

  // --- LVGL ---
  lv_init();
  lv_tick_set_cb(my_tick);

  lv_disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
  lv_display_set_flush_cb(lv_disp, my_flush);

  // ✅ Larger buffers = fewer flushes
  const int lines = 4; // tuned: reduces stripe effect heavily
  const size_t bytes = (size_t)TFT_HOR_RES * lines * sizeof(lv_color_t);

  // lvbuf1 = (lv_color_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  // lvbuf2 = (lv_color_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  lvbuf1 = (lv_color_t *)heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  // lvbuf2 = (lv_color_t*)heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  lvbuf2 = nullptr;

  if (lvbuf1 && lvbuf2)
  {
    lv_display_set_buffers(
        lv_disp,
        lvbuf1,
        lvbuf2,
        bytes,
        LV_DISPLAY_RENDER_MODE_PARTIAL);
  }
  else
  {
    // fallback single buffer
    lv_display_set_buffers(
        lv_disp,
        lvbuf1,
        nullptr,
        bytes,
        LV_DISPLAY_RENDER_MODE_PARTIAL);
  }

  // --- Touch ---
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touch_read);
  lv_indev_set_display(indev, lv_disp);

  // --- UI ---
  ui_init();

  lv_timer_create(ui_update_timer_cb, UI_UPDATE_PERIOD_MS, nullptr);

  xTaskCreatePinnedToCore([](void *)
                          {

    for (;;) {

        lv_timer_handler();

        // 3️⃣ Delay sets LVGL cadence
        vTaskDelay(pdMS_TO_TICKS(8));
    } }, "LVGL", 6144, nullptr, LVGL_TASK_PRIORITY, nullptr, 1);

  Serial.println("[LVGL] TFT setup done");
}

// -------------------------------------------------
// ✅ GUI Updates - LVGL Timer
// -------------------------------------------------
static void ui_update_timer_cb(lv_timer_t *t)
{

  // Objects
  uint32_t cur = HttpStreamEngine::getPlaySession();

  // Output section
  static char a2dpName[32];

  ID3v2Meta meta;
  MP3StatusInfo info;
  A2DPConnectedDetails btinfo;

  // place calls that require constant updates here

  // Buffer stats - updates every n ticks
  ui_update_stats_bars(HttpStreamEngine::net_fill_percent(), AudioCore::pcm_buffer_percent());

  // Outputs = Updates ever n ticks
  ui_update_stats_outputs(AudioCore::is_i2s_output_enabled(), a2dp.isConnected(), a2dpName);

  // WiFi
  ui_update_stats_wifi(WiFi.status(), WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  // widgets here only get updated when session/track changes when ID3 tag done

  strlcpy(a2dpName, "disconnected", sizeof(a2dpName));

  if (a2dp.connected_details(btinfo))
  {
    const char *name = (btinfo.name && btinfo.name[0]) ? btinfo.name : "Unknown";
    snprintf(a2dpName, sizeof(a2dpName), "%s (%s)", name, btinfo.auto_reconnect ? "Auto" : "New");
  }

  // ID3 metadata
  if (HttpStreamEngine::getID3(meta) && meta.header_found)
  {
    ui_update_player_id3(true, meta.artist, meta.title, meta.album, (int)meta.track, meta.albumArtValid ? meta.albumArtImage : nullptr, 120, 120);
  }
  else
  {
    ui_update_player_id3(true, "-", "-", "-", 0, nullptr, 0, 0);
  }

  // Decoder info
  if (AudioCore::getMP3Info(info))
  {
    ui_update_stats_decoder(info.codec, info.samplerate, info.channels, info.kbps);
  }
}

// -------------------------------------------------
// ✅ NON-BLOCKING FLUSH (CRITICAL FIX)
// -------------------------------------------------
static void my_flush(lv_display_t *disp,
                     const lv_area_t *area,
                     uint8_t *px_map)
{
  const uint32_t w = area->x2 - area->x1 + 1;
  const uint32_t h = area->y2 - area->y1 + 1;

  display.startWrite();
  display.setAddrWindow(area->x1, area->y1, w, h);

  // ✅ Chunked write with yields → avoids starvation
  const size_t total = w * h;
  const size_t CHUNK = 256;

  lgfx::rgb565_t *p = (lgfx::rgb565_t *)px_map;

  for (size_t i = 0; i < total; i += CHUNK)
  {
    size_t n = (i + CHUNK <= total) ? CHUNK : (total - i);
    display.writePixels(p + i, n);
    vTaskDelay(0); // ✅ yield to decoder / BT
  }

  display.endWrite();

  lv_display_flush_ready(disp);
}

// -------------------------------------------------
// Touch
// -------------------------------------------------
static void my_touch_read(lv_indev_t *, lv_indev_data_t *data)
{
  uint16_t x = 0, y = 0;
  bool pressed = false;
  bool ok = display.readTouch(x, y, pressed);

  data->state =
      (ok && pressed) ? LV_INDEV_STATE_PRESSED
                      : LV_INDEV_STATE_RELEASED;

  if (ok && pressed)
  {
    data->point.x = x;
    data->point.y = y;
  }
}
