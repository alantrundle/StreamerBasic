#include <LovyanGFX.hpp>
#include "gfxDriver.h"
#include <Arduino.h>
#include <Wire.h>

/* ---- Global instance ---- */
LGFX_ST7796 display;

LGFX_ST7796::LGFX_ST7796() {
  /* ---- SPI bus ---- */
  {
    auto cfg = _bus.config();
    cfg.spi_host    = SPI3_HOST;     // VSPI
    cfg.spi_mode    = 0;
    cfg.freq_write  = 80000000;      // 80 MHz
    cfg.freq_read   = 16000000;
    cfg.spi_3wire   = false;
    cfg.use_lock    = true;
    cfg.dma_channel = 1;
    cfg.pin_sclk    = TFT_SCLK;
    cfg.pin_mosi    = TFT_MOSI;
    cfg.pin_miso    = TFT_MISO;
    cfg.pin_dc      = TFT_DC;
    _bus.config(cfg);
    _panel.setBus(&_bus);
  }

  /* ---- Panel ---- */
  {
    auto cfg = _panel.config();
    cfg.pin_cs        = TFT_CS;
    cfg.pin_rst       = TFT_RST;
    cfg.pin_busy      = -1;
    cfg.panel_width   = 320;       // ST7796 is 320x480
    cfg.panel_height  = 480;
    cfg.memory_width  = 320;
    cfg.memory_height = 480;
    cfg.offset_x = 0;
    cfg.offset_y = 0;
    cfg.offset_rotation = 0;
    cfg.readable   = false;
    cfg.invert     = false;
    cfg.rgb_order  = false;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;
    _panel.config(cfg);
  }

  /* ---- Backlight ---- */
  {
    auto cfg = _light.config();
    cfg.pin_bl      = TFT_BL;
    cfg.invert      = false;
    cfg.freq        = 20000;
    cfg.pwm_channel = 7;
    _light.config(cfg);
    _panel.setLight(&_light);
  }

  /* ---- Touch (FT6336/FT5x06) ---- */
  {
    // I2C is handled by LovyanGFX driver; configure pins and timing.
    auto tc = _touch.config();
    tc.x_min      = 0;
    tc.y_min      = 0;
    tc.x_max      = 320;        // raw portrait range for ST7796 module
    tc.y_max      = 480;
    tc.i2c_port   = 0;          // I2C_NUM_0
    tc.i2c_addr   = TOUCH_ADDR; // 0x38
    tc.pin_sda    = TOUCH_SDA;
    tc.pin_scl    = TOUCH_SCL;
    tc.pin_int    = TOUCH_INT;  // your INT pin = 5
    tc.pin_rst    = TOUCH_RST;  // your RST pin = 4
    tc.freq       = 400000;
    _touch.config(tc);

    _panel.setTouch(&_touch);   // attach touch to the panel
  }

  setPanel(&_panel);
}

bool LGFX_ST7796::readTouch(uint16_t &x, uint16_t &y, bool &pressed) {
  int32_t ix = 0, iy = 0;
  if (this->getTouch(&ix, &iy)) {     // <- use LGFX_Device::getTouch(...)
    x = (uint16_t)ix;
    y = (uint16_t)iy;
    pressed = true;
    return true;
  }
  pressed = false;
  return false;
}


