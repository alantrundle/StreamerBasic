#include "actions.h"
#include "ui.h"        // EEZ Studio export, contains ui_start, ui_* objects

#include "LVGLCore.h"
#include "AudioPlayer.h"  

#include "A2DPCore.h"  

extern A2DPCore a2dp;

bool isPaused = false;

extern void AudioPlayer_Play();
extern void AudioPlayer_Pause();
extern void AudioPlayer_Stop();
extern void AudioPlayer_Next();
extern void AudioPlayer_Prev();

// -----------------------------------------------------------------------------
// Helper: switch to START screen
// -----------------------------------------------------------------------------
static void goto_start_view()
{
    if (objects.start) {
        lv_scr_load_anim( objects.start, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);                       
    }
}

static void goto_stats_view()
{
    if (objects.stats) {
        lv_scr_load_anim( objects.stats, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);                 
    }
}

static void goto_player_view()
{
    if (objects.player) {
        lv_scr_load_anim( objects.player, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);                 
    }
}

static void goto_bluetooth_view()
{
    if (objects.player) {
        lv_scr_load_anim( objects.bluetooth, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);                 
    }
}

static void goto_wifi_view()
{
    if (objects.player) {
        lv_scr_load_anim( objects.wifi, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);                 
    }
}

static void goto_brightness_view()
{
    if (objects.player) {
        lv_scr_load_anim( objects.brightness, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);                 
    }
}

static void drag_brightness() {
    int32_t val = lv_slider_get_value(objects.slider_brightness);
    display.setBrightness(val);
}





// -----------------------------------------------------------------------------
// action_back: Assigned in EEZ Studio for “Back” buttons
// -----------------------------------------------------------------------------
void action_back(lv_event_t * e)
{
    LV_UNUSED(e);   // avoid compiler warnings
    goto_start_view();
}

void action_goto_stats_view(lv_event_t * e){
    LV_UNUSED(e);   // avoid compiler warnings
    goto_stats_view();
}

void action_goto_player_view(lv_event_t * e) {
    LV_UNUSED(e);   // avoid compiler warnings
    goto_player_view();
}

void action_goto_bluetooth_view(lv_event_t * e) {
    LV_UNUSED(e);   // avoid compiler warnings
    goto_bluetooth_view();
}

void action_goto_wifi_view(lv_event_t * e) {
    LV_UNUSED(e);   // avoid compiler warnings
    goto_wifi_view();
}

void action_goto_brightness_view(lv_event_t * e) {
    LV_UNUSED(e);   // avoid compiler warnings
    goto_brightness_view();
}

void action_brightness_drag(lv_event_t * e) {
    LV_UNUSED(e);   // avoid compiler warnings
    drag_brightness();
}

void action_player_play_pause(lv_event_t * e) {

    if(isPaused) {
        lv_label_set_text(objects.player_lbl_btn_play_pause, "Play");
        AudioPlayer_Pause();
        isPaused = false;
    } else {
        lv_label_set_text(objects.player_lbl_btn_play_pause, "Pause");
        AudioPlayer_Play();
        isPaused = true;
    }

}

void action_player_previous(lv_event_t * e) {
    AudioPlayer_Prev();
}

void action_player_next(lv_event_t * e) {
    AudioPlayer_Next();
}

// Blueooth View
void action_bluetooth_startscan(lv_event_t * e) {

    if(a2dp.isScanning()) {
        lv_label_set_text(objects.bt_btn_start, "Stop Scan");
        a2dp.stop_scan();
    } else if (!a2dp.scan_blocked() && !a2dp.isScanning() && !a2dp.isConnected()) {
        lv_label_set_text(objects.bt_btn_start, "Start Scan");
        a2dp.erase_autoreconnect_table();
        a2dp.start_scan(10);
    }

}