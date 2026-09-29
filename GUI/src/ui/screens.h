#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN_SCREEN = 1,
    SCREEN_ID_QR_READ = 2,
    SCREEN_ID_RFID_READ = 3,
    SCREEN_ID_ACCESS_REJECTED = 4,
    SCREEN_ID_ACCESS_ACCEPTED = 5,
    SCREEN_ID_ADMIN_PASSWORD = 6,
    SCREEN_ID_ADMIN_USERS = 7,
    SCREEN_ID_ADMIN_HISTORY = 8,
    SCREEN_ID_ADMIN_SETTINGS = 9,
    SCREEN_ID_ADMIN_TEST = 10,
    _SCREEN_ID_LAST = 10
};

typedef struct _objects_t {
    lv_obj_t *main_screen;
    lv_obj_t *qr_read;
    lv_obj_t *rfid_read;
    lv_obj_t *access_rejected;
    lv_obj_t *access_accepted;
    lv_obj_t *admin_password;
    lv_obj_t *admin_users;
    lv_obj_t *admin_history;
    lv_obj_t *admin_settings;
    lv_obj_t *admin_test;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *spinner_loading_1;
    lv_obj_t *obj4;
    lv_obj_t *spinner_loading;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *label_rej_reason;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *label_acc_name;
    lv_obj_t *bar_door;
    lv_obj_t *obj12;
    lv_obj_t *textarea_input_password;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *obj23;
    lv_obj_t *btn_users_add;
    lv_obj_t *obj24;
    lv_obj_t *btn_users_edit;
    lv_obj_t *btn_users_del;
    lv_obj_t *obj25;
    lv_obj_t *obj26;
    lv_obj_t *obj27;
    lv_obj_t *obj28;
    lv_obj_t *lbl_user1_uid;
    lv_obj_t *lbl_user1_name;
    lv_obj_t *lbl_user1_role;
    lv_obj_t *lbl_user2_uid;
    lv_obj_t *lbl_user2_name;
    lv_obj_t *lbl_user2_role;
    lv_obj_t *obj29;
    lv_obj_t *obj30;
    lv_obj_t *obj31;
    lv_obj_t *obj32;
    lv_obj_t *obj33;
    lv_obj_t *obj34;
    lv_obj_t *obj35;
    lv_obj_t *obj36;
    lv_obj_t *obj37;
    lv_obj_t *obj38;
    lv_obj_t *obj39;
    lv_obj_t *obj40;
    lv_obj_t *obj41;
    lv_obj_t *obj42;
    lv_obj_t *lbl_hist1_time;
    lv_obj_t *lbl_hist1_name;
    lv_obj_t *lbl_hist1_method;
    lv_obj_t *lbl_hist2_time;
    lv_obj_t *lbl_hist2_name;
    lv_obj_t *lbl_hist2_method;
    lv_obj_t *obj43;
    lv_obj_t *obj44;
    lv_obj_t *obj45;
    lv_obj_t *obj46;
    lv_obj_t *obj47;
    lv_obj_t *obj48;
    lv_obj_t *obj49;
    lv_obj_t *obj50;
    lv_obj_t *obj51;
    lv_obj_t *obj52;
    lv_obj_t *obj53;
    lv_obj_t *obj54;
    lv_obj_t *inp_ssid;
    lv_obj_t *obj55;
    lv_obj_t *inp_pass;
    lv_obj_t *obj56;
    lv_obj_t *inp_ip;
    lv_obj_t *obj57;
    lv_obj_t *inp_ntp;
    lv_obj_t *obj58;
    lv_obj_t *slider_relay;
    lv_obj_t *lbl_relay_val;
    lv_obj_t *btn_settings_save;
    lv_obj_t *obj59;
    lv_obj_t *obj60;
    lv_obj_t *obj61;
    lv_obj_t *obj62;
    lv_obj_t *obj63;
    lv_obj_t *obj64;
    lv_obj_t *obj65;
    lv_obj_t *obj66;
    lv_obj_t *obj67;
    lv_obj_t *obj68;
    lv_obj_t *obj69;
    lv_obj_t *obj70;
    lv_obj_t *btn_test_relay;
    lv_obj_t *lbl_test_relay;
    lv_obj_t *btn_test_camera;
    lv_obj_t *lbl_test_camera;
    lv_obj_t *btn_test_rfid;
    lv_obj_t *lbl_test_rfid;
} objects_t;

extern objects_t objects;

void create_screen_main_screen();
void tick_screen_main_screen();

void create_screen_qr_read();
void tick_screen_qr_read();

void create_screen_rfid_read();
void tick_screen_rfid_read();

void create_screen_access_rejected();
void tick_screen_access_rejected();

void create_screen_access_accepted();
void tick_screen_access_accepted();

void create_screen_admin_password();
void tick_screen_admin_password();

void create_screen_admin_users();
void tick_screen_admin_users();

void create_screen_admin_history();
void tick_screen_admin_history();

void create_screen_admin_settings();
void tick_screen_admin_settings();

void create_screen_admin_test();
void tick_screen_admin_test();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

// Color themes

enum Themes {
    THEME_ID_DEFAULT,
    THEME_ID_DARK,
};
enum Colors {
    COLOR_ID_BG_DESK,
    COLOR_ID_BG_BODY,
    COLOR_ID_BG_CARD,
    COLOR_ID_BG_CARD_HOVER,
    COLOR_ID_BG_BAR,
    COLOR_ID_BORDER_COLOR,
    COLOR_ID_TEXT_PRIMARY,
    COLOR_ID_TEXT_SECONDARY,
    COLOR_ID_TEXT_MUTED,
    COLOR_ID_ACCENT_PRIMARY,
    COLOR_ID_ACCENT_SOFT,
    COLOR_ID_SOLAR,
    COLOR_ID_SOLAR_SOFT,
    COLOR_ID_INFO,
    COLOR_ID_DANGER,
    COLOR_ID_SUCCESS,
    COLOR_ID_SLATE_NEUTRAL,
    COLOR_ID_TANK_FRESH_DARK,
    COLOR_ID_TANK_FRESH_LIGHT,
    COLOR_ID_TANK_GREY_DARK,
    COLOR_ID_TANK_GREY_LIGHT,
    COLOR_ID_TANK_BLACK_DARK,
    COLOR_ID_TANK_BLACK_LIGHT,
    COLOR_ID_GREY_WATER,
    COLOR_ID_BLACK_WATER,
    COLOR_ID_GRID_LINE,
    COLOR_ID_FOREGROUND_WHITE,
    COLOR_ID_FOREGROUND_BLACK,
};
void change_color_theme(uint32_t themeIndex);
extern uint32_t theme_colors[2][28];
extern uint32_t active_theme_index;

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/