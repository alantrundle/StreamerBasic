#pragma once
#include <LovyanGFX.hpp>

/* ==== TFT pins (change if needed) ==== */
#ifndef TFT_SCLK
#  define TFT_SCLK 18
#endif
#ifndef TFT_MOSI
#  define TFT_MOSI 23
#endif
#ifndef TFT_MISO
#  define TFT_MISO 19
#endif
#ifndef TFT_DC
#  define TFT_DC   27
#endif
#ifndef TFT_CS
#  define TFT_CS   15
#endif
#ifndef TFT_RST
#  define TFT_RST  33
#endif
#ifndef TFT_BL
#  define TFT_BL   32
#endif

/* ==== Touch (FT6336/FT5x06) pins ==== */
#ifndef TOUCH_SDA
#  define TOUCH_SDA 21
#endif
#ifndef TOUCH_SCL
#  define TOUCH_SCL 22
#endif
#ifndef TOUCH_RST
#  define TOUCH_RST 4   // <- you asked for 4
#endif
#ifndef TOUCH_INT
#  define TOUCH_INT 5   // <- you asked for 5
#endif
#ifndef TOUCH_ADDR
#  define TOUCH_ADDR 0x38
#endif

class LGFX_ST7796 : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796 _panel;
  lgfx::Bus_SPI      _bus;
  lgfx::Light_PWM    _light;
  lgfx::Touch_FT5x06 _touch;   // FT6336/FT5x06-compatible

public:
  LGFX_ST7796();

  /** Simple wrapper for polling touch; returns true if pressed and fills x,y */
  bool readTouch(uint16_t &x, uint16_t &y, bool &pressed);
};

/** Global display instance (defined in gfx_driver.cpp) */
extern LGFX_ST7796 display;
