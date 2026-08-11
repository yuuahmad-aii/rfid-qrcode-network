#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_ADMIN_PASSWORD = 2,
    SCREEN_ID_ADMIN_PANEL = 3,
    SCREEN_ID_QR_READ = 4,
    SCREEN_ID_RFID_READ = 5,
    SCREEN_ID_ACCESS_REJECTED = 6,
    SCREEN_ID_ACCESS_ACCEPTED = 7,
    _SCREEN_ID_LAST = 7
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *admin_password;
    lv_obj_t *admin_panel;
    lv_obj_t *qr_read;
    lv_obj_t *rfid_read;
    lv_obj_t *access_rejected;
    lv_obj_t *access_accepted;
    lv_obj_t *tombol_admin;
    lv_obj_t *button_matrix_password;
    lv_obj_t *textarea_input_password;
    lv_obj_t *spinner_loading_1;
    lv_obj_t *id_qrcode;
    lv_obj_t *rfid_terdeteksi;
    lv_obj_t *spinner_loading;
    lv_obj_t *id_rfid;
    lv_obj_t *error_reason;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void create_screen_admin_password();
void tick_screen_admin_password();

void create_screen_admin_panel();
void tick_screen_admin_panel();

void create_screen_qr_read();
void tick_screen_qr_read();

void create_screen_rfid_read();
void tick_screen_rfid_read();

void create_screen_access_rejected();
void tick_screen_access_rejected();

void create_screen_access_accepted();
void tick_screen_access_accepted();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/