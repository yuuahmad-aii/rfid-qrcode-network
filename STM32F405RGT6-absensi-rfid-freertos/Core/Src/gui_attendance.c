/**
  ******************************************************************************
  * @file    gui_attendance.c
  * @brief   LVGL Modern GUI Implementation for RFID Attendance & Admin Settings
  ******************************************************************************
  */

#include "gui_attendance.h"
#include "lvgl.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

/* Eksternal variabel dari main.c */
extern RTC_HandleTypeDef hrtc;
extern TIM_HandleTypeDef htim3;
extern osMessageQueueId_t espTxQueueHandle;
extern volatile uint32_t g_attendance_total;

/* Master PIN untuk akses menu Admin */
#define ADMIN_MASTER_PIN        "1234"

/* Palet Warna Tema Gelap (Default) */
#define COLOR_DARK_BG           0x0B1120  /* Deep Obsidian / Dark Slate */
#define COLOR_DARK_CARD         0x1E293B  /* Slate Card Container */
#define COLOR_DARK_SUB_CARD     0x0F172A  /* Inner Dark Frame */
#define COLOR_DARK_BORDER       0x334155  /* Border Slate */
#define COLOR_DARK_TEXT_PRIMARY 0xF8FAFC  /* Crisp White */
#define COLOR_DARK_TEXT_MUTED   0x94A3B8  /* Slate Muted Text */

/* Palet Warna Tema Terang */
#define COLOR_LIGHT_BG          0xF1F5F9  /* Light Slate Background */
#define COLOR_LIGHT_CARD        0xFFFFFF  /* Clean White Container */
#define COLOR_LIGHT_SUB_CARD    0xE2E8F0  /* Inner Soft Frame */
#define COLOR_LIGHT_BORDER      0xCBD5E1  /* Border Light Slate */
#define COLOR_LIGHT_TEXT_PRIMARY 0x0F172A /* Deep Navy Text */
#define COLOR_LIGHT_TEXT_MUTED  0x64748B  /* Muted Grey Text */

/* Warna Aksen Universal */
#define COLOR_CYAN_ACCENT       0x0284C7  /* Glowing Electric Blue */
#define COLOR_CYAN_TEXT         0x38BDF8  /* Crisp Cyan Text */
#define COLOR_GREEN_STATUS      0x10B981  /* Emerald Green Status */
#define COLOR_RED_STATUS        0xEF4444  /* Coral Red Error */
#define COLOR_AMBER_STATUS      0xF59E0B  /* Warm Amber Highlights / Clock */

/* Status Tema */
static bool g_theme_is_dark = true;

/* Objek Layar LVGL */
static lv_obj_t * screen_main     = NULL;
static lv_obj_t * screen_password = NULL;
static lv_obj_t * screen_admin    = NULL;

/* Widgets Layar Utama (screen_main) */
static lv_obj_t * header_main         = NULL;
static lv_obj_t * lbl_header_title    = NULL;
static lv_obj_t * badge_rfid          = NULL;
static lv_obj_t * lbl_badge_rfid      = NULL;
static lv_obj_t * badge_wifi          = NULL;
static lv_obj_t * lbl_badge_wifi      = NULL;
static lv_obj_t * badge_sd            = NULL;
static lv_obj_t * lbl_badge_sd        = NULL;
static lv_obj_t * lbl_clock_main      = NULL;

static lv_obj_t * card_hero           = NULL;
static lv_obj_t * lbl_hero_title      = NULL;
static lv_obj_t * tap_zone            = NULL;
static lv_obj_t * lbl_tap_icon        = NULL;
static lv_obj_t * lbl_tap_desc        = NULL;
static lv_obj_t * box_uid             = NULL;
static lv_obj_t * lbl_uid             = NULL;
static lv_obj_t * box_status          = NULL;
static lv_obj_t * lbl_status          = NULL;

static lv_obj_t * card_summary        = NULL;
static lv_obj_t * lbl_sum_title       = NULL;
static lv_obj_t * lbl_total_title     = NULL;
static lv_obj_t * lbl_total_val       = NULL;
static lv_obj_t * lbl_total_unit      = NULL;
static lv_obj_t * lbl_date_title      = NULL;
static lv_obj_t * lbl_date_val        = NULL;
static lv_obj_t * lbl_last_title      = NULL;
static lv_obj_t * lbl_last_val        = NULL;
static lv_obj_t * corner_hint_dot     = NULL;

/* Widgets Layar Password (screen_password) */
static lv_obj_t * box_pin_container   = NULL;
static lv_obj_t * lbl_pin_title       = NULL;
static lv_obj_t * lbl_pin_subtitle    = NULL;
static lv_obj_t * box_pin_display     = NULL;
static lv_obj_t * lbl_pin_display     = NULL;
static lv_obj_t * lbl_pin_feedback    = NULL;
static char       g_entered_pin[16]   = {0};
static uint8_t    g_pin_len           = 0;

/* Widgets Layar Admin (screen_admin) */
static lv_obj_t * header_admin        = NULL;
static lv_obj_t * btn_admin_back      = NULL;
static lv_obj_t * lbl_admin_back      = NULL;
static lv_obj_t * lbl_admin_title     = NULL;
static lv_obj_t * lbl_admin_clock     = NULL;
static lv_obj_t * tabview_admin       = NULL;

/* Tab 1: Tampilan & Sistem */
static lv_obj_t * slider_brightness   = NULL;
static lv_obj_t * lbl_bright_val      = NULL;
static lv_obj_t * sw_theme            = NULL;
static lv_obj_t * lbl_theme_desc      = NULL;
static lv_obj_t * btn_test_beep       = NULL;
static lv_obj_t * btn_reset_stats     = NULL;
static lv_obj_t * btn_ntp_sync        = NULL;
static lv_obj_t * lbl_admin_feedback  = NULL;

/* Tab 2: Wi-Fi Setup */
static lv_obj_t * ta_wifi_ssid        = NULL;
static lv_obj_t * ta_wifi_pass        = NULL;
static lv_obj_t * btn_save_wifi       = NULL;
static lv_obj_t * kb_wifi             = NULL;

/* Timer */
static lv_timer_t * revert_timer      = NULL;
static lv_timer_t * pwd_err_timer     = NULL;

/* Variabel Deteksi Sentuh Pojok Layar (3 Detik) */
static uint32_t corner_touch_start    = 0;
static bool     corner_touching       = false;

/* Forward Callbacks */
static void build_screen_main(void);
static void build_screen_password(void);
static void build_screen_admin(void);
static void apply_theme(bool is_dark);
static void revert_standby_cb(lv_timer_t * t);
static void pwd_err_timeout_cb(lv_timer_t * t);

static void btn_keypad_click_cb(lv_event_t * e);
static void slider_brightness_cb(lv_event_t * e);
static void sw_theme_cb(lv_event_t * e);
static void btn_admin_back_cb(lv_event_t * e);
static void btn_test_beep_cb(lv_event_t * e);
static void btn_reset_stats_cb(lv_event_t * e);
static void btn_ntp_sync_cb(lv_event_t * e);
static void btn_save_wifi_cb(lv_event_t * e);
static void ta_wifi_focus_cb(lv_event_t * e);

/**
  * @brief  Inisialisasi antarmuka pengguna grafis LVGL
  */
void GUI_Attendance_Init(void)
{
    /* Bangun ketiga layar antarmuka */
    build_screen_main();
    build_screen_password();
    build_screen_admin();

    /* Muat Layar Utama sebagai tampilan default */
    lv_screen_load(screen_main);
}

/**
  * @brief  Membangun Layar Utama (Layar Absensi Bersih & Modern)
  */
static void build_screen_main(void)
{
    screen_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(COLOR_DARK_BG), 0);
    lv_obj_set_style_bg_opa(screen_main, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen_main, LV_OBJ_FLAG_SCROLLABLE);

    /* 1. HEADER BAR (y: 0, h: 38, w: 480) */
    header_main = lv_obj_create(screen_main);
    lv_obj_set_pos(header_main, 0, 0);
    lv_obj_set_size(header_main, 480, 38);
    lv_obj_set_style_bg_color(header_main, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_opa(header_main, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(header_main, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(header_main, 2, 0);
    lv_obj_set_style_border_side(header_main, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(header_main, 0, 0);
    lv_obj_set_style_pad_all(header_main, 4, 0);
    lv_obj_remove_flag(header_main, LV_OBJ_FLAG_SCROLLABLE);

    /* Judul Sistem */
    lbl_header_title = lv_label_create(header_main);
    lv_obj_set_pos(lbl_header_title, 10, 4);
    lv_label_set_text(lbl_header_title, "ABSENSI RFID");
    lv_obj_set_style_text_color(lbl_header_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_header_title, &lv_font_montserrat_14, 0);

    /* Badge Status RFID */
    badge_rfid = lv_obj_create(header_main);
    lv_obj_set_pos(badge_rfid, 138, 2);
    lv_obj_set_size(badge_rfid, 75, 24);
    lv_obj_set_style_bg_color(badge_rfid, lv_color_hex(0x064E3B), 0);
    lv_obj_set_style_border_color(badge_rfid, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_border_width(badge_rfid, 1, 0);
    lv_obj_set_style_radius(badge_rfid, 12, 0);
    lv_obj_set_style_pad_all(badge_rfid, 0, 0);
    lv_obj_remove_flag(badge_rfid, LV_OBJ_FLAG_SCROLLABLE);

    lbl_badge_rfid = lv_label_create(badge_rfid);
    lv_obj_center(lbl_badge_rfid);
    lv_label_set_text(lbl_badge_rfid, "RFID: OK");
    lv_obj_set_style_text_color(lbl_badge_rfid, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_text_font(lbl_badge_rfid, &lv_font_montserrat_14, 0);

    /* Badge Status Wi-Fi */
    badge_wifi = lv_obj_create(header_main);
    lv_obj_set_pos(badge_wifi, 220, 2);
    lv_obj_set_size(badge_wifi, 75, 24);
    lv_obj_set_style_bg_color(badge_wifi, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_color(badge_wifi, lv_color_hex(COLOR_DARK_BORDER), 0);
    lv_obj_set_style_border_width(badge_wifi, 1, 0);
    lv_obj_set_style_radius(badge_wifi, 12, 0);
    lv_obj_set_style_pad_all(badge_wifi, 0, 0);
    lv_obj_remove_flag(badge_wifi, LV_OBJ_FLAG_SCROLLABLE);

    lbl_badge_wifi = lv_label_create(badge_wifi);
    lv_obj_center(lbl_badge_wifi);
    lv_label_set_text(lbl_badge_wifi, "WIFI: --");
    lv_obj_set_style_text_color(lbl_badge_wifi, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_badge_wifi, &lv_font_montserrat_14, 0);

    /* Badge Status SD Card */
    badge_sd = lv_obj_create(header_main);
    lv_obj_set_pos(badge_sd, 302, 2);
    lv_obj_set_size(badge_sd, 70, 24);
    lv_obj_set_style_bg_color(badge_sd, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_color(badge_sd, lv_color_hex(COLOR_DARK_BORDER), 0);
    lv_obj_set_style_border_width(badge_sd, 1, 0);
    lv_obj_set_style_radius(badge_sd, 12, 0);
    lv_obj_set_style_pad_all(badge_sd, 0, 0);
    lv_obj_remove_flag(badge_sd, LV_OBJ_FLAG_SCROLLABLE);

    lbl_badge_sd = lv_label_create(badge_sd);
    lv_obj_center(lbl_badge_sd);
    lv_label_set_text(lbl_badge_sd, "SD: --");
    lv_obj_set_style_text_color(lbl_badge_sd, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_badge_sd, &lv_font_montserrat_14, 0);

    /* Jam Digital Header */
    lbl_clock_main = lv_label_create(header_main);
    lv_obj_set_pos(lbl_clock_main, 388, 3);
    lv_label_set_text(lbl_clock_main, "00:00:00");
    lv_obj_set_style_text_color(lbl_clock_main, lv_color_hex(COLOR_AMBER_STATUS), 0);
    lv_obj_set_style_text_font(lbl_clock_main, &lv_font_montserrat_16, 0);

    /* 2. CARD TAP HERO (Kiri: x: 12, y: 48, w: 295, h: 260) */
    card_hero = lv_obj_create(screen_main);
    lv_obj_set_pos(card_hero, 12, 48);
    lv_obj_set_size(card_hero, 295, 260);
    lv_obj_set_style_bg_color(card_hero, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(card_hero, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(card_hero, 2, 0);
    lv_obj_set_style_radius(card_hero, 12, 0);
    lv_obj_set_style_pad_all(card_hero, 8, 0);
    lv_obj_remove_flag(card_hero, LV_OBJ_FLAG_SCROLLABLE);

    lbl_hero_title = lv_label_create(card_hero);
    lv_obj_set_pos(lbl_hero_title, 0, 4);
    lv_obj_set_width(lbl_hero_title, 275);
    lv_label_set_text(lbl_hero_title, "SILAKAN TAP KARTU");
    lv_obj_set_style_text_align(lbl_hero_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_hero_title, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_hero_title, &lv_font_montserrat_16, 0);

    /* Kotak Sensor Visual Tap */
    tap_zone = lv_obj_create(card_hero);
    lv_obj_set_pos(tap_zone, 8, 32);
    lv_obj_set_size(tap_zone, 260, 95);
    lv_obj_set_style_bg_color(tap_zone, lv_color_hex(COLOR_DARK_SUB_CARD), 0);
    lv_obj_set_style_border_color(tap_zone, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(tap_zone, 1, 0);
    lv_obj_set_style_radius(tap_zone, 10, 0);
    lv_obj_set_style_pad_all(tap_zone, 8, 0);
    lv_obj_remove_flag(tap_zone, LV_OBJ_FLAG_SCROLLABLE);

    lbl_tap_icon = lv_label_create(tap_zone);
    lv_obj_set_pos(lbl_tap_icon, 0, 10);
    lv_obj_set_width(lbl_tap_icon, 244);
    lv_label_set_text(lbl_tap_icon, "((( RFID TAP )))");
    lv_obj_set_style_text_align(lbl_tap_icon, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_tap_icon, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_text_font(lbl_tap_icon, &lv_font_montserrat_22, 0);

    lbl_tap_desc = lv_label_create(tap_zone);
    lv_obj_set_pos(lbl_tap_desc, 0, 52);
    lv_obj_set_width(lbl_tap_desc, 244);
    lv_label_set_text(lbl_tap_desc, "Dekatkan kartu absensi Anda ke sensor");
    lv_obj_set_style_text_align(lbl_tap_desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_tap_desc, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_tap_desc, &lv_font_montserrat_14, 0);

    /* Kotak UID */
    box_uid = lv_obj_create(card_hero);
    lv_obj_set_pos(box_uid, 8, 136);
    lv_obj_set_size(box_uid, 260, 44);
    lv_obj_set_style_bg_color(box_uid, lv_color_hex(0x060E1A), 0);
    lv_obj_set_style_border_color(box_uid, lv_color_hex(COLOR_DARK_BORDER), 0);
    lv_obj_set_style_border_width(box_uid, 1, 0);
    lv_obj_set_style_radius(box_uid, 8, 0);
    lv_obj_set_style_pad_all(box_uid, 4, 0);
    lv_obj_remove_flag(box_uid, LV_OBJ_FLAG_SCROLLABLE);

    lbl_uid = lv_label_create(box_uid);
    lv_obj_center(lbl_uid);
    lv_label_set_text(lbl_uid, "UID: -- -- -- --");
    lv_obj_set_style_text_color(lbl_uid, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_uid, &lv_font_montserrat_16, 0);

    /* Kotak Status Absensi */
    box_status = lv_obj_create(card_hero);
    lv_obj_set_pos(box_status, 8, 188);
    lv_obj_set_size(box_status, 260, 50);
    lv_obj_set_style_bg_color(box_status, lv_color_hex(0x060E1A), 0);
    lv_obj_set_style_border_color(box_status, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(box_status, 1, 0);
    lv_obj_set_style_radius(box_status, 8, 0);
    lv_obj_set_style_pad_all(box_status, 4, 0);
    lv_obj_remove_flag(box_status, LV_OBJ_FLAG_SCROLLABLE);

    lbl_status = lv_label_create(box_status);
    lv_obj_center(lbl_status);
    lv_label_set_text(lbl_status, "Siap Merekam Kehadiran Anda");
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);

    /* 3. CARD SUMMARY INFO (Kanan: x: 317, y: 48, w: 151, h: 260) */
    card_summary = lv_obj_create(screen_main);
    lv_obj_set_pos(card_summary, 317, 48);
    lv_obj_set_size(card_summary, 151, 260);
    lv_obj_set_style_bg_color(card_summary, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(card_summary, lv_color_hex(COLOR_DARK_BORDER), 0);
    lv_obj_set_style_border_width(card_summary, 2, 0);
    lv_obj_set_style_radius(card_summary, 12, 0);
    lv_obj_set_style_pad_all(card_summary, 6, 0);
    lv_obj_remove_flag(card_summary, LV_OBJ_FLAG_SCROLLABLE);

    lbl_sum_title = lv_label_create(card_summary);
    lv_obj_set_pos(lbl_sum_title, 0, 4);
    lv_obj_set_width(lbl_sum_title, 135);
    lv_label_set_text(lbl_sum_title, "RINGKASAN");
    lv_obj_set_style_text_align(lbl_sum_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_sum_title, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_sum_title, &lv_font_montserrat_14, 0);

    /* Total Hadir */
    lbl_total_title = lv_label_create(card_summary);
    lv_obj_set_pos(lbl_total_title, 0, 32);
    lv_obj_set_width(lbl_total_title, 135);
    lv_label_set_text(lbl_total_title, "TOTAL HADIR");
    lv_obj_set_style_text_align(lbl_total_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_total_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_total_title, &lv_font_montserrat_14, 0);

    lbl_total_val = lv_label_create(card_summary);
    lv_obj_set_pos(lbl_total_val, 0, 56);
    lv_obj_set_width(lbl_total_val, 135);
    lv_label_set_text(lbl_total_val, "0");
    lv_obj_set_style_text_align(lbl_total_val, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_total_val, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_total_val, &lv_font_montserrat_22, 0);

    lbl_total_unit = lv_label_create(card_summary);
    lv_obj_set_pos(lbl_total_unit, 0, 88);
    lv_obj_set_width(lbl_total_unit, 135);
    lv_label_set_text(lbl_total_unit, "Orang");
    lv_obj_set_style_text_align(lbl_total_unit, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_total_unit, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_total_unit, &lv_font_montserrat_14, 0);

    /* Tanggal */
    lbl_date_title = lv_label_create(card_summary);
    lv_obj_set_pos(lbl_date_title, 0, 120);
    lv_obj_set_width(lbl_date_title, 135);
    lv_label_set_text(lbl_date_title, "TANGGAL");
    lv_obj_set_style_text_align(lbl_date_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_date_title, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_date_title, &lv_font_montserrat_14, 0);

    lbl_date_val = lv_label_create(card_summary);
    lv_obj_set_pos(lbl_date_val, 0, 140);
    lv_obj_set_width(lbl_date_val, 135);
    lv_label_set_text(lbl_date_val, "2026-10-17");
    lv_obj_set_style_text_align(lbl_date_val, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_date_val, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_date_val, &lv_font_montserrat_14, 0);

    /* Transaksi Terakhir */
    lbl_last_title = lv_label_create(card_summary);
    lv_obj_set_pos(lbl_last_title, 0, 172);
    lv_obj_set_width(lbl_last_title, 135);
    lv_label_set_text(lbl_last_title, "TERAKHIR");
    lv_obj_set_style_text_align(lbl_last_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_last_title, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_last_title, &lv_font_montserrat_14, 0);

    lbl_last_val = lv_label_create(card_summary);
    lv_obj_set_pos(lbl_last_val, 0, 194);
    lv_obj_set_width(lbl_last_val, 135);
    lv_label_set_text(lbl_last_val, "--:--:--");
    lv_obj_set_style_text_align(lbl_last_val, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_last_val, lv_color_hex(COLOR_AMBER_STATUS), 0);
    lv_obj_set_style_text_font(lbl_last_val, &lv_font_montserrat_14, 0);

    /* Subtle indicator dot di pojok kanan bawah (petunjuk sentuh admin) */
    corner_hint_dot = lv_obj_create(screen_main);
    lv_obj_set_pos(corner_hint_dot, 468, 308);
    lv_obj_set_size(corner_hint_dot, 8, 8);
    lv_obj_set_style_bg_color(corner_hint_dot, lv_color_hex(COLOR_DARK_BORDER), 0);
    lv_obj_set_style_border_width(corner_hint_dot, 0, 0);
    lv_obj_set_style_radius(corner_hint_dot, 4, 0);
    lv_obj_remove_flag(corner_hint_dot, LV_OBJ_FLAG_CLICKABLE);
}

/**
  * @brief  Membangun Layar Masukkan Password PIN Admin
  */
static void build_screen_password(void)
{
    screen_password = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_password, lv_color_hex(0x050811), 0);
    lv_obj_set_style_bg_opa(screen_password, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen_password, LV_OBJ_FLAG_SCROLLABLE);

    /* Kontainer Tengah PIN */
    box_pin_container = lv_obj_create(screen_password);
    lv_obj_set_size(box_pin_container, 380, 296);
    lv_obj_center(box_pin_container);
    lv_obj_set_style_bg_color(box_pin_container, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(box_pin_container, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(box_pin_container, 2, 0);
    lv_obj_set_style_radius(box_pin_container, 14, 0);
    lv_obj_set_style_pad_all(box_pin_container, 8, 0);
    lv_obj_remove_flag(box_pin_container, LV_OBJ_FLAG_SCROLLABLE);

    /* Judul Modal */
    lbl_pin_title = lv_label_create(box_pin_container);
    lv_obj_set_pos(lbl_pin_title, 0, 4);
    lv_obj_set_width(lbl_pin_title, 360);
    lv_label_set_text(lbl_pin_title, "MASUKKAN PIN ADMIN");
    lv_obj_set_style_text_align(lbl_pin_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_pin_title, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_pin_title, &lv_font_montserrat_16, 0);

    /* Subtitle */
    lbl_pin_subtitle = lv_label_create(box_pin_container);
    lv_obj_set_pos(lbl_pin_subtitle, 0, 26);
    lv_obj_set_width(lbl_pin_subtitle, 360);
    lv_label_set_text(lbl_pin_subtitle, "Verifikasi akses menu konfigurasi");
    lv_obj_set_style_text_align(lbl_pin_subtitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_pin_subtitle, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_pin_subtitle, &lv_font_montserrat_14, 0);

    /* Kotak Display Digit PIN (Masked) */
    box_pin_display = lv_obj_create(box_pin_container);
    lv_obj_set_pos(box_pin_display, 40, 48);
    lv_obj_set_size(box_pin_display, 280, 36);
    lv_obj_set_style_bg_color(box_pin_display, lv_color_hex(0x060E1A), 0);
    lv_obj_set_style_border_color(box_pin_display, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(box_pin_display, 1, 0);
    lv_obj_set_style_radius(box_pin_display, 8, 0);
    lv_obj_remove_flag(box_pin_display, LV_OBJ_FLAG_SCROLLABLE);

    lbl_pin_display = lv_label_create(box_pin_display);
    lv_obj_center(lbl_pin_display);
    lv_label_set_text(lbl_pin_display, "_ _ _ _");
    lv_obj_set_style_text_color(lbl_pin_display, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_pin_display, &lv_font_montserrat_20, 0);

    /* Label Feedback Error / Status */
    lbl_pin_feedback = lv_label_create(box_pin_container);
    lv_obj_set_pos(lbl_pin_feedback, 0, 88);
    lv_obj_set_width(lbl_pin_feedback, 360);
    lv_label_set_text(lbl_pin_feedback, "");
    lv_obj_set_style_text_align(lbl_pin_feedback, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_pin_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
    lv_obj_set_style_text_font(lbl_pin_feedback, &lv_font_montserrat_14, 0);

    /* Numeric Keypad Buttons (Grid 3 Kolom x 4 Baris) */
    const char * key_labels[12] = {
        "1", "2", "3",
        "4", "5", "6",
        "7", "8", "9",
        "BATAL", "0", "DEL"
    };

    uint16_t start_y = 110;
    uint8_t btn_w = 82;
    uint8_t btn_h = 36;
    uint8_t gap_x = 10;
    uint8_t gap_y = 6;
    uint8_t start_x = 48;

    for (int i = 0; i < 12; i++) {
        int row = i / 3;
        int col = i % 3;

        lv_obj_t * btn = lv_button_create(box_pin_container);
        lv_obj_set_pos(btn, start_x + col * (btn_w + gap_x), start_y + row * (btn_h + gap_y));
        lv_obj_set_size(btn, btn_w, btn_h);
        lv_obj_set_style_radius(btn, 8, 0);

        if (i == 9) {
            /* BATAL Button */
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x450A0A), 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(COLOR_RED_STATUS), 0);
        } else if (i == 11) {
            /* DEL Button */
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x451A03), 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(COLOR_AMBER_STATUS), 0);
        } else {
            /* Number Button */
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x0F172A), 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(COLOR_CYAN_ACCENT), 0);
        }
        lv_obj_set_style_border_width(btn, 1, 0);

        lv_obj_t * lbl = lv_label_create(btn);
        lv_obj_center(lbl);
        lv_label_set_text(lbl, key_labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);

        lv_obj_add_event_cb(btn, btn_keypad_click_cb, LV_EVENT_CLICKED, (void*)key_labels[i]);
    }
}

/**
  * @brief  Membangun Layar Pengaturan Admin (Tabview: Sistem & Wi-Fi)
  */
static void build_screen_admin(void)
{
    screen_admin = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_admin, lv_color_hex(COLOR_DARK_BG), 0);
    lv_obj_set_style_bg_opa(screen_admin, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen_admin, LV_OBJ_FLAG_SCROLLABLE);

    /* 1. Header Admin Bar (y: 0, h: 38, w: 480) */
    header_admin = lv_obj_create(screen_admin);
    lv_obj_set_pos(header_admin, 0, 0);
    lv_obj_set_size(header_admin, 480, 38);
    lv_obj_set_style_bg_color(header_admin, lv_color_hex(0x111827), 0);
    lv_obj_set_style_border_color(header_admin, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(header_admin, 2, 0);
    lv_obj_set_style_border_side(header_admin, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(header_admin, 0, 0);
    lv_obj_set_style_pad_all(header_admin, 3, 0);
    lv_obj_remove_flag(header_admin, LV_OBJ_FLAG_SCROLLABLE);

    /* Tombol Kembali ke Main Screen */
    btn_admin_back = lv_button_create(header_admin);
    lv_obj_set_pos(btn_admin_back, 6, 2);
    lv_obj_set_size(btn_admin_back, 100, 28);
    lv_obj_set_style_bg_color(btn_admin_back, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_admin_back, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_admin_back, 1, 0);
    lv_obj_set_style_radius(btn_admin_back, 6, 0);
    lv_obj_add_event_cb(btn_admin_back, btn_admin_back_cb, LV_EVENT_CLICKED, NULL);

    lbl_admin_back = lv_label_create(btn_admin_back);
    lv_obj_center(lbl_admin_back);
    lv_label_set_text(lbl_admin_back, "< KEMBALI");
    lv_obj_set_style_text_color(lbl_admin_back, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_admin_back, &lv_font_montserrat_14, 0);

    /* Judul Header */
    lbl_admin_title = lv_label_create(header_admin);
    lv_obj_set_pos(lbl_admin_title, 130, 6);
    lv_label_set_text(lbl_admin_title, "PENGATURAN ADMIN");
    lv_obj_set_style_text_color(lbl_admin_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_admin_title, &lv_font_montserrat_16, 0);

    /* Jam Digital Header Admin */
    lbl_admin_clock = lv_label_create(header_admin);
    lv_obj_set_pos(lbl_admin_clock, 388, 6);
    lv_label_set_text(lbl_admin_clock, "00:00:00");
    lv_obj_set_style_text_color(lbl_admin_clock, lv_color_hex(COLOR_AMBER_STATUS), 0);
    lv_obj_set_style_text_font(lbl_admin_clock, &lv_font_montserrat_14, 0);

    /* 2. Tabview Admin (y: 40, w: 480, h: 280) */
    tabview_admin = lv_tabview_create(screen_admin);
    lv_obj_set_pos(tabview_admin, 0, 40);
    lv_obj_set_size(tabview_admin, 480, 280);
    lv_obj_set_style_bg_color(tabview_admin, lv_color_hex(COLOR_DARK_BG), 0);

    lv_obj_t * tab_sistem = lv_tabview_add_tab(tabview_admin, "SISTEM & DISPLAY");
    lv_obj_t * tab_wifi   = lv_tabview_add_tab(tabview_admin, "KONFIGURASI WI-FI");

    /* ====================================================================== */
    /* TAB 1: SISTEM & DISPLAY                                                */
    /* ====================================================================== */
    lv_obj_set_style_pad_all(tab_sistem, 12, 0);

    /* Baris 1: Kecerahan Layar */
    lv_obj_t * lbl_bright_title = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_bright_title, 10, 8);
    lv_label_set_text(lbl_bright_title, "Kecerahan Layar:");
    lv_obj_set_style_text_color(lbl_bright_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_bright_title, &lv_font_montserrat_14, 0);

    slider_brightness = lv_slider_create(tab_sistem);
    lv_obj_set_pos(slider_brightness, 150, 10);
    lv_obj_set_size(slider_brightness, 180, 16);
    lv_slider_set_range(slider_brightness, 10, 100);
    lv_slider_set_value(slider_brightness, 75, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider_brightness, slider_brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lbl_bright_val = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_bright_val, 345, 8);
    lv_label_set_text(lbl_bright_val, "75%");
    lv_obj_set_style_text_color(lbl_bright_val, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_bright_val, &lv_font_montserrat_14, 0);

    /* Baris 2: Tema Tampilan */
    lv_obj_t * lbl_theme_title = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_theme_title, 10, 48);
    lv_label_set_text(lbl_theme_title, "Tema Tampilan:");
    lv_obj_set_style_text_color(lbl_theme_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_theme_title, &lv_font_montserrat_14, 0);

    sw_theme = lv_switch_create(tab_sistem);
    lv_obj_set_pos(sw_theme, 150, 44);
    lv_obj_add_event_cb(sw_theme, sw_theme_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lbl_theme_desc = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_theme_desc, 220, 48);
    lv_label_set_text(lbl_theme_desc, "Mode Gelap (Dark)");
    lv_obj_set_style_text_color(lbl_theme_desc, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_theme_desc, &lv_font_montserrat_14, 0);

    /* Baris 3: Tombol Aksi Hardware & Sinkronisasi */
    btn_test_beep = lv_button_create(tab_sistem);
    lv_obj_set_pos(btn_test_beep, 10, 92);
    lv_obj_set_size(btn_test_beep, 130, 36);
    lv_obj_set_style_bg_color(btn_test_beep, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_test_beep, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_test_beep, 1, 0);
    lv_obj_set_style_radius(btn_test_beep, 8, 0);
    lv_obj_add_event_cb(btn_test_beep, btn_test_beep_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_t_beep = lv_label_create(btn_test_beep);
    lv_obj_center(lbl_t_beep);
    lv_label_set_text(lbl_t_beep, "TEST BEEP");
    lv_obj_set_style_text_color(lbl_t_beep, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_t_beep, &lv_font_montserrat_14, 0);

    btn_reset_stats = lv_button_create(tab_sistem);
    lv_obj_set_pos(btn_reset_stats, 155, 92);
    lv_obj_set_size(btn_reset_stats, 130, 36);
    lv_obj_set_style_bg_color(btn_reset_stats, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_reset_stats, lv_color_hex(COLOR_RED_STATUS), 0);
    lv_obj_set_style_border_width(btn_reset_stats, 1, 0);
    lv_obj_set_style_radius(btn_reset_stats, 8, 0);
    lv_obj_add_event_cb(btn_reset_stats, btn_reset_stats_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_t_reset = lv_label_create(btn_reset_stats);
    lv_obj_center(lbl_t_reset);
    lv_label_set_text(lbl_t_reset, "RESET STATS");
    lv_obj_set_style_text_color(lbl_t_reset, lv_color_hex(COLOR_RED_STATUS), 0);
    lv_obj_set_style_text_font(lbl_t_reset, &lv_font_montserrat_14, 0);

    btn_ntp_sync = lv_button_create(tab_sistem);
    lv_obj_set_pos(btn_ntp_sync, 300, 92);
    lv_obj_set_size(btn_ntp_sync, 140, 36);
    lv_obj_set_style_bg_color(btn_ntp_sync, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_ntp_sync, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_border_width(btn_ntp_sync, 1, 0);
    lv_obj_set_style_radius(btn_ntp_sync, 8, 0);
    lv_obj_add_event_cb(btn_ntp_sync, btn_ntp_sync_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_t_ntp = lv_label_create(btn_ntp_sync);
    lv_obj_center(lbl_t_ntp);
    lv_label_set_text(lbl_t_ntp, "SYNC JAM (NTP)");
    lv_obj_set_style_text_color(lbl_t_ntp, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_text_font(lbl_t_ntp, &lv_font_montserrat_14, 0);

    /* Baris 4: Label Feedback Aksi */
    lbl_admin_feedback = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_admin_feedback, 10, 145);
    lv_obj_set_width(lbl_admin_feedback, 430);
    lv_label_set_text(lbl_admin_feedback, "Status: Siap");
    lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_admin_feedback, &lv_font_montserrat_14, 0);

    /* ====================================================================== */
    /* TAB 2: KONFIGURASI WI-FI                                               */
    /* ====================================================================== */
    lv_obj_set_style_pad_all(tab_wifi, 8, 0);

    lv_obj_t * lbl_ssid_t = lv_label_create(tab_wifi);
    lv_obj_set_pos(lbl_ssid_t, 10, 6);
    lv_label_set_text(lbl_ssid_t, "SSID Wi-Fi:");
    lv_obj_set_style_text_color(lbl_ssid_t, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_ssid_t, &lv_font_montserrat_14, 0);

    ta_wifi_ssid = lv_textarea_create(tab_wifi);
    lv_obj_set_pos(ta_wifi_ssid, 100, 2);
    lv_obj_set_size(ta_wifi_ssid, 180, 32);
    lv_textarea_set_placeholder_text(ta_wifi_ssid, "Nama Wi-Fi");
    lv_textarea_set_one_line(ta_wifi_ssid, true);
    lv_obj_add_event_cb(ta_wifi_ssid, ta_wifi_focus_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * lbl_pass_t = lv_label_create(tab_wifi);
    lv_obj_set_pos(lbl_pass_t, 10, 42);
    lv_label_set_text(lbl_pass_t, "Password:");
    lv_obj_set_style_text_color(lbl_pass_t, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_pass_t, &lv_font_montserrat_14, 0);

    ta_wifi_pass = lv_textarea_create(tab_wifi);
    lv_obj_set_pos(ta_wifi_pass, 100, 38);
    lv_obj_set_size(ta_wifi_pass, 180, 32);
    lv_textarea_set_password_mode(ta_wifi_pass, true);
    lv_textarea_set_placeholder_text(ta_wifi_pass, "Password Wi-Fi");
    lv_textarea_set_one_line(ta_wifi_pass, true);
    lv_obj_add_event_cb(ta_wifi_pass, ta_wifi_focus_cb, LV_EVENT_ALL, NULL);

    /* Tombol Simpan Wi-Fi */
    btn_save_wifi = lv_button_create(tab_wifi);
    lv_obj_set_pos(btn_save_wifi, 295, 10);
    lv_obj_set_size(btn_save_wifi, 145, 54);
    lv_obj_set_style_bg_color(btn_save_wifi, lv_color_hex(0x064E3B), 0);
    lv_obj_set_style_border_color(btn_save_wifi, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_border_width(btn_save_wifi, 1, 0);
    lv_obj_set_style_radius(btn_save_wifi, 8, 0);
    lv_obj_add_event_cb(btn_save_wifi, btn_save_wifi_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_save_btn = lv_label_create(btn_save_wifi);
    lv_obj_center(lbl_save_btn);
    lv_label_set_text(lbl_save_btn, "SIMPAN & KIRIM\nKE ESP32");
    lv_obj_set_style_text_align(lbl_save_btn, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_save_btn, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_text_font(lbl_save_btn, &lv_font_montserrat_14, 0);

    /* Keyboard Virtual untuk Input Teks Wi-Fi */
    kb_wifi = lv_keyboard_create(tab_wifi);
    lv_obj_set_pos(kb_wifi, 0, 78);
    lv_obj_set_size(kb_wifi, 460, 130);
    lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
}

/**
  * @brief  Proses event kedatangan data RFID / Verifikasi Server
  */
void GUI_Attendance_ProcessEvent(const ui_event_t *evt)
{
    if (evt == NULL || screen_main == NULL) return;

    if (evt->type == UI_EVENT_RFID_CARD_DETECTED)
    {
        /* Format UID string */
        char uid_str[32];
        if (evt->uid_len >= 4) {
            snprintf(uid_str, sizeof(uid_str), "UID: %02X %02X %02X %02X",
                     evt->uid[0], evt->uid[1], evt->uid[2], evt->uid[3]);
        } else {
            snprintf(uid_str, sizeof(uid_str), "UID: UNKNOWN");
        }
        lv_label_set_text(lbl_uid, uid_str);

        /* Tampilkan status sedang memverifikasi ke server */
        lv_label_set_text(lbl_status, "Memverifikasi ke Server...");
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(COLOR_AMBER_STATUS), 0);
        lv_obj_set_style_border_color(card_hero, lv_color_hex(COLOR_AMBER_STATUS), 0);
        lv_obj_set_style_border_color(box_status, lv_color_hex(COLOR_AMBER_STATUS), 0);

        /* Update waktu transaksi terakhir */
        if (evt->extra[0] != '\0') {
            lv_label_set_text(lbl_last_val, evt->extra);
        }

        /* Perbarui counter total */
        g_attendance_total++;
        lv_label_set_text_fmt(lbl_total_val, "%lu", (unsigned long)g_attendance_total);

        /* Mulai timer fallback kembali ke standby */
        if (revert_timer) {
            lv_timer_delete(revert_timer);
        }
        revert_timer = lv_timer_create(revert_standby_cb, 3500, NULL);
        lv_timer_set_repeat_count(revert_timer, 1);
    }
    else if (evt->type == UI_EVENT_ATTENDANCE_SUCCESS)
    {
        /* Sukses verifikasi kehadiran dari ESP32 */
        char msg[40];
        if (evt->extra[0] != '\0') {
            snprintf(msg, sizeof(msg), "HADIR: %s", evt->extra);
        } else {
            snprintf(msg, sizeof(msg), "AKSES DITERIMA");
        }
        lv_label_set_text(lbl_status, msg);
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(COLOR_GREEN_STATUS), 0);
        lv_obj_set_style_border_color(card_hero, lv_color_hex(COLOR_GREEN_STATUS), 0);
        lv_obj_set_style_border_color(box_status, lv_color_hex(COLOR_GREEN_STATUS), 0);

        if (revert_timer) {
            lv_timer_delete(revert_timer);
        }
        revert_timer = lv_timer_create(revert_standby_cb, 3000, NULL);
        lv_timer_set_repeat_count(revert_timer, 1);
    }
    else if (evt->type == UI_EVENT_ATTENDANCE_REJECTED)
    {
        /* Akses ditolak dari ESP32 */
        char msg[40];
        if (evt->extra[0] != '\0') {
            snprintf(msg, sizeof(msg), "DITOLAK: %s", evt->extra);
        } else {
            snprintf(msg, sizeof(msg), "KARTU DITOLAK");
        }
        lv_label_set_text(lbl_status, msg);
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(COLOR_RED_STATUS), 0);
        lv_obj_set_style_border_color(card_hero, lv_color_hex(COLOR_RED_STATUS), 0);
        lv_obj_set_style_border_color(box_status, lv_color_hex(COLOR_RED_STATUS), 0);

        if (revert_timer) {
            lv_timer_delete(revert_timer);
        }
        revert_timer = lv_timer_create(revert_standby_cb, 3000, NULL);
        lv_timer_set_repeat_count(revert_timer, 1);
    }
    else if (evt->type == UI_EVENT_NTP_SYNCED)
    {
        if (lbl_admin_feedback) {
            lv_label_set_text(lbl_admin_feedback, "Status: Jam Berhasil Disinkronkan!");
            lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_GREEN_STATUS), 0);
        }
    }
}

/**
  * @brief  Perbarui tampilan jam digital dan tanggal pada header
  */
void GUI_Attendance_UpdateClock(const RTC_TimeTypeDef *sTime, const RTC_DateTypeDef *sDate)
{
    if (sTime != NULL) {
        char time_buf[16];
        snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d",
                 sTime->Hours, sTime->Minutes, sTime->Seconds);
        if (lbl_clock_main) lv_label_set_text(lbl_clock_main, time_buf);
        if (lbl_admin_clock) lv_label_set_text(lbl_admin_clock, time_buf);
    }

    if (sDate != NULL && lbl_date_val != NULL) {
        char date_buf[20];
        snprintf(date_buf, sizeof(date_buf), "20%02d-%02d-%02d",
                 sDate->Year, sDate->Month, sDate->Date);
        lv_label_set_text(lbl_date_val, date_buf);
    }
}

/**
  * @brief  Perbarui status badge peripheral (RFID, Wi-Fi, MicroSD) secara real-time
  */
void GUI_Attendance_UpdateBadges(bool rfid_ok, bool wifi_ok, bool sd_ok)
{
    /* 1. RFID Status Badge */
    if (badge_rfid != NULL && lbl_badge_rfid != NULL) {
        if (rfid_ok) {
            lv_obj_set_style_bg_color(badge_rfid, lv_color_hex(0x064E3B), 0);
            lv_obj_set_style_border_color(badge_rfid, lv_color_hex(COLOR_GREEN_STATUS), 0);
            lv_label_set_text(lbl_badge_rfid, "RFID: OK");
            lv_obj_set_style_text_color(lbl_badge_rfid, lv_color_hex(COLOR_GREEN_STATUS), 0);
        } else {
            lv_obj_set_style_bg_color(badge_rfid, lv_color_hex(0x450A0A), 0);
            lv_obj_set_style_border_color(badge_rfid, lv_color_hex(COLOR_RED_STATUS), 0);
            lv_label_set_text(lbl_badge_rfid, "RFID: ERR");
            lv_obj_set_style_text_color(lbl_badge_rfid, lv_color_hex(COLOR_RED_STATUS), 0);
        }
    }

    /* 2. Wi-Fi Status Badge Real-Time */
    if (badge_wifi != NULL && lbl_badge_wifi != NULL) {
        if (wifi_ok) {
            lv_obj_set_style_bg_color(badge_wifi, lv_color_hex(0x064E3B), 0);
            lv_obj_set_style_border_color(badge_wifi, lv_color_hex(COLOR_GREEN_STATUS), 0);
            lv_label_set_text(lbl_badge_wifi, "WIFI: OK");
            lv_obj_set_style_text_color(lbl_badge_wifi, lv_color_hex(COLOR_GREEN_STATUS), 0);
        } else {
            lv_obj_set_style_bg_color(badge_wifi, lv_color_hex(0x450A0A), 0);
            lv_obj_set_style_border_color(badge_wifi, lv_color_hex(COLOR_RED_STATUS), 0);
            lv_label_set_text(lbl_badge_wifi, "WIFI: ERR");
            lv_obj_set_style_text_color(lbl_badge_wifi, lv_color_hex(COLOR_RED_STATUS), 0);
        }
    }

    /* 3. MicroSD Card Status Badge Real-Time */
    if (badge_sd != NULL && lbl_badge_sd != NULL) {
        if (sd_ok) {
            lv_obj_set_style_bg_color(badge_sd, lv_color_hex(0x064E3B), 0);
            lv_obj_set_style_border_color(badge_sd, lv_color_hex(COLOR_GREEN_STATUS), 0);
            lv_label_set_text(lbl_badge_sd, "SD: OK");
            lv_obj_set_style_text_color(lbl_badge_sd, lv_color_hex(COLOR_GREEN_STATUS), 0);
        } else {
            lv_obj_set_style_bg_color(badge_sd, lv_color_hex(0x450A0A), 0);
            lv_obj_set_style_border_color(badge_sd, lv_color_hex(COLOR_RED_STATUS), 0);
            lv_label_set_text(lbl_badge_sd, "SD: ERR");
            lv_obj_set_style_text_color(lbl_badge_sd, lv_color_hex(COLOR_RED_STATUS), 0);
        }
    }
}

/**
  * @brief  Deteksi sentuhan tahan pojok bawah kanan layar selama 3 detik untuk masuk ke menu admin
  */
void GUI_Attendance_CheckCornerTouch(uint16_t x, uint16_t y)
{
    /* Pojok bawah kanan: X > 360, Y > 220 pada layar utama */
    if (lv_screen_active() == screen_main && x > 360 && y > 220)
    {
        if (!corner_touching)
        {
            corner_touching = true;
            corner_touch_start = HAL_GetTick();
        }
        else
        {
            if (HAL_GetTick() - corner_touch_start >= 3000)
            {
                corner_touching = false;

                /* Feedback Audio */
                HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
                osDelay(70);
                HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);

                /* Reset input PIN */
                g_pin_len = 0;
                g_entered_pin[0] = '\0';
                if (pwd_err_timer) {
                    lv_timer_delete(pwd_err_timer);
                    pwd_err_timer = NULL;
                }
                if (lbl_pin_display) lv_label_set_text(lbl_pin_display, "_ _ _ _");
                if (lbl_pin_feedback) lv_label_set_text(lbl_pin_feedback, "");

                /* Buka Layar Masukkan Password */
                lv_screen_load(screen_password);
            }
        }
    }
    else
    {
        corner_touching = false;
    }
}

/**
  * @brief  Reset hitungan total absensi
  */
void GUI_Attendance_ResetCounter(void)
{
    g_attendance_total = 0;
    if (lbl_total_val) {
        lv_label_set_text(lbl_total_val, "0");
    }

    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
    osDelay(40);
    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);
    osDelay(40);
    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
    osDelay(40);
    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);
}

/**
  * @brief  Terapkan perubahan tema (Terang / Gelap) ke seluruh elemen GUI
  */
static void apply_theme(bool is_dark)
{
    g_theme_is_dark = is_dark;
    uint32_t c_bg       = is_dark ? COLOR_DARK_BG       : COLOR_LIGHT_BG;
    uint32_t c_card     = is_dark ? COLOR_DARK_CARD     : COLOR_LIGHT_CARD;
    uint32_t c_sub      = is_dark ? COLOR_DARK_SUB_CARD : COLOR_LIGHT_SUB_CARD;
    uint32_t c_border   = is_dark ? COLOR_DARK_BORDER   : COLOR_LIGHT_BORDER;
    uint32_t c_text_pri = is_dark ? COLOR_DARK_TEXT_PRIMARY : COLOR_LIGHT_TEXT_PRIMARY;
    uint32_t c_text_mut = is_dark ? COLOR_DARK_TEXT_MUTED   : COLOR_LIGHT_TEXT_MUTED;

    /* Layar Utama */
    if (screen_main) lv_obj_set_style_bg_color(screen_main, lv_color_hex(c_bg), 0);
    if (header_main) lv_obj_set_style_bg_color(header_main, lv_color_hex(is_dark ? 0x111827 : 0xFFFFFF), 0);
    if (lbl_header_title) lv_obj_set_style_text_color(lbl_header_title, lv_color_hex(c_text_pri), 0);

    if (card_hero) {
        lv_obj_set_style_bg_color(card_hero, lv_color_hex(c_card), 0);
        lv_obj_set_style_border_color(card_hero, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    }
    if (tap_zone) {
        lv_obj_set_style_bg_color(tap_zone, lv_color_hex(c_sub), 0);
    }
    if (lbl_tap_desc) lv_obj_set_style_text_color(lbl_tap_desc, lv_color_hex(c_text_mut), 0);

    if (box_uid) {
        lv_obj_set_style_bg_color(box_uid, lv_color_hex(is_dark ? 0x060E1A : 0xF8FAFC), 0);
        lv_obj_set_style_border_color(box_uid, lv_color_hex(c_border), 0);
    }
    if (lbl_uid) lv_obj_set_style_text_color(lbl_uid, lv_color_hex(c_text_pri), 0);

    if (box_status) {
        lv_obj_set_style_bg_color(box_status, lv_color_hex(is_dark ? 0x060E1A : 0xF8FAFC), 0);
    }

    if (card_summary) {
        lv_obj_set_style_bg_color(card_summary, lv_color_hex(c_card), 0);
        lv_obj_set_style_border_color(card_summary, lv_color_hex(c_border), 0);
    }
    if (lbl_sum_title) lv_obj_set_style_text_color(lbl_sum_title, lv_color_hex(c_text_mut), 0);
    if (lbl_total_title) lv_obj_set_style_text_color(lbl_total_title, lv_color_hex(c_text_pri), 0);
    if (lbl_total_unit) lv_obj_set_style_text_color(lbl_total_unit, lv_color_hex(c_text_mut), 0);
    if (lbl_date_title) lv_obj_set_style_text_color(lbl_date_title, lv_color_hex(c_text_mut), 0);
    if (lbl_date_val) lv_obj_set_style_text_color(lbl_date_val, lv_color_hex(c_text_pri), 0);
    if (lbl_last_title) lv_obj_set_style_text_color(lbl_last_title, lv_color_hex(c_text_mut), 0);

    /* Layar Admin */
    if (screen_admin) lv_obj_set_style_bg_color(screen_admin, lv_color_hex(c_bg), 0);
    if (header_admin) lv_obj_set_style_bg_color(header_admin, lv_color_hex(is_dark ? 0x111827 : 0xFFFFFF), 0);
    if (lbl_admin_title) lv_obj_set_style_text_color(lbl_admin_title, lv_color_hex(c_text_pri), 0);
    if (tabview_admin) lv_obj_set_style_bg_color(tabview_admin, lv_color_hex(c_bg), 0);
    if (lbl_theme_desc) {
        lv_label_set_text(lbl_theme_desc, is_dark ? "Mode Gelap (Dark)" : "Mode Terang (Light)");
        lv_obj_set_style_text_color(lbl_theme_desc, lv_color_hex(c_text_mut), 0);
    }
}

/* ========================================================================== */
/*                              STATIC CALLBACKS                              */
/* ========================================================================== */

static void revert_standby_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    if (lbl_status) {
        lv_label_set_text(lbl_status, "Siap Merekam Kehadiran Anda");
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(COLOR_CYAN_TEXT), 0);
    }
    if (lbl_uid) {
        lv_label_set_text(lbl_uid, "UID: -- -- -- --");
    }
    if (card_hero) {
        lv_obj_set_style_border_color(card_hero, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    }
    if (box_status) {
        lv_obj_set_style_border_color(box_status, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    }
    revert_timer = NULL;
}

static void pwd_err_timeout_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    pwd_err_timer = NULL;
    lv_screen_load(screen_main);
}

static void btn_keypad_click_cb(lv_event_t * e)
{
    if (pwd_err_timer != NULL) return; /* Abaikan tombol saat error timer sedang aktif */

    const char * key = (const char *)lv_event_get_user_data(e);
    if (key == NULL) return;

    if (strcmp(key, "BATAL") == 0) {
        g_pin_len = 0;
        g_entered_pin[0] = '\0';
        if (pwd_err_timer) {
            lv_timer_delete(pwd_err_timer);
            pwd_err_timer = NULL;
        }
        lv_screen_load(screen_main);
        return;
    }

    if (strcmp(key, "DEL") == 0) {
        if (g_pin_len > 0) {
            g_pin_len--;
            g_entered_pin[g_pin_len] = '\0';
        }
    }
    else {
        /* Tambah digit */
        if (g_pin_len < 8) {
            g_entered_pin[g_pin_len++] = key[0];
            g_entered_pin[g_pin_len] = '\0';
        }
    }

    /* Update Masked PIN Display */
    char mask_str[32] = {0};
    if (g_pin_len == 0) {
        snprintf(mask_str, sizeof(mask_str), "_ _ _ _");
    } else {
        for (uint8_t i = 0; i < g_pin_len; i++) {
            strncat(mask_str, "* ", sizeof(mask_str) - strlen(mask_str) - 1);
        }
    }
    lv_label_set_text(lbl_pin_display, mask_str);

    /* Cek jika sudah memasukkan 4 digit */
    if (g_pin_len >= 4) {
        if (strcmp(g_entered_pin, ADMIN_MASTER_PIN) == 0) {
            /* PIN Benar! */
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
            osDelay(40);
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);
            osDelay(40);
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
            osDelay(40);
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);

            g_pin_len = 0;
            g_entered_pin[0] = '\0';
            lv_screen_load(screen_admin);
        } else {
            /* PIN Salah! */
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
            osDelay(250);
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);

            lv_label_set_text(lbl_pin_feedback, "PIN SALAH! KEMBALI KE MENU UTAMA...");
            g_pin_len = 0;
            g_entered_pin[0] = '\0';

            /* Kembali ke main screen setelah 1.5 detik */
            if (pwd_err_timer) lv_timer_delete(pwd_err_timer);
            pwd_err_timer = lv_timer_create(pwd_err_timeout_cb, 1500, NULL);
            lv_timer_set_repeat_count(pwd_err_timer, 1);
        }
    }
}

static void slider_brightness_cb(lv_event_t * e)
{
    int32_t val = lv_slider_get_value(slider_brightness);
    uint32_t compare = (val * 65535) / 100;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, compare);
    lv_label_set_text_fmt(lbl_bright_val, "%ld%%", (long)val);
}

static void sw_theme_cb(lv_event_t * e)
{
    bool is_checked = lv_obj_has_state(sw_theme, LV_STATE_CHECKED);
    apply_theme(!is_checked); /* Checked = Terang, Unchecked = Gelap */
}

static void btn_admin_back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_screen_load(screen_main);
}

static void btn_test_beep_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
    osDelay(80);
    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);
    if (lbl_admin_feedback) {
        lv_label_set_text(lbl_admin_feedback, "Status: Test Beep Berhasil");
        lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_CYAN_TEXT), 0);
    }
}

static void btn_reset_stats_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_ResetCounter();
    if (lbl_admin_feedback) {
        lv_label_set_text(lbl_admin_feedback, "Status: Counter Absensi Direset ke 0");
        lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_AMBER_STATUS), 0);
    }
}

static void btn_ntp_sync_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    /* Kirim request sinkronisasi jam ke ESP32 */
    esp_tx_item_t item = {0};
    snprintf(item.payload, sizeof(item.payload), "CMD:SYNC_TIME\n");
    osMessageQueuePut(espTxQueueHandle, &item, 0, 50);

    if (lbl_admin_feedback) {
        lv_label_set_text(lbl_admin_feedback, "Status: Mengirim Request NTP ke Server...");
        lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_AMBER_STATUS), 0);
    }
}

static void btn_save_wifi_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    const char * ssid = lv_textarea_get_text(ta_wifi_ssid);
    const char * pass = lv_textarea_get_text(ta_wifi_pass);

    if (strlen(ssid) == 0) {
        if (lbl_admin_feedback) {
            lv_label_set_text(lbl_admin_feedback, "Peringatan: SSID tidak boleh kosong!");
            lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
        }
        return;
    }

    /* Kirim paket konfigurasi Wi-Fi ke ESP32-C6 via USART1 */
    esp_tx_item_t item = {0};
    snprintf(item.payload, sizeof(item.payload), "WIFI:%s|%s|0.0.0.0\n", ssid, pass);
    osMessageQueuePut(espTxQueueHandle, &item, 0, 50);

    /* Sembunyikan keyboard */
    lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);

    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
    osDelay(50);
    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);

    if (lbl_admin_feedback) {
        lv_label_set_text(lbl_admin_feedback, "Status: Konfigurasi Wi-Fi Dikirim ke ESP32!");
        lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_GREEN_STATUS), 0);
    }
}

static void ta_wifi_focus_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        lv_keyboard_set_textarea(kb_wifi, ta);
        lv_obj_clear_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(kb_wifi);
    } else if (code == LV_EVENT_DEFOCUSED || code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
    }
}
