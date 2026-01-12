#include <Arduino.h>
#include "bindings.h"
#include "ui.h"       // EEZ Studio generated header
#include "screens.h"  // for 'objects'
#include "images.h"  // for 'objects'

static uint8_t last_net = 255;
static uint8_t last_pcm = 255;
char buf[8];

static inline uint8_t clamp_pct(int v) {
    if (v < 0)   return 0;
    if (v > 100) return 100;
    return (uint8_t)v;
}

void ui_update_stats_bars(uint8_t net_pct, uint8_t pcm_pct)
{
    net_pct = clamp_pct(net_pct);
    pcm_pct = clamp_pct(pcm_pct);

    // NET bar ---------------------------------------------------------
    if (net_pct != last_net) {
        last_net = net_pct;
        if (objects.stats_bar_net) {
             
            snprintf(buf, sizeof(buf), "%u%%", net_pct);
            lv_bar_set_value(objects.stats_bar_net, net_pct, LV_ANIM_ON);
            lv_label_set_text(objects.stats_lbl_pct_net, buf);
        }
    }

    // PCM bar ---------------------------------------------------------
    if (pcm_pct != last_pcm) {
        last_pcm = pcm_pct;
        if (objects.stats_bar_pcm) {
            snprintf(buf, sizeof(buf), "%u%%", pcm_pct);
            lv_bar_set_value(objects.stats_bar_pcm, pcm_pct, LV_ANIM_ON);
            lv_label_set_text(objects.stats_lbl_pct_pcm, buf);
        }
    }
}

void ui_update_stats_decoder(const char* codec,
                             uint32_t samplerate,
                             uint8_t channels,
                             uint16_t kbps)
{
    static char last_codec[16] = "";
    static uint32_t last_sr = 0;
    static uint8_t last_ch = 0;
    static uint16_t last_kbps = 0;

    char buf[32];

    /* ---------------- CODEC NAME ---------------- */
    if (codec && strcmp(codec, last_codec) != 0) {
        strncpy(last_codec, codec, sizeof(last_codec));
        last_codec[sizeof(last_codec)-1] = '\0';

        if (objects.stats_lbl_codec) {
            lv_label_set_text(objects.stats_lbl_codec, last_codec);
        }
    }

    /* ---------------- SAMPLE RATE ---------------- */
    if (samplerate != last_sr) {
        last_sr = samplerate;

        if (objects.stats_lbl_samplerate) {
            snprintf(buf, sizeof(buf), "%lu Hz", (unsigned long)samplerate);
            lv_label_set_text(objects.stats_lbl_samplerate, buf);
        }
    }

    /* ---------------- CHANNELS ---------------- */
    if (channels != last_ch) {
        last_ch = channels;

        if (objects.stats_lbl_channels) {
            snprintf(buf, sizeof(buf), "%u ch", channels);
            lv_label_set_text(objects.stats_lbl_channels, buf);
        }
    }

    /* ---------------- BITRATE ---------------- */
    if (kbps != last_kbps) {
        last_kbps = kbps;

        if (objects.stats_lbl_bitrate) {
            snprintf(buf, sizeof(buf), "%u kbps", kbps);
            lv_label_set_text(objects.stats_lbl_bitrate, buf);
        }
    }
}

void ui_update_stats_outputs(bool i2s_on,
                             bool a2dp_on,
                             const char* a2dp_name)
{
    static bool last_i2s_on   = false;
    static bool last_a2dp_on  = false;
    static char last_name[64] = "";   // ✅ was too small

    /* ---------------- I2S OUTPUT STATE ---------------- */
    if (i2s_on != last_i2s_on) {
        last_i2s_on = i2s_on;

        if (objects.stats_lbl_i2s_on) {
            lv_label_set_text(objects.stats_lbl_i2s_on,
                              i2s_on ? "ON" : "OFF");
        }
    }

    /* ---------------- A2DP OUTPUT STATE ---------------- */
    if (a2dp_on != last_a2dp_on) {
        last_a2dp_on = a2dp_on;

        if (objects.stats_lbl_a2dp_on) {
            lv_label_set_text(objects.stats_lbl_a2dp_on,
                              a2dp_on ? "ON" : "OFF");
        }
    }

    /* ---------------- A2DP DEVICE NAME ---------------- */
    if (objects.stats_lbl_a2dp_name) {

        // ✅ Update label if name OR connection state changes
        if (!a2dp_on) {
            lv_label_set_text(objects.stats_lbl_a2dp_name, "[None]");
        }
        else if (a2dp_name && strcmp(a2dp_name, last_name) != 0) {

            strncpy(last_name, a2dp_name, sizeof(last_name) - 1);
            last_name[sizeof(last_name) - 1] = '\0';

            lv_label_set_text(objects.stats_lbl_a2dp_name, last_name);
        }
    }
}

void ui_update_stats_wifi(bool connected, const char* ssid, const char* ip) {

    static bool  last_connected = false;
    static char  last_ssid[32]  = "";
    static char  last_ip[32]    = "";

    if (objects.stats_lbl_wifi_on) {
            lv_label_set_text(objects.stats_lbl_wifi_on,
                              connected ? "Connected" : "Disconnected");
        }

    /* ---------------- WIFI STATE ---------------- */
    if (connected != last_connected) {
        last_connected = connected;

        // If we just became disconnected, clear the other labels immediately
        if (!connected) {
            last_ssid[0] = '\0';
            last_ip[0]   = '\0';

            if (objects.stats_lbl_wifi_ssid) {
                lv_label_set_text(objects.stats_lbl_wifi_ssid, "");
            }
            if (objects.stats_lbl_wifi_ipaddress) {
                lv_label_set_text(objects.stats_lbl_wifi_ipaddress, "");
            }
        }
    }

    /* If not connected, we're done (keep SSID/IP blank). */
    if (!connected) {
        return;
    }

    /* ---------------- SSID / AP NAME ---------------- */
    const char* ssid_safe = (ssid && *ssid) ? ssid : "";

    if (strcmp(ssid_safe, last_ssid) != 0) {
        strncpy(last_ssid, ssid_safe, sizeof(last_ssid) - 1);
        last_ssid[sizeof(last_ssid) - 1] = '\0';

        if (objects.stats_lbl_wifi_ssid) {
            lv_label_set_text(objects.stats_lbl_wifi_ssid, last_ssid);
        }
    }

    /* ---------------- IP ADDRESS ---------------- */
    const char* ip_safe = (ip && *ip) ? ip : "";

    if (strcmp(ip_safe, last_ip) != 0) {
        strncpy(last_ip, ip_safe, sizeof(last_ip) - 1);
        last_ip[sizeof(last_ip) - 1] = '\0';

        if (objects.stats_lbl_wifi_ipaddress) {
            lv_label_set_text(objects.stats_lbl_wifi_ipaddress, last_ip);
        }
    }
}

// Put this near the top of the file (once), using YOUR asset name:
//LV_IMG_DECLARE(img_no_art);

void ui_update_player_id3(bool has_meta,
                          const char* artist,
                          const char* title,
                          const char* album,
                          int track,
                          const uint16_t* pixels,
                          uint16_t w,
                          uint16_t h)
{
    // Cache last values so LVGL is only updated when needed
    static bool last_have_meta = false;
    static char last_artist[64] = "";
    static char last_title[64]  = "";
    static char last_album[64]  = "";
    static int  last_track      = 0;

    // Album art cache
    static const uint16_t* last_pixels = nullptr;
    static uint16_t last_w = 0, last_h = 0;
    static bool last_default_art = true;      // what we last displayed
    static lv_image_dsc_t img_dsc;

    char buf[32];

    // Default art pointer (LVGL built-in image asset)
    const void* default_art = &img_no_albumart;

    /* ---------- NO METADATA ---------- */
    if (!has_meta) {

        if (last_have_meta) {
            last_have_meta = false;

            last_artist[0] = '\0';
            last_title[0]  = '\0';
            last_album[0]  = '\0';
            last_track     = 0;

            if (objects.player_lbl_artist)
                lv_label_set_text(objects.player_lbl_artist, "—");
            if (objects.player_lbl_title)
                lv_label_set_text(objects.player_lbl_title, "—");
            if (objects.player_lbl_album)
                lv_label_set_text(objects.player_lbl_album, "—");
            if (objects.player_lbl_tracknumber)
                lv_label_set_text(objects.player_lbl_tracknumber, "—");
        }

        // Show default image when no metadata
        if (objects.player_img_albumart && !last_default_art) {
            last_pixels = nullptr;
            last_w = last_h = 0;
            last_default_art = true;
            lv_image_set_src(objects.player_img_albumart, default_art);
        }

        return;
    }

    last_have_meta = true;

    /* ---------- ARTIST ---------- */
    const char* artist_safe = (artist && *artist) ? artist : "Unknown Artist";
    if (!artist || !*artist) {
        last_artist[0] = '\0';   // force refresh
    }

    if (strcmp(artist_safe, last_artist) != 0) {
        strncpy(last_artist, artist_safe, sizeof(last_artist) - 1);
        last_artist[sizeof(last_artist) - 1] = '\0';

        if (objects.player_lbl_artist)
            lv_label_set_text(objects.player_lbl_artist, last_artist);
    }

    /* ---------- TITLE ---------- */
    const char* title_safe = (title && *title) ? title : "Unknown Title";
    if (!title || !*title) {
        last_title[0] = '\0';
    }

    if (strcmp(title_safe, last_title) != 0) {
        strncpy(last_title, title_safe, sizeof(last_title) - 1);
        last_title[sizeof(last_title) - 1] = '\0';

        if (objects.player_lbl_title)
            lv_label_set_text(objects.player_lbl_title, last_title);
    }

    /* ---------- ALBUM ---------- */
    const char* album_safe = (album && *album) ? album : "Unknown Album";
    if (!album || !*album) {
        last_album[0] = '\0';
    }

    if (strcmp(album_safe, last_album) != 0) {
        strncpy(last_album, album_safe, sizeof(last_album) - 1);
        last_album[sizeof(last_album) - 1] = '\0';

        if (objects.player_lbl_album)
            lv_label_set_text(objects.player_lbl_album, last_album);
    }

    /* ---------- TRACK NUMBER ---------- */
    if (track != last_track) {
        last_track = track;

        if (objects.player_lbl_tracknumber) {
            if (track > 0)
                snprintf(buf, sizeof(buf), "%d", track);
            else
                strcpy(buf, "—");

            lv_label_set_text(objects.player_lbl_tracknumber, buf);
        }
    }

    /* ---------- ALBUM ART ---------- */
    if (!objects.player_img_albumart)
        return;

    // If no pixels (or invalid size): show default art
    if (!pixels || w == 0 || h == 0) {
        if (!last_default_art) {
            last_pixels = nullptr;
            last_w = last_h = 0;
            last_default_art = true;
            lv_image_set_src(objects.player_img_albumart, default_art);
        }
        return;
    }

    // If same pointer+size and we previously showed raw art, do nothing
    if (!last_default_art && pixels == last_pixels && w == last_w && h == last_h)
        return;

    // Switch to raw art
    last_default_art = false;
    last_pixels = pixels;
    last_w = w;
    last_h = h;

    img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    img_dsc.header.w  = w;
    img_dsc.header.h  = h;
    img_dsc.data      = (const uint8_t*)pixels;
    img_dsc.data_size = (uint32_t)w * (uint32_t)h * 2u;

    lv_image_set_src(objects.player_img_albumart, &img_dsc);
}

// wifi connected status label
void ui_wifi_set_status(const char* text, bool connected) {
    if (!objects.wifi_lbl_connectstatus) return;

    lv_label_set_text(objects.wifi_lbl_connectstatus, text);

    if (connected) {
        lv_obj_set_style_text_color(objects.wifi_lbl_connectstatus,
                                    lv_color_hex(0x00C853), // green
                                    LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(objects.wifi_lbl_connectstatus,
                                    lv_color_hex(0xD50000), // red
                                    LV_PART_MAIN);
    }
}

void ui_wifi_status_poll(lv_timer_t* t) {
    (void)t;

    wl_status_t st = WiFi.status();

    if (st == WL_CONNECTED) {
        ui_wifi_set_status("Connected", true);
    } else if (st == WL_CONNECT_FAILED ||
               st == WL_CONNECTION_LOST ||
               st == WL_DISCONNECTED) {
        ui_wifi_set_status("Disconnected", false);
    }
}
