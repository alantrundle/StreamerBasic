# ESP32 Audio Streaming & Playback Platform

An all-in-one **ESP32-based audio platform** providing HTTP streaming, on-device decoding, and multiple audio outputs with a full graphical UI.  
Designed for **ESP32-WROVER (8MB PSRAM)** and intended for use with a custom PCB (v1.1) or off-the-shelf modules.

---

## 📸 PCB – Prototype (v1.1)

<p align="center">
  <img src="docs/front_stats11.JPG" width="50%">
  <img src="docs/front_player_closesup.JPG" width="50%">
  <img src="docs/front_top11.jpeg" width="50%">
  <img src="docs/rear_battery_11.jpeg" width="50%">
</p>

**Left → Right:**  
Front view with LVGL UI • Angled top view • Rear view with Li-Po battery fitted

> ⚠️ **ESP32-WROVER with 8MB PSRAM is mandatory**  
> Bluetooth Classic (A2DP) and LVGL require PSRAM for stability.

---

## ✨ Key Features

- **Automatic MP3 & AAC codec detection** (header-level)
- **ID3v2 tag parsing** (title, artist, album & JPEG Album Art)
- **Bluetooth A2DP output**
  - Line-out & A2DP sinks supported
  - Automatic reconnect logic
- **I2S audio output**
  - Developed using PCM5102 DAC
- **Dual-reader PCM ring buffer**
  - Independent A2DP & I2S consumption
- **LVGL graphical UI**
  - Touch support
  - ST7796 demo included
- **Transport controls**
  - Play / Pause / Stop
  - Next / Previous track

---

## 🧠 Architecture Overview

- HTTP audio streaming
- On-device MP3 / AAC decoding
- Shared PCM buffer with independent read pointers
- Parallel output to:
  - Bluetooth A2DP
  - I2S DAC
- LVGL UI running primarily from PSRAM
- Designed around **FreeRTOS task separation** and buffer hysteresis

---

## ⚠️ Known Issues & Limitations

- **LVGL + Bluetooth + Wi-Fi are extremely DRAM-heavy**
  - LVGL buffers must reside mostly in PSRAM
  - Moving `lvbuf1` to DRAM can crash Wi-Fi + BT
- **Dual buffering in PSRAM is slow**
  - Trade-off required for system stability
- **Wi-Fi coexistence instability**
  - Occasional crashes under heavy load
- **ESP-IDF versions**
  - Rolled back to **ESP32 core v6.4.0** for stability
  - Newer versions attempt BT sleep → A2DP instability
- **Bluetooth**
  - Rare crashes due to DRAM exhaustion
  - Actively under investigation

---

## 🧪 Current Testing Status

- LVGL running partially in DRAM  
  - Single-buffer, 4-line configuration
- **24-hour soak test** currently in progress
- PCB **v1.1 production** underway

---

## 🛠 To-Do

- Plex Media Server integration
- General bug fixes & cleanup
- Ogg Vorbis support
- Improved AAC decoding
- LVGL dual buffering in DRAM (if feasible)

---

## ✅ Pros

- Stable during extended playback
- Robust codec detection
- Clean separation of decode and output paths
- Scales well with PSRAM availability

---

## 📦 Recommended Development Kit (Without PCB)

### ESP32 Module
**ESP32-WROVER (8MB PSRAM + TF Card)**
- https://www.aliexpress.com/item/32879370278.html

### Display
**4″ TFT Capacitive Touch Display**  
(Required for PCB v1.1 – LVGL)
- https://www.aliexpress.com/item/1005006698127763.html

### Audio DAC
**PCM5102 I2S DAC**
- Stereo RCA + headphone/line-out
- https://www.aliexpress.com/item/1005005352684938.html

---

## 📄 License

**Project status:** Active Development  

**License:**  
Free for **personal, educational, and hobbyist use**.

**Commercial use is not permitted without explicit written permission from the author.**  
This includes (but is not limited to) use in commercial products, services, or redistributed hardware.

If you wish to use this project commercially, please contact the author via GitHub.

---

## 🙌 Acknowledgements

- ESP32 Arduino Core
- LVGL Graphics Library
- Helix Audio Codecs
