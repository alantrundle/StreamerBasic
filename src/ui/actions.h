#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_back(lv_event_t * e);
extern void action_goto_player_view(lv_event_t * e);
extern void action_goto_stats_view(lv_event_t * e);
extern void action_player_previous(lv_event_t * e);
extern void action_player_next(lv_event_t * e);
extern void action_player_play_pause(lv_event_t * e);
extern void action_goto_bluetooth_view(lv_event_t * e);
extern void action_bluetooth_startscan(lv_event_t * e);
extern void action_goto_wifi_view(lv_event_t * e);
extern void action_goto_brightness_view(lv_event_t * e);
extern void action_brightness_drag(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/