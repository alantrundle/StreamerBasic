#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *start;
    lv_obj_t *stats;
    lv_obj_t *player;
    lv_obj_t *bluetooth;
    lv_obj_t *buffers;
    lv_obj_t *obj0;
    lv_obj_t *stats_btn;
    lv_obj_t *start_btn;
    lv_obj_t *start_btn_1;
    lv_obj_t *obj1;
    lv_obj_t *stats_lbl_pct_net;
    lv_obj_t *stats_lbl_pct_pcm;
    lv_obj_t *stats_bar_net;
    lv_obj_t *stats_bar_pcm;
    lv_obj_t *stats_lbl_bitrate;
    lv_obj_t *stats_lbl_channels;
    lv_obj_t *stats_lbl_samplerate;
    lv_obj_t *stats_lbl_codec;
    lv_obj_t *stats_lbl_a2dp_name;
    lv_obj_t *stats_lbl_i2s_on;
    lv_obj_t *stats_lbl_a2dp_on;
    lv_obj_t *stats_lbl_a2dp_name_1;
    lv_obj_t *stats_lbl_i2s_on_1;
    lv_obj_t *stats_lbl_a2dp_on_1;
    lv_obj_t *stats_lbl_bitrate_1;
    lv_obj_t *stats_lbl_wifi_ssid;
    lv_obj_t *stats_lbl_wifi_ipaddress;
    lv_obj_t *stats_lbl_wifi_on;
    lv_obj_t *obj2;
    lv_obj_t *player_img_albumart;
    lv_obj_t *player_lbl_btn_play_pause;
    lv_obj_t *player_lbl_title;
    lv_obj_t *player_lbl_artist;
    lv_obj_t *player_lbl_album;
    lv_obj_t *player_lbl_tracknumber;
    lv_obj_t *obj3;
    lv_obj_t *bt_btn_start;
    lv_obj_t *bt_btn_connect;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_START = 1,
    SCREEN_ID_STATS = 2,
    SCREEN_ID_PLAYER = 3,
    SCREEN_ID_BLUETOOTH = 4,
    SCREEN_ID_BUFFERS = 5,
};

void create_screen_start();
void tick_screen_start();

void create_screen_stats();
void tick_screen_stats();

void create_screen_player();
void tick_screen_player();

void create_screen_bluetooth();
void tick_screen_bluetooth();

void create_screen_buffers();
void tick_screen_buffers();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/