#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

void create_screen_start() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.start = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 33);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff149aa0), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "AT1053 Audio Streamer");
        }
        {
            // btn_menu_information
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.btn_menu_information = obj;
            lv_obj_set_pos(obj, 13, 45);
            lv_obj_set_size(obj, 50, 50);
            lv_obj_add_event_cb(obj, action_goto_stats_view, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_image_src(obj, &img_information_50, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // btn_menu_player
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.btn_menu_player = obj;
            lv_obj_set_pos(obj, 149, 45);
            lv_obj_set_size(obj, 50, 50);
            lv_obj_add_event_cb(obj, action_goto_player_view, LV_EVENT_PRESSED, (void *)0);
            lv_obj_add_state(obj, LV_STATE_DISABLED);
            lv_obj_set_style_bg_image_src(obj, &img_player_50, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // btn_menu_bluetooth
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.btn_menu_bluetooth = obj;
            lv_obj_set_pos(obj, 284, 45);
            lv_obj_set_size(obj, 50, 50);
            lv_obj_add_event_cb(obj, action_goto_bluetooth_view, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_image_src(obj, &img_bluetooth_50, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // btn_menu_wifi
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.btn_menu_wifi = obj;
            lv_obj_set_pos(obj, 415, 45);
            lv_obj_set_size(obj, 50, 50);
            lv_obj_add_event_cb(obj, action_goto_wifi_view, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_image_src(obj, &img_wifi_50, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 13, 105);
            lv_obj_set_size(obj, 50, 16);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Info");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 149, 105);
            lv_obj_set_size(obj, 50, 16);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Player");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 284, 105);
            lv_obj_set_size(obj, 50, 16);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "BT");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 415, 105);
            lv_obj_set_size(obj, 50, 16);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "WiFi");
        }
        {
            // btn_menu_brigtness
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.btn_menu_brigtness = obj;
            lv_obj_set_pos(obj, 415, 228);
            lv_obj_set_size(obj, 50, 50);
            lv_obj_add_event_cb(obj, action_goto_brightness_view, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_image_src(obj, &img_brightness_50, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 415, 288);
            lv_obj_set_size(obj, 50, 16);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Screen");
        }
    }
    
    tick_screen_start();
}

void tick_screen_start() {
}

void create_screen_stats() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.stats = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_back, LV_EVENT_PRESSED, (void *)0);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 33);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff149aa0), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Stats");
        }
        {
            // btn_stats_back
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.btn_stats_back = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 85, 33);
            lv_obj_add_event_cb(obj, action_back, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff149aa0), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "< back");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 7, 46);
            lv_obj_set_size(obj, 470, 66);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -10, 20);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "PCM");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -6, -5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "NET");
                }
                {
                    // statsLblPctNet
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_pct_net = obj;
                    lv_obj_set_pos(obj, 408, -5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "100%");
                }
                {
                    // statsLblPctPcm
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_pct_pcm = obj;
                    lv_obj_set_pos(obj, 408, 20);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "100%");
                }
                {
                    // statsBarNet
                    lv_obj_t *obj = lv_bar_create(parent_obj);
                    objects.stats_bar_net = obj;
                    lv_obj_set_pos(obj, 26, -5);
                    lv_obj_set_size(obj, 376, 15);
                    lv_bar_set_value(obj, 5, LV_ANIM_OFF);
                }
                {
                    // statsBarPcm
                    lv_obj_t *obj = lv_bar_create(parent_obj);
                    objects.stats_bar_pcm = obj;
                    lv_obj_set_pos(obj, 26, 20);
                    lv_obj_set_size(obj, 376, 15);
                    lv_bar_set_value(obj, 5, LV_ANIM_OFF);
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 7, 122);
            lv_obj_set_size(obj, 227, 105);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // statsLblBitrate
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_bitrate = obj;
                    lv_obj_set_pos(obj, 58, 50);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "0");
                }
                {
                    // statsLblChannels
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_channels = obj;
                    lv_obj_set_pos(obj, 58, 31);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "0");
                }
                {
                    // statsLblSamplerate
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_samplerate = obj;
                    lv_obj_set_pos(obj, 58, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "0");
                }
                {
                    // statsLblCodec
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_codec = obj;
                    lv_obj_set_pos(obj, 58, -7);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "NONE");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -24, 31);
                    lv_obj_set_size(obj, 71, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Channels:");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -10, 50);
                    lv_obj_set_size(obj, 57, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Bitrate:");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -8, -7);
                    lv_obj_set_size(obj, 53, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Codec:");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 11, 12);
                    lv_obj_set_size(obj, 34, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "SR:");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 250, 122);
            lv_obj_set_size(obj, 227, 105);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // statsLblA2dpName
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_a2dp_name = obj;
                    lv_obj_set_pos(obj, 58, 31);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "NONE");
                }
                {
                    // statsLblI2sON
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_i2s_on = obj;
                    lv_obj_set_pos(obj, 58, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "NO");
                }
                {
                    // statsLblA2dp_On
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_a2dp_on = obj;
                    lv_obj_set_pos(obj, 58, -7);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "NO");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -24, 31);
                    lv_obj_set_size(obj, 71, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Sink:");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -8, -7);
                    lv_obj_set_size(obj, 53, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "A2DP:");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 11, 12);
                    lv_obj_set_size(obj, 34, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "I2S:");
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_pos(obj, -260, 96);
                    lv_obj_set_size(obj, 469, 73);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // statsLblA2dpName_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.stats_lbl_a2dp_name_1 = obj;
                            lv_obj_set_pos(obj, 58, 31);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "NONE");
                        }
                        {
                            // statsLblI2sON_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.stats_lbl_i2s_on_1 = obj;
                            lv_obj_set_pos(obj, 58, 12);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "NO");
                        }
                        {
                            // statsLblA2dp_On_1
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.stats_lbl_a2dp_on_1 = obj;
                            lv_obj_set_pos(obj, 58, -7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "NO");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, -24, 31);
                            lv_obj_set_size(obj, 71, 19);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Sink:");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, -8, -7);
                            lv_obj_set_size(obj, 53, 19);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "A2DP:");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 11, 12);
                            lv_obj_set_size(obj, 34, 19);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "I2S:");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -259, 136);
                    lv_obj_set_size(obj, 71, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Sink:");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -243, 98);
                    lv_obj_set_size(obj, 53, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "A2DP:");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -224, 117);
                    lv_obj_set_size(obj, 34, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "I2S:");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 7, 238);
            lv_obj_set_size(obj, 470, 71);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // statsLblBitrate_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_bitrate_1 = obj;
                    lv_obj_set_pos(obj, 58, 52);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "0");
                }
                {
                    // statsLblWifiSsid
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_wifi_ssid = obj;
                    lv_obj_set_pos(obj, 85, 30);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "NONE");
                }
                {
                    // statsLblWifiIpaddress
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_wifi_ipaddress = obj;
                    lv_obj_set_pos(obj, 85, 11);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "0.0.0.0");
                }
                {
                    // statsLblWifiOn
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.stats_lbl_wifi_on = obj;
                    lv_obj_set_pos(obj, 85, -9);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Disconnected");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 6, 30);
                    lv_obj_set_size(obj, 71, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "AP Name:");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -10, 50);
                    lv_obj_set_size(obj, 57, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Bitrate:");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -6, -9);
                    lv_obj_set_size(obj, 83, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Connected:");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -6, 11);
                    lv_obj_set_size(obj, 83, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "IP Address:");
                }
            }
        }
    }
    
    tick_screen_stats();
}

void tick_screen_stats() {
}

void create_screen_player() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.player = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_back, LV_EVENT_PRESSED, (void *)0);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj2 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 33);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff149aa0), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Player");
        }
        {
            // btn_player_back
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.btn_player_back = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 85, 33);
            lv_obj_add_event_cb(obj, action_back, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff149aa0), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 1, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "< back");
                }
            }
        }
        {
            // playerImgAlbumart
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.player_img_albumart = obj;
            lv_obj_set_pos(obj, 21, 48);
            lv_obj_set_size(obj, 120, 120);
            lv_image_set_src(obj, &img_no_albumart);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 202, 198);
            lv_obj_set_size(obj, 77, 50);
            lv_obj_add_event_cb(obj, action_player_play_pause, LV_EVENT_RELEASED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // playerLblBtnPlayPause
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.player_lbl_btn_play_pause = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Play");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 95, 198);
            lv_obj_set_size(obj, 77, 50);
            lv_obj_add_event_cb(obj, action_player_previous, LV_EVENT_RELEASED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Previous");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 310, 198);
            lv_obj_set_size(obj, 77, 50);
            lv_obj_add_event_cb(obj, action_player_next, LV_EVENT_RELEASED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Next");
                }
            }
        }
        {
            // playerLblTitle
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.player_lbl_title = obj;
            lv_obj_set_pos(obj, 174, 53);
            lv_obj_set_size(obj, 230, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "-");
        }
        {
            // playerLblArtist
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.player_lbl_artist = obj;
            lv_obj_set_pos(obj, 174, 86);
            lv_obj_set_size(obj, 250, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "-");
        }
        {
            // playerLblAlbum
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.player_lbl_album = obj;
            lv_obj_set_pos(obj, 174, 118);
            lv_obj_set_size(obj, 250, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "-");
        }
        {
            // playerLblTracknumber
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.player_lbl_tracknumber = obj;
            lv_obj_set_pos(obj, 174, 148);
            lv_obj_set_size(obj, 250, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "-");
        }
        {
            // img_a2dp
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_a2dp = obj;
            lv_obj_set_pos(obj, 411, 44);
            lv_obj_set_size(obj, 25, 25);
            lv_obj_set_style_bg_image_src(obj, &img_a2dp_off, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // img_i2s
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_i2s = obj;
            lv_obj_set_pos(obj, 444, 44);
            lv_obj_set_size(obj, 25, 25);
            lv_obj_set_style_bg_image_src(obj, &img_i2s_off, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_image_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_image_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 147, 48);
            lv_obj_set_size(obj, 25, 25);
            lv_obj_set_style_bg_image_src(obj, &img_id3_title, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 147, 81);
            lv_obj_set_size(obj, 25, 25);
            lv_obj_set_style_bg_image_src(obj, &img_id3_artist, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 147, 113);
            lv_obj_set_size(obj, 25, 25);
            lv_obj_set_style_bg_image_src(obj, &img_id3_album, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 147, 143);
            lv_obj_set_size(obj, 25, 25);
            lv_obj_set_style_bg_image_src(obj, &img_id3_track, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_player();
}

void tick_screen_player() {
}

void create_screen_bluetooth() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.bluetooth = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj3 = obj;
            lv_obj_set_pos(obj, 1, 0);
            lv_obj_set_size(obj, 480, 33);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff149aa0), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Bluetooth");
        }
        {
            // btn_bluetooth_back
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.btn_bluetooth_back = obj;
            lv_obj_set_pos(obj, 0, 1);
            lv_obj_set_size(obj, 85, 32);
            lv_obj_add_event_cb(obj, action_back, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff149aa0), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "< back");
                }
            }
        }
        {
            // bt_btn_start
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.bt_btn_start = obj;
            lv_obj_set_pos(obj, 190, 66);
            lv_obj_set_size(obj, 100, 40);
            lv_obj_add_event_cb(obj, action_bluetooth_startscan, LV_EVENT_PRESSED, (void *)0);
            lv_obj_add_state(obj, LV_STATE_DISABLED);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Scan");
                }
            }
        }
        {
            // bt_devicelist
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.bt_devicelist = obj;
            lv_obj_set_pos(obj, 10, 168);
            lv_obj_set_size(obj, 340, LV_SIZE_CONTENT);
            lv_dropdown_set_options(obj, "No devices");
            lv_dropdown_set_selected(obj, 0);
        }
        {
            // btBtnConnect
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.bt_btn_connect = obj;
            lv_obj_set_pos(obj, 369, 168);
            lv_obj_set_size(obj, 100, 40);
            lv_obj_add_event_cb(obj, action_bt_connectdevice, LV_EVENT_PRESSED, (void *)0);
            lv_obj_add_state(obj, LV_STATE_DISABLED);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Connect");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 10, 259);
            lv_obj_set_size(obj, 459, 48);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // bt_lbl_lastdevice
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.bt_lbl_lastdevice = obj;
                    lv_obj_set_pos(obj, -11, -3);
                    lv_obj_set_size(obj, 224, 19);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0018), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "[EMPTY]");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 388, 267);
            lv_obj_set_size(obj, 70, 33);
            lv_obj_add_event_cb(obj, action_bt_delete_connecttable, LV_EVENT_PRESSED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Remove");
                }
            }
        }
    }
    
    tick_screen_bluetooth();
}

void tick_screen_bluetooth() {
}

void create_screen_wifi() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.wifi = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_back, LV_EVENT_PRESSED, (void *)0);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj4 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 33);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff149aa0), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "WiFi");
        }
        {
            // btn_wifi_back
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.btn_wifi_back = obj;
            lv_obj_set_pos(obj, 0, 1);
            lv_obj_set_size(obj, 85, 32);
            lv_obj_add_event_cb(obj, action_back, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff149aa0), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "< back");
                }
            }
        }
        {
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            lv_obj_set_pos(obj, 24, 79);
            lv_obj_set_size(obj, 216, LV_SIZE_CONTENT);
            lv_dropdown_set_options(obj, "None");
            lv_dropdown_set_selected(obj, 0);
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 295, 79);
            lv_obj_set_size(obj, 100, 40);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Connect");
                }
            }
        }
    }
    
    tick_screen_wifi();
}

void tick_screen_wifi() {
}

void create_screen_brightness() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.brightness = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj5 = obj;
            lv_obj_set_pos(obj, 0, 1);
            lv_obj_set_size(obj, 480, 33);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff149aa0), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Screen");
        }
        {
            // btn_wifi_back_1
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.btn_wifi_back_1 = obj;
            lv_obj_set_pos(obj, 0, 1);
            lv_obj_set_size(obj, 85, 33);
            lv_obj_add_event_cb(obj, action_back, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff149aa0), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // btn_brightness_back
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.btn_brightness_back = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "< back");
                }
            }
        }
        {
            // slider_brightness
            lv_obj_t *obj = lv_slider_create(parent_obj);
            objects.slider_brightness = obj;
            lv_obj_set_pos(obj, 13, 160);
            lv_obj_set_size(obj, 455, 25);
            lv_slider_set_range(obj, 10, 255);
            lv_slider_set_value(obj, 150, LV_ANIM_OFF);
            lv_obj_add_event_cb(obj, action_brightness_drag, LV_EVENT_PRESSING, (void *)0);
        }
    }
    
    tick_screen_brightness();
}

void tick_screen_brightness() {
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_start,
    tick_screen_stats,
    tick_screen_player,
    tick_screen_bluetooth,
    tick_screen_wifi,
    tick_screen_brightness,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_start();
    create_screen_stats();
    create_screen_player();
    create_screen_bluetooth();
    create_screen_wifi();
    create_screen_brightness();
}
