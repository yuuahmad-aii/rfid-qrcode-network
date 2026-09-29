#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_btn_admin_on_pressed(lv_event_t * e);
extern void action_button_matrix_password_pressed(lv_event_t * e);
extern void action_pwd_btn_1_pressed(lv_event_t * e);
extern void action_pwd_btn_2_pressed(lv_event_t * e);
extern void action_pwd_btn_3_pressed(lv_event_t * e);
extern void action_pwd_btn_4_pressed(lv_event_t * e);
extern void action_pwd_btn_5_pressed(lv_event_t * e);
extern void action_pwd_btn_6_pressed(lv_event_t * e);
extern void action_pwd_btn_7_pressed(lv_event_t * e);
extern void action_pwd_btn_8_pressed(lv_event_t * e);
extern void action_pwd_btn_9_pressed(lv_event_t * e);
extern void action_pwd_btn_0_pressed(lv_event_t * e);
extern void action_pwd_btn_del_pressed(lv_event_t * e);
extern void action_pwd_btn_ok_pressed(lv_event_t * e);
extern void action_load_admin_users(lv_event_t * e);
extern void action_load_admin_history(lv_event_t * e);
extern void action_load_admin_settings(lv_event_t * e);
extern void action_load_admin_test(lv_event_t * e);
extern void action_load_main(lv_event_t * e);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/