All in one HTTP streaming, decoding & output

Must use WROVER with 8MB PSRAM - required for BT classic mode

- Fully Automated MP3 & AAC detection at header level
- Supports ID3v2 Tags
- Supports A2DP sink devices (headphones) & Auto Reconnect code
- Supports I2S Output - PCM5102 used when developing
- Dual Read PCM buffer for sim outputs independantly
- Supports LVGL with touch (Runable Demo included) for ST7796
- Supports Pause, Play, Stop, Next & Prev tracks

Issues

LVGL + BT + WiFi pushes DRAM too far, so LVGL runs mostly in PSRAM so the dual-buffering is increidibly slow, however putting lvbuf1 in DRAM crashes WiFi + BT, again if pushed too far.
WiFi due to co-existance crashes sometimes. 
I rolled back to ESP32 version 6.4.0 to work around the issues.

Latest ESP32 version causes BT to try to sleep, and it's generally unstable with A2DP.

BT Sometimes crashes - I am working on this, however for the most part it's down to lack of available DRAM.

To-Do

- Plex Media Server intergration
- Bug fixes
- Ogg Vorbis & support for other codes + better AAC support
- LVGL dual buffering in DRAM

Pros

Pretty stable on the whole

Currently Testing

LVGL running in DRAM - 4 lines single buffer
24 hour soak test in progess

PCB V1.1 Production in progress.


Recommend Kit without PCB

ESP32 WROVER with 8MB PSRAM with TF Card
- https://www.aliexpress.com/item/32879370278.html?spm=a2g0o.order_list.order_list_main.151.73d21802kujr90

4" TFT Capacitive Touch
- https://www.aliexpress.com/item/1005006698127763.html?spm=a2g0o.order_list.order_list_main.106.73d21802kujr90

PCM5102 DAC (I2S with Stereo RCA and headphone/lineout jack outputs)
- https://www.aliexpress.com/item/1005005352684938.html?spm=a2g0o.order_list.order_list_main.136.73d21802kujr90

