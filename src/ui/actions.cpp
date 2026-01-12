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

// Wifi Globals
#define MAX_WIFI_APS 20

static char wifi_ssid_list[MAX_WIFI_APS][33]; // SSID max 32 + null
static int  wifi_ap_count = 0;

// --------------------------------------------------
// Static popup state (Wi-Fi password + keyboard)
// --------------------------------------------------
static lv_obj_t* s_wifi_popup   = nullptr;
static lv_obj_t* s_wifi_pass_ta = nullptr;
static lv_obj_t* s_wifi_kb      = nullptr;

static char s_wifi_ssid[33] = {0};

static void ui_wifi_scan_and_populate_dropdown();
static void wifi_popup_close();
static void wifi_evt_cancel(lv_event_t* e);
static void wifi_evt_connect(lv_event_t* e);
static void wifi_evt_pass_focus(lv_event_t* e);
static void wifi_evt_kb(lv_event_t* e);
static void ui_wifi_set_status(const char* text, bool connected);

static lv_timer_t* s_wifi_conn_timer = nullptr;
static uint32_t    s_wifi_conn_start = 0;
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;

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

static void bt_delete_connecttable() {
    // delete the auto connect table
    if (a2dp.isConnected())
    a2dp.disconnect();

    a2dp.erase_autoreconnect_table();
}

static void bt_connectdevice() {
    // delete the auto connect table
    uint16_t sel = lv_dropdown_get_selected(objects.bt_devicelist);

    Serial.printf("[BT] Connecting to device at index %d\r\n", sel);

    a2dp.connect_by_index((int)sel);    
}

// wifi section
void ui_wifi_scan_and_populate_dropdown() {
  Serial.println("[WIFI] 🔍 Scanning for networks...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
  if (n <= 0) {
    Serial.println("[WIFI] ❌ No networks found");
    lv_dropdown_set_options(objects.wifi_dd_apnlist, "No networks found");
    lv_dropdown_set_selected(objects.wifi_dd_apnlist, 0);
    return;
  }

  static char options[1024];
  options[0] = '\0';

  int count = 0;
  for (int i = 0; i < n && count < 20; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0)
      continue;

    if (count > 0)
      strcat(options, "\n");

    strncat(options,
            ssid.c_str(),
            sizeof(options) - strlen(options) - 1);

    count++;
  }

  WiFi.scanDelete();

  lv_dropdown_set_options(objects.wifi_dd_apnlist, options);
  lv_dropdown_set_selected(objects.wifi_dd_apnlist, 0);

  Serial.printf("[WIFI] ✅ %d networks added to dropdown\n", count);
}

// --------------------------------------------------
// Show password popup (password box only + keyboard)
// --------------------------------------------------
void ui_wifi_show_password_popup() {
  // Get selected SSID from dropdown
  if (!objects.wifi_dd_apnlist) return;

  lv_dropdown_get_selected_str(objects.wifi_dd_apnlist,
                               s_wifi_ssid,
                               sizeof(s_wifi_ssid));

  if (s_wifi_ssid[0] == '\0' ||
      strcmp(s_wifi_ssid, "No networks found") == 0) {
    Serial.println("[WIFI] ❌ No valid SSID selected");
    return;
  }

  wifi_popup_close();

  // ---- Popup ----
  s_wifi_popup = lv_obj_create(lv_layer_top());
  lv_obj_set_size(s_wifi_popup, 300, 160);
  lv_obj_center(s_wifi_popup);
  lv_obj_set_style_pad_all(s_wifi_popup, 12, 0);
  lv_obj_set_style_radius(s_wifi_popup, 12, 0);

  // ---- Password textbox ----
  s_wifi_pass_ta = lv_textarea_create(s_wifi_popup);
  lv_obj_set_width(s_wifi_pass_ta, lv_pct(100));
  lv_textarea_set_placeholder_text(s_wifi_pass_ta, "Wi-Fi password");
  lv_textarea_set_password_mode(s_wifi_pass_ta, true);
  lv_textarea_set_one_line(s_wifi_pass_ta, true);
  lv_obj_align(s_wifi_pass_ta, LV_ALIGN_TOP_MID, 0, 0);

  // Show/hide keyboard on focus changes
  lv_obj_add_event_cb(s_wifi_pass_ta, wifi_evt_pass_focus, LV_EVENT_FOCUSED, nullptr);
  lv_obj_add_event_cb(s_wifi_pass_ta, wifi_evt_pass_focus, LV_EVENT_DEFOCUSED, nullptr);

  // ---- Buttons row ----
  lv_obj_t* row = lv_obj_create(s_wifi_popup);
  lv_obj_set_size(row, lv_pct(100), 44);
  lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_layout(row, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row,
                        LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  // Cancel
  lv_obj_t* btn_cancel = lv_btn_create(row);
  lv_obj_set_size(btn_cancel, 130, 40);
  lv_obj_add_event_cb(btn_cancel, wifi_evt_cancel, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* lbl_cancel = lv_label_create(btn_cancel);
  lv_label_set_text(lbl_cancel, "Cancel");
  lv_obj_center(lbl_cancel);

  // Connect
  lv_obj_t* btn_connect = lv_btn_create(row);
  lv_obj_set_size(btn_connect, 130, 40);
  lv_obj_add_event_cb(btn_connect, wifi_evt_connect, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* lbl_connect = lv_label_create(btn_connect);
  lv_label_set_text(lbl_connect, "Connect");
  lv_obj_center(lbl_connect);

  // ---- Keyboard (hidden until focus) ----
  s_wifi_kb = lv_keyboard_create(lv_layer_top());
  lv_obj_set_size(s_wifi_kb, lv_pct(100), 140);
  lv_obj_align(s_wifi_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_flag(s_wifi_kb, LV_OBJ_FLAG_HIDDEN);

  // Keyboard events: OK / Cancel buttons on the kb
  lv_obj_add_event_cb(s_wifi_kb, wifi_evt_kb, LV_EVENT_ALL, nullptr);

  // Attach kb to textarea
  lv_keyboard_set_textarea(s_wifi_kb, s_wifi_pass_ta);

  // Focus password box so keyboard appears immediately
  lv_group_t* g = lv_group_get_default();
  if (g) lv_group_focus_obj(s_wifi_pass_ta);
  lv_obj_add_state(s_wifi_pass_ta, LV_STATE_FOCUSED);

  Serial.printf("[WIFI] Password popup opened for '%s'\n", s_wifi_ssid);
}

// --------------------------------------------------
// Textarea focus handler: show/hide keyboard
// --------------------------------------------------
static void wifi_evt_pass_focus(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (!s_wifi_kb) return;

  if (code == LV_EVENT_FOCUSED) {
    lv_keyboard_set_textarea(s_wifi_kb, s_wifi_pass_ta);
    lv_obj_clear_flag(s_wifi_kb, LV_OBJ_FLAG_HIDDEN);
  }
  else if (code == LV_EVENT_DEFOCUSED) {
    // Hide kb when leaving field (optional but typical)
    lv_obj_add_flag(s_wifi_kb, LV_OBJ_FLAG_HIDDEN);
  }
}

// --------------------------------------------------
// Keyboard handler: OK/Cancel on keyboard itself
// --------------------------------------------------
static void wifi_evt_kb(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (!s_wifi_kb) return;

  if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
    lv_obj_add_flag(s_wifi_kb, LV_OBJ_FLAG_HIDDEN);
  }
}

// --------------------------------------------------
// Button events
// --------------------------------------------------
static void wifi_evt_cancel(lv_event_t* e) {
  (void)e;
  wifi_popup_close();
}

static void wifi_evt_connect(lv_event_t* e) {
    (void)e;

    const char* pass = (s_wifi_pass_ta) ? lv_textarea_get_text(s_wifi_pass_ta) : "";
    if (!pass) pass = "";

    ui_wifi_set_status("Connecting...", false);

    Serial.printf("[WIFI] 🔗 Connecting to '%s'\n", s_wifi_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);
    delay(50);
    WiFi.begin(s_wifi_ssid, pass);

    // Close popup while we try
    wifi_popup_close();

    const uint32_t start = millis();
    while (millis() - start < 15000) {
        wl_status_t st = WiFi.status();

        if (st == WL_CONNECTED) {
            ui_wifi_set_status("Connected", true);
            Serial.printf("[WIFI] ✅ Connected, IP=%s\n", WiFi.localIP().toString().c_str());
            return;
        }

        if (st == WL_CONNECT_FAILED) {
            break; // immediate failure
        }

        // Keep UI responsive during wait
        lv_timer_handler();
        delay(10);
    }

    // Timed out / failed
    ui_wifi_set_status("Disconnected", false);
    Serial.println("[WIFI] ❌ Connect failed/timeout");

    // Show password popup again
    ui_wifi_show_password_popup();
}

static void ui_wifi_set_status(const char* text, bool connected) {
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

// --------------------------------------------------
// Destroy popup + keyboard
// --------------------------------------------------
static void wifi_popup_close() {
  if (s_wifi_popup) {
    lv_obj_del(s_wifi_popup);
    s_wifi_popup = nullptr;
    s_wifi_pass_ta = nullptr;
  }
  if (s_wifi_kb) {
    lv_obj_del(s_wifi_kb);
    s_wifi_kb = nullptr;
  }
}









// -----------------------------------------------------------------------------
// action_back: Assigned in EEZ Studio for button actions
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

void action_bt_delete_connecttable(lv_event_t * e) {
    LV_UNUSED(e);   // avoid compiler warnings
    bt_delete_connecttable();
}

void action_bt_connectdevice(lv_event_t * e) {
    LV_UNUSED(e);   // avoid compiler warnings
    bt_connectdevice();
}

void action_player_play_pause(lv_event_t * e) {

    LV_UNUSED(e);   // avoid compiler warnings

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
    LV_UNUSED(e);   // avoid compiler warnings

    AudioPlayer_Prev();
}

void action_player_next(lv_event_t * e) {
    LV_UNUSED(e);   // avoid compiler warnings

    AudioPlayer_Next();
}

// Blueooth View actions
void action_bluetooth_startscan(lv_event_t * e) {

    LV_UNUSED(e);   // avoid compiler warnings

    Serial.printf("[UI] isScanning=%d blocked=%d isConnected=%d\n", a2dp.isScanning(), a2dp.scan_blocked(), a2dp.isConnected());

    if (a2dp.isScanning()) {
        a2dp.stop_scan();
    }
    
    a2dp.start_scan(10);
}

// Wifi view actions
void action_wifi_connectapn(lv_event_t * e) {

    LV_UNUSED(e);   // avoid compiler warnings
    ui_wifi_show_password_popup();

}

void action_wifi_scan_networks(lv_event_t * e) {

    LV_UNUSED(e);   // avoid compiler warnings
    ui_wifi_scan_and_populate_dropdown();

}




