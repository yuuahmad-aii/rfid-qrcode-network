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
#include "ili9488.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>

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
static lv_obj_t * btn_keypad[12]      = {0};
static lv_obj_t * lbl_keypad[12]      = {0};
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
static lv_obj_t * lbl_bright_title    = NULL;
static lv_obj_t * slider_brightness   = NULL;
static lv_obj_t * lbl_bright_val      = NULL;
static lv_obj_t * lbl_theme_title     = NULL;
static lv_obj_t * sw_theme            = NULL;
static lv_obj_t * lbl_theme_desc      = NULL;
static lv_obj_t * lbl_timeout_title   = NULL;
static lv_obj_t * dropdown_timeout    = NULL;
static lv_obj_t * btn_test_beep       = NULL;
static lv_obj_t * lbl_t_beep          = NULL;
static lv_obj_t * btn_reset_stats     = NULL;
static lv_obj_t * lbl_t_reset         = NULL;
static lv_obj_t * btn_ntp_sync        = NULL;
static lv_obj_t * lbl_t_ntp           = NULL;
static lv_obj_t * lbl_admin_feedback  = NULL;

/* Tab 2: Wi-Fi Setup */
static lv_obj_t * lbl_ssid_t          = NULL;
static lv_obj_t * ta_wifi_ssid        = NULL;
static lv_obj_t * lbl_pass_t          = NULL;
static lv_obj_t * ta_wifi_pass        = NULL;
static lv_obj_t * btn_save_wifi       = NULL;
static lv_obj_t * lbl_save_btn        = NULL;
static lv_obj_t * kb_wifi             = NULL;

/* Tab 3: Log Absensi MicroSD */
static lv_obj_t * tab_log             = NULL;
static lv_obj_t * lbl_log_title       = NULL;
static lv_obj_t * btn_refresh_log     = NULL;
static lv_obj_t * lbl_refresh_log     = NULL;
static lv_obj_t * cont_log_list       = NULL;
static lv_obj_t * table_log           = NULL;

/* Tab 4: Test Device (Diagnostik Hardware) */
static lv_obj_t * tab_test            = NULL;
static lv_obj_t * btn_diag_all        = NULL;
static lv_obj_t * btn_diag_beep       = NULL;
static lv_obj_t * btn_diag_rfid       = NULL;
static lv_obj_t * btn_diag_sd         = NULL;
static lv_obj_t * btn_diag_wifi       = NULL;
static lv_obj_t * btn_diag_rtc        = NULL;
static lv_obj_t * cont_diag_box       = NULL;
static lv_obj_t * lbl_diag_title      = NULL;
static lv_obj_t * lbl_diag_summary    = NULL;
static lv_obj_t * lbl_diag_detail     = NULL;

/* Timer */
static lv_timer_t * revert_timer      = NULL;
static lv_timer_t * pwd_err_timer     = NULL;
static lv_timer_t * ntp_timeout_timer = NULL;

/* Variabel Deteksi Sentuh Pojok Layar (3 Detik) */
static uint32_t corner_touch_start    = 0;
static bool     corner_touching       = false;

/* Variabel Screen Sleep & Kecerahan */
static uint32_t g_screen_timeout_sec  = 60;  /* Default 60 detik (1 menit) */
static uint32_t g_last_activity_tick  = 0;
static bool     g_screen_is_sleeping  = false;
static uint32_t g_current_brightness_val = 75;

/* Forward Callbacks */
static void build_screen_main(void);
static void build_screen_password(void);
static void build_screen_admin(void);
static void apply_theme(bool is_dark);
static void update_admin_tab_styles(bool is_dark);
static void revert_standby_cb(lv_timer_t * t);
static void pwd_err_timeout_cb(lv_timer_t * t);
static void ntp_timeout_cb(lv_timer_t * t);
static void load_recent_logs(void);

static void btn_keypad_click_cb(lv_event_t * e);
static void slider_brightness_cb(lv_event_t * e);
static void sw_theme_cb(lv_event_t * e);
static void dropdown_timeout_cb(lv_event_t * e);
static void btn_admin_back_cb(lv_event_t * e);
static void btn_test_beep_cb(lv_event_t * e);
static void btn_reset_stats_cb(lv_event_t * e);
static void btn_ntp_sync_cb(lv_event_t * e);
static void btn_save_wifi_cb(lv_event_t * e);
static void ta_wifi_focus_cb(lv_event_t * e);
static void kb_wifi_event_cb(lv_event_t * e);
static void btn_refresh_log_cb(lv_event_t * e);

/* Diagnostic Callbacks */
static void btn_diag_beep_cb(lv_event_t * e);
static void btn_diag_rfid_cb(lv_event_t * e);
static void btn_diag_sd_cb(lv_event_t * e);
static void btn_diag_wifi_cb(lv_event_t * e);
static void btn_diag_rtc_cb(lv_event_t * e);
static void btn_diag_all_cb(lv_event_t * e);

/* ========================================================================== */
/*                PERSISTENT CONFIGURATION (STM32 FLASH SECTOR 11)            */
/* ========================================================================== */
app_settings_t g_app_settings = {0};

void App_Settings_ResetDefaults(app_settings_t *s)
{
    memset(s, 0, sizeof(app_settings_t));
    s->magic              = SETTINGS_MAGIC;
    s->version            = SETTINGS_VERSION;
    s->brightness         = 75;  /* 75% */
    s->theme_mode         = 1;   /* 1 = Dark Mode */
    s->screen_timeout_sec = 60;  /* 60 detik */
    strncpy(s->wifi_ssid, "WiFi_Absensi", sizeof(s->wifi_ssid) - 1);
    strncpy(s->wifi_pass, "12345678", sizeof(s->wifi_pass) - 1);
    strncpy(s->server_ip, "0.0.0.0", sizeof(s->server_ip) - 1);
}

bool App_Settings_Load(app_settings_t *out)
{
    const app_settings_t *flash_cfg = (const app_settings_t *)SETTINGS_FLASH_ADDR;
    if (flash_cfg->magic != SETTINGS_MAGIC || flash_cfg->version != SETTINGS_VERSION) {
        return false;
    }

    uint16_t sum = 0;
    const uint8_t *p_bytes = (const uint8_t *)flash_cfg;
    for (size_t i = 0; i < sizeof(app_settings_t); i++) {
        if (i == offsetof(app_settings_t, checksum) || i == (offsetof(app_settings_t, checksum) + 1)) {
            continue;
        }
        sum += p_bytes[i];
    }
    if (sum != flash_cfg->checksum) {
        return false;
    }

    *out = *flash_cfg;
    return true;
}

bool App_Settings_Save(const app_settings_t *in)
{
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    FLASH_EraseInitTypeDef EraseInitStruct;
    EraseInitStruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector       = SETTINGS_FLASH_SECTOR;
    EraseInitStruct.NbSectors    = 1;

    uint32_t SectorError = 0;
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    app_settings_t temp = *in;
    temp.magic = SETTINGS_MAGIC;
    temp.version = SETTINGS_VERSION;
    temp.checksum = 0;

    uint16_t sum = 0;
    const uint8_t *p_bytes = (const uint8_t *)&temp;
    for (size_t i = 0; i < sizeof(app_settings_t); i++) {
        if (i == offsetof(app_settings_t, checksum) || i == (offsetof(app_settings_t, checksum) + 1)) {
            continue;
        }
        sum += p_bytes[i];
    }
    temp.checksum = sum;

    const uint32_t *p_data = (const uint32_t *)&temp;
    uint32_t dest = SETTINGS_FLASH_ADDR;
    for (size_t i = 0; i < sizeof(app_settings_t) / sizeof(uint32_t); i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest, p_data[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
        dest += 4;
    }

    HAL_FLASH_Lock();
    return true;
}

void App_Settings_Init(void)
{
    if (!App_Settings_Load(&g_app_settings)) {
        App_Settings_ResetDefaults(&g_app_settings);
        App_Settings_Save(&g_app_settings);
    }
    g_current_brightness_val = g_app_settings.brightness;
    g_screen_timeout_sec     = g_app_settings.screen_timeout_sec;
}

/* ========================================================================== */
/*                CUSTOM KEYBOARD MAP (FULL 5 ROWS WITH NUMBERS)             */
/* ========================================================================== */
static const char * const custom_kb_map_lc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_NEW_LINE, "\n",
    "ABC", "z", "x", "c", "v", "b", "n", "m", "_", "-", ".", "\n",
    "1#", " ", "@", LV_SYMBOL_CLOSE, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t custom_kb_ctrl_lc_map[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, LV_BUTTONMATRIX_CTRL_CHECKED | 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, 6, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2
};

static const char * const custom_kb_map_uc[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", LV_SYMBOL_NEW_LINE, "\n",
    "abc", "Z", "X", "C", "V", "B", "N", "M", "_", "-", ".", "\n",
    "1#", " ", "@", LV_SYMBOL_CLOSE, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t custom_kb_ctrl_uc_map[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, LV_BUTTONMATRIX_CTRL_CHECKED | 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, 6, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2
};

static const char * const custom_kb_map_spec[] = {
    "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", LV_SYMBOL_BACKSPACE, "\n",
    "~", "`", "+", "=", "[", "]", "{", "}", "\\", "|", "\n",
    ":", ";", "\"", "'", "<", ">", ",", "?", "/", LV_SYMBOL_NEW_LINE, "\n",
    "abc", "_", "-", ".", "#", "%", "*", "+", "=", "\n",
    "1#", " ", "@", LV_SYMBOL_CLOSE, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t custom_kb_ctrl_spec_map[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, LV_BUTTONMATRIX_CTRL_CHECKED | 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, 1, 1, 1, 1, 1, 1, 1, 1,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, 6, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2
};


/**
  * @brief  Inisialisasi antarmuka pengguna grafis LVGL
  */
void GUI_Attendance_Init(void)
{
    g_last_activity_tick = HAL_GetTick();

    /* Muat pengaturan dari Flash */
    App_Settings_Init();

    /* Bangun ketiga layar antarmuka */
    build_screen_main();
    build_screen_password();
    build_screen_admin();

    /* Pulihkan nilai widget di menu admin sesuai konfigurasi tersimpan */
    if (slider_brightness) {
        lv_slider_set_value(slider_brightness, g_app_settings.brightness, LV_ANIM_OFF);
    }
    if (lbl_bright_val) {
        lv_label_set_text_fmt(lbl_bright_val, "%d%%", g_app_settings.brightness);
    }
    if (sw_theme) {
        if (g_app_settings.theme_mode == 0) {
            lv_obj_add_state(sw_theme, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(sw_theme, LV_STATE_CHECKED);
        }
    }
    if (dropdown_timeout) {
        uint16_t sel_idx = 2; /* 60s default */
        if (g_app_settings.screen_timeout_sec == 0) sel_idx = 0;
        else if (g_app_settings.screen_timeout_sec <= 30) sel_idx = 1;
        else if (g_app_settings.screen_timeout_sec <= 60) sel_idx = 2;
        else if (g_app_settings.screen_timeout_sec <= 120) sel_idx = 3;
        else sel_idx = 4;
        lv_dropdown_set_selected(dropdown_timeout, sel_idx);
    }
    if (ta_wifi_ssid && strlen(g_app_settings.wifi_ssid) > 0) {
        lv_textarea_set_text(ta_wifi_ssid, g_app_settings.wifi_ssid);
    }
    if (ta_wifi_pass && strlen(g_app_settings.wifi_pass) > 0) {
        lv_textarea_set_text(ta_wifi_pass, g_app_settings.wifi_pass);
    }

    /* Muat Layar Utama sebagai tampilan default */
    lv_screen_load(screen_main);

    /* Terapkan tema sesuai pengaturan tersimpan */
    apply_theme(g_app_settings.theme_mode == 1);
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
    lv_obj_set_pos(badge_rfid, 135, 2);
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
    lv_obj_set_pos(badge_wifi, 218, 2);
    lv_obj_set_size(badge_wifi, 76, 24);
    lv_obj_set_style_bg_color(badge_wifi, lv_color_hex(0x450A0A), 0);
    lv_obj_set_style_border_color(badge_wifi, lv_color_hex(COLOR_RED_STATUS), 0);
    lv_obj_set_style_border_width(badge_wifi, 1, 0);
    lv_obj_set_style_radius(badge_wifi, 12, 0);
    lv_obj_set_style_pad_all(badge_wifi, 0, 0);
    lv_obj_remove_flag(badge_wifi, LV_OBJ_FLAG_SCROLLABLE);

    lbl_badge_wifi = lv_label_create(badge_wifi);
    lv_obj_center(lbl_badge_wifi);
    lv_label_set_text(lbl_badge_wifi, "WIFI: ERR");
    lv_obj_set_style_text_color(lbl_badge_wifi, lv_color_hex(COLOR_RED_STATUS), 0);
    lv_obj_set_style_text_font(lbl_badge_wifi, &lv_font_montserrat_14, 0);

    /* Badge Status SD Card */
    badge_sd = lv_obj_create(header_main);
    lv_obj_set_pos(badge_sd, 302, 2);
    lv_obj_set_size(badge_sd, 72, 24);
    lv_obj_set_style_bg_color(badge_sd, lv_color_hex(0x450A0A), 0);
    lv_obj_set_style_border_color(badge_sd, lv_color_hex(COLOR_RED_STATUS), 0);
    lv_obj_set_style_border_width(badge_sd, 1, 0);
    lv_obj_set_style_radius(badge_sd, 12, 0);
    lv_obj_set_style_pad_all(badge_sd, 0, 0);
    lv_obj_remove_flag(badge_sd, LV_OBJ_FLAG_SCROLLABLE);

    lbl_badge_sd = lv_label_create(badge_sd);
    lv_obj_center(lbl_badge_sd);
    lv_label_set_text(lbl_badge_sd, "SD: ERR");
    lv_obj_set_style_text_color(lbl_badge_sd, lv_color_hex(COLOR_RED_STATUS), 0);
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
    lv_obj_set_style_bg_color(screen_password, lv_color_hex(COLOR_DARK_BG), 0);
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

        btn_keypad[i] = lv_button_create(box_pin_container);
        lv_obj_set_pos(btn_keypad[i], start_x + col * (btn_w + gap_x), start_y + row * (btn_h + gap_y));
        lv_obj_set_size(btn_keypad[i], btn_w, btn_h);
        lv_obj_set_style_radius(btn_keypad[i], 8, 0);

        if (i == 9) {
            /* BATAL Button */
            lv_obj_set_style_bg_color(btn_keypad[i], lv_color_hex(0x450A0A), 0);
            lv_obj_set_style_border_color(btn_keypad[i], lv_color_hex(COLOR_RED_STATUS), 0);
        } else if (i == 11) {
            /* DEL Button */
            lv_obj_set_style_bg_color(btn_keypad[i], lv_color_hex(0x451A03), 0);
            lv_obj_set_style_border_color(btn_keypad[i], lv_color_hex(COLOR_AMBER_STATUS), 0);
        } else {
            /* Number Button */
            lv_obj_set_style_bg_color(btn_keypad[i], lv_color_hex(0x0F172A), 0);
            lv_obj_set_style_border_color(btn_keypad[i], lv_color_hex(COLOR_CYAN_ACCENT), 0);
        }
        lv_obj_set_style_border_width(btn_keypad[i], 1, 0);

        lbl_keypad[i] = lv_label_create(btn_keypad[i]);
        lv_obj_center(lbl_keypad[i]);
        lv_label_set_text(lbl_keypad[i], key_labels[i]);
        lv_obj_set_style_text_color(lbl_keypad[i], lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
        lv_obj_set_style_text_font(lbl_keypad[i], &lv_font_montserrat_16, 0);

        lv_obj_add_event_cb(btn_keypad[i], btn_keypad_click_cb, LV_EVENT_CLICKED, (void*)key_labels[i]);
    }
}

/**
  * @brief  Membangun Layar Pengaturan Admin (Tabview: Sistem, Wi-Fi, Log Absensi)
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
    lv_tabview_set_tab_bar_size(tabview_admin, 32);

    /* Matikan swipe gesture pada tabview content agar slider tidak memicu pergantian tab */
    lv_obj_t * tv_cont = lv_tabview_get_content(tabview_admin);
    if (tv_cont != NULL) {
        lv_obj_remove_flag(tv_cont, LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_t * tab_sistem = lv_tabview_add_tab(tabview_admin, "SISTEM");
    lv_obj_t * tab_wifi   = lv_tabview_add_tab(tabview_admin, "WI-FI");
    tab_log               = lv_tabview_add_tab(tabview_admin, "LOG ABSENSI");
    tab_test              = lv_tabview_add_tab(tabview_admin, "TEST DEVICE");

    /* ====================================================================== */
    /* TAB 1: SISTEM & DISPLAY                                                */
    /* ====================================================================== */
    lv_obj_set_style_pad_all(tab_sistem, 8, 0);

    /* Baris 1: Kecerahan Layar */
    lbl_bright_title = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_bright_title, 10, 6);
    lv_label_set_text(lbl_bright_title, "Kecerahan Layar:");
    lv_obj_set_style_text_color(lbl_bright_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_bright_title, &lv_font_montserrat_14, 0);

    slider_brightness = lv_slider_create(tab_sistem);
    lv_obj_set_pos(slider_brightness, 150, 8);
    lv_obj_set_size(slider_brightness, 180, 16);
    lv_slider_set_range(slider_brightness, 10, 100);
    lv_slider_set_value(slider_brightness, 75, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider_brightness, slider_brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lbl_bright_val = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_bright_val, 345, 6);
    lv_label_set_text(lbl_bright_val, "75%");
    lv_obj_set_style_text_color(lbl_bright_val, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_bright_val, &lv_font_montserrat_14, 0);

    /* Baris 2: Tema Tampilan */
    lbl_theme_title = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_theme_title, 10, 40);
    lv_label_set_text(lbl_theme_title, "Tema Tampilan:");
    lv_obj_set_style_text_color(lbl_theme_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_theme_title, &lv_font_montserrat_14, 0);

    sw_theme = lv_switch_create(tab_sistem);
    lv_obj_set_pos(sw_theme, 150, 36);
    lv_obj_add_event_cb(sw_theme, sw_theme_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lbl_theme_desc = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_theme_desc, 220, 40);
    lv_label_set_text(lbl_theme_desc, "Mode Gelap (Dark)");
    lv_obj_set_style_text_color(lbl_theme_desc, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_theme_desc, &lv_font_montserrat_14, 0);

    /* Baris 3: Timeout Layar (Sleep Mode / Hemat Daya Baterai) */
    lbl_timeout_title = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_timeout_title, 10, 76);
    lv_label_set_text(lbl_timeout_title, "Timeout Layar:");
    lv_obj_set_style_text_color(lbl_timeout_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_timeout_title, &lv_font_montserrat_14, 0);

    dropdown_timeout = lv_dropdown_create(tab_sistem);
    lv_obj_set_pos(dropdown_timeout, 150, 70);
    lv_obj_set_size(dropdown_timeout, 190, 32);
    lv_dropdown_set_options(dropdown_timeout, "Selalu Nyala\n30 Detik\n1 Menit\n2 Menit\n5 Menit");
    lv_dropdown_set_selected(dropdown_timeout, 2); /* Default 1 Menit */
    lv_obj_add_event_cb(dropdown_timeout, dropdown_timeout_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Baris 4: Tombol Aksi Hardware & Sinkronisasi */
    btn_test_beep = lv_button_create(tab_sistem);
    lv_obj_set_pos(btn_test_beep, 10, 114);
    lv_obj_set_size(btn_test_beep, 130, 36);
    lv_obj_set_style_bg_color(btn_test_beep, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_test_beep, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_test_beep, 1, 0);
    lv_obj_set_style_radius(btn_test_beep, 8, 0);
    lv_obj_add_event_cb(btn_test_beep, btn_test_beep_cb, LV_EVENT_CLICKED, NULL);

    lbl_t_beep = lv_label_create(btn_test_beep);
    lv_obj_center(lbl_t_beep);
    lv_label_set_text(lbl_t_beep, "TEST BEEP");
    lv_obj_set_style_text_color(lbl_t_beep, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_t_beep, &lv_font_montserrat_14, 0);

    btn_reset_stats = lv_button_create(tab_sistem);
    lv_obj_set_pos(btn_reset_stats, 155, 114);
    lv_obj_set_size(btn_reset_stats, 130, 36);
    lv_obj_set_style_bg_color(btn_reset_stats, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_reset_stats, lv_color_hex(COLOR_RED_STATUS), 0);
    lv_obj_set_style_border_width(btn_reset_stats, 1, 0);
    lv_obj_set_style_radius(btn_reset_stats, 8, 0);
    lv_obj_add_event_cb(btn_reset_stats, btn_reset_stats_cb, LV_EVENT_CLICKED, NULL);

    lbl_t_reset = lv_label_create(btn_reset_stats);
    lv_obj_center(lbl_t_reset);
    lv_label_set_text(lbl_t_reset, "RESET STATS");
    lv_obj_set_style_text_color(lbl_t_reset, lv_color_hex(COLOR_RED_STATUS), 0);
    lv_obj_set_style_text_font(lbl_t_reset, &lv_font_montserrat_14, 0);

    btn_ntp_sync = lv_button_create(tab_sistem);
    lv_obj_set_pos(btn_ntp_sync, 300, 114);
    lv_obj_set_size(btn_ntp_sync, 140, 36);
    lv_obj_set_style_bg_color(btn_ntp_sync, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_ntp_sync, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_border_width(btn_ntp_sync, 1, 0);
    lv_obj_set_style_radius(btn_ntp_sync, 8, 0);
    lv_obj_add_event_cb(btn_ntp_sync, btn_ntp_sync_cb, LV_EVENT_CLICKED, NULL);

    lbl_t_ntp = lv_label_create(btn_ntp_sync);
    lv_obj_center(lbl_t_ntp);
    lv_label_set_text(lbl_t_ntp, "SYNC JAM (NTP)");
    lv_obj_set_style_text_color(lbl_t_ntp, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_text_font(lbl_t_ntp, &lv_font_montserrat_14, 0);

    /* Baris 5: Label Feedback Aksi */
    lbl_admin_feedback = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_admin_feedback, 10, 160);
    lv_obj_set_width(lbl_admin_feedback, 440);
    lv_label_set_text(lbl_admin_feedback, "Status: Siap");
    lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_admin_feedback, &lv_font_montserrat_14, 0);

    /* ====================================================================== */
    /* TAB 2: KONFIGURASI WI-FI                                               */
    /* ====================================================================== */
    lv_obj_set_style_pad_all(tab_wifi, 8, 0);

    lbl_ssid_t = lv_label_create(tab_wifi);
    lv_obj_set_pos(lbl_ssid_t, 10, 8);
    lv_label_set_text(lbl_ssid_t, "SSID Wi-Fi:");
    lv_obj_set_style_text_color(lbl_ssid_t, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_ssid_t, &lv_font_montserrat_14, 0);

    ta_wifi_ssid = lv_textarea_create(tab_wifi);
    lv_obj_set_pos(ta_wifi_ssid, 100, 4);
    lv_obj_set_size(ta_wifi_ssid, 180, 32);
    lv_textarea_set_placeholder_text(ta_wifi_ssid, "Nama Wi-Fi");
    lv_textarea_set_one_line(ta_wifi_ssid, true);
    lv_obj_add_event_cb(ta_wifi_ssid, ta_wifi_focus_cb, LV_EVENT_ALL, NULL);

    lbl_pass_t = lv_label_create(tab_wifi);
    lv_obj_set_pos(lbl_pass_t, 10, 44);
    lv_label_set_text(lbl_pass_t, "Password:");
    lv_obj_set_style_text_color(lbl_pass_t, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_pass_t, &lv_font_montserrat_14, 0);

    ta_wifi_pass = lv_textarea_create(tab_wifi);
    lv_obj_set_pos(ta_wifi_pass, 100, 40);
    lv_obj_set_size(ta_wifi_pass, 180, 32);
    lv_textarea_set_password_mode(ta_wifi_pass, true);
    lv_textarea_set_placeholder_text(ta_wifi_pass, "Password Wi-Fi");
    lv_textarea_set_one_line(ta_wifi_pass, true);
    lv_obj_add_event_cb(ta_wifi_pass, ta_wifi_focus_cb, LV_EVENT_ALL, NULL);

    /* Tombol Simpan Wi-Fi */
    btn_save_wifi = lv_button_create(tab_wifi);
    lv_obj_set_pos(btn_save_wifi, 295, 4);
    lv_obj_set_size(btn_save_wifi, 145, 68);
    lv_obj_set_style_bg_color(btn_save_wifi, lv_color_hex(0x064E3B), 0);
    lv_obj_set_style_border_color(btn_save_wifi, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_border_width(btn_save_wifi, 1, 0);
    lv_obj_set_style_radius(btn_save_wifi, 8, 0);
    lv_obj_add_event_cb(btn_save_wifi, btn_save_wifi_cb, LV_EVENT_CLICKED, NULL);

    lbl_save_btn = lv_label_create(btn_save_wifi);
    lv_obj_center(lbl_save_btn);
    lv_label_set_text(lbl_save_btn, "SIMPAN & KIRIM\nKE ESP32");
    lv_obj_set_style_text_align(lbl_save_btn, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_save_btn, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_text_font(lbl_save_btn, &lv_font_montserrat_14, 0);

    /* Keyboard Virtual ditempatkan di lv_layer_top() dengan custom 5-baris QWERTY lengkap */
    kb_wifi = lv_keyboard_create(lv_layer_top());
    lv_obj_set_size(kb_wifi, 480, 155);
    lv_obj_align(kb_wifi, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_pad_all(kb_wifi, 2, 0);
    lv_keyboard_set_map(kb_wifi, LV_KEYBOARD_MODE_TEXT_LOWER, (const char **)custom_kb_map_lc, custom_kb_ctrl_lc_map);
    lv_keyboard_set_map(kb_wifi, LV_KEYBOARD_MODE_TEXT_UPPER, (const char **)custom_kb_map_uc, custom_kb_ctrl_uc_map);
    lv_keyboard_set_map(kb_wifi, LV_KEYBOARD_MODE_SPECIAL,    (const char **)custom_kb_map_spec, custom_kb_ctrl_spec_map);
    lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb_wifi, kb_wifi_event_cb, LV_EVENT_ALL, NULL);

    /* ====================================================================== */
    /* TAB 3: LOG ABSENSI MICROSD                                             */
    /* ====================================================================== */
    lv_obj_remove_flag(tab_log, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tab_log, 6, 0);

    lbl_log_title = lv_label_create(tab_log);
    lv_obj_set_pos(lbl_log_title, 10, 6);
    lv_label_set_text(lbl_log_title, "Riwayat Kehadiran (MicroSD):");
    lv_obj_set_style_text_color(lbl_log_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_log_title, &lv_font_montserrat_14, 0);

    btn_refresh_log = lv_button_create(tab_log);
    lv_obj_set_pos(btn_refresh_log, 310, 2);
    lv_obj_set_size(btn_refresh_log, 130, 28);
    lv_obj_set_style_bg_color(btn_refresh_log, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_refresh_log, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_refresh_log, 1, 0);
    lv_obj_set_style_radius(btn_refresh_log, 6, 0);
    lv_obj_add_event_cb(btn_refresh_log, btn_refresh_log_cb, LV_EVENT_CLICKED, NULL);

    lbl_refresh_log = lv_label_create(btn_refresh_log);
    lv_obj_center(lbl_refresh_log);
    lv_label_set_text(lbl_refresh_log, "REFRESH LOG");
    lv_obj_set_style_text_color(lbl_refresh_log, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_refresh_log, &lv_font_montserrat_14, 0);

    /* Kotak log tabel: scrollable hanya pada container ini (1 Scroll Control) */
    cont_log_list = lv_obj_create(tab_log);
    lv_obj_set_pos(cont_log_list, 10, 36);
    lv_obj_set_size(cont_log_list, 440, 194);
    lv_obj_set_style_bg_color(cont_log_list, lv_color_hex(0x060E1A), 0);
    lv_obj_set_style_border_color(cont_log_list, lv_color_hex(COLOR_DARK_BORDER), 0);
    lv_obj_set_style_border_width(cont_log_list, 1, 0);
    lv_obj_set_style_radius(cont_log_list, 8, 0);
    lv_obj_set_style_pad_all(cont_log_list, 2, 0);
    lv_obj_add_flag(cont_log_list, LV_OBJ_FLAG_SCROLLABLE);

    table_log = lv_table_create(cont_log_list);
    lv_obj_remove_flag(table_log, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(table_log, &lv_font_montserrat_14, 0);
    lv_table_set_column_count(table_log, 4);
    lv_table_set_column_width(table_log, 0, 36);   /* No */
    lv_table_set_column_width(table_log, 1, 140);  /* Waktu */
    lv_table_set_column_width(table_log, 2, 130);  /* UID Kartu */
    lv_table_set_column_width(table_log, 3, 110);  /* Status */

    lv_table_set_cell_value(table_log, 0, 0, "#");
    lv_table_set_cell_value(table_log, 0, 1, "WAKTU");
    lv_table_set_cell_value(table_log, 0, 2, "UID KARTU");
    lv_table_set_cell_value(table_log, 0, 3, "STATUS");

    /* ====================================================================== */
    /* TAB 4: TEST DEVICE (DIAGNOSTIK HARDWARE LENGKAP)                       */
    /* ====================================================================== */
    lv_obj_remove_flag(tab_test, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tab_test, 6, 0);

    /* Tombol Uji Semua (Master Test) */
    btn_diag_all = lv_button_create(tab_test);
    lv_obj_set_pos(btn_diag_all, 6, 4);
    lv_obj_set_size(btn_diag_all, 140, 32);
    lv_obj_set_style_bg_color(btn_diag_all, lv_color_hex(0x0E7490), 0);
    lv_obj_set_style_border_color(btn_diag_all, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_diag_all, 1, 0);
    lv_obj_set_style_radius(btn_diag_all, 6, 0);
    lv_obj_add_event_cb(btn_diag_all, btn_diag_all_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_diag_all_t = lv_label_create(btn_diag_all);
    lv_obj_center(lbl_diag_all_t);
    lv_label_set_text(lbl_diag_all_t, ">> UJI SEMUA");
    lv_obj_set_style_text_color(lbl_diag_all_t, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_diag_all_t, &lv_font_montserrat_14, 0);

    /* Tombol 1: Test Buzzer */
    btn_diag_beep = lv_button_create(tab_test);
    lv_obj_set_pos(btn_diag_beep, 6, 40);
    lv_obj_set_size(btn_diag_beep, 140, 30);
    lv_obj_set_style_bg_color(btn_diag_beep, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_diag_beep, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_diag_beep, 1, 0);
    lv_obj_set_style_radius(btn_diag_beep, 6, 0);
    lv_obj_add_event_cb(btn_diag_beep, btn_diag_beep_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_db = lv_label_create(btn_diag_beep);
    lv_obj_center(lbl_db);
    lv_label_set_text(lbl_db, "1. Test Buzzer");
    lv_obj_set_style_text_color(lbl_db, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_db, &lv_font_montserrat_14, 0);

    /* Tombol 2: Test RFID */
    btn_diag_rfid = lv_button_create(tab_test);
    lv_obj_set_pos(btn_diag_rfid, 6, 74);
    lv_obj_set_size(btn_diag_rfid, 140, 30);
    lv_obj_set_style_bg_color(btn_diag_rfid, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_diag_rfid, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_diag_rfid, 1, 0);
    lv_obj_set_style_radius(btn_diag_rfid, 6, 0);
    lv_obj_add_event_cb(btn_diag_rfid, btn_diag_rfid_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_dr = lv_label_create(btn_diag_rfid);
    lv_obj_center(lbl_dr);
    lv_label_set_text(lbl_dr, "2. Test RFID");
    lv_obj_set_style_text_color(lbl_dr, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_dr, &lv_font_montserrat_14, 0);

    /* Tombol 3: Test MicroSD */
    btn_diag_sd = lv_button_create(tab_test);
    lv_obj_set_pos(btn_diag_sd, 6, 108);
    lv_obj_set_size(btn_diag_sd, 140, 30);
    lv_obj_set_style_bg_color(btn_diag_sd, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_diag_sd, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_diag_sd, 1, 0);
    lv_obj_set_style_radius(btn_diag_sd, 6, 0);
    lv_obj_add_event_cb(btn_diag_sd, btn_diag_sd_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_ds = lv_label_create(btn_diag_sd);
    lv_obj_center(lbl_ds);
    lv_label_set_text(lbl_ds, "3. Test MicroSD");
    lv_obj_set_style_text_color(lbl_ds, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_ds, &lv_font_montserrat_14, 0);

    /* Tombol 4: Test Wi-Fi */
    btn_diag_wifi = lv_button_create(tab_test);
    lv_obj_set_pos(btn_diag_wifi, 6, 142);
    lv_obj_set_size(btn_diag_wifi, 140, 30);
    lv_obj_set_style_bg_color(btn_diag_wifi, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_diag_wifi, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_diag_wifi, 1, 0);
    lv_obj_set_style_radius(btn_diag_wifi, 6, 0);
    lv_obj_add_event_cb(btn_diag_wifi, btn_diag_wifi_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_dw = lv_label_create(btn_diag_wifi);
    lv_obj_center(lbl_dw);
    lv_label_set_text(lbl_dw, "4. Test Wi-Fi");
    lv_obj_set_style_text_color(lbl_dw, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_dw, &lv_font_montserrat_14, 0);

    /* Tombol 5: Test RTC */
    btn_diag_rtc = lv_button_create(tab_test);
    lv_obj_set_pos(btn_diag_rtc, 6, 176);
    lv_obj_set_size(btn_diag_rtc, 140, 30);
    lv_obj_set_style_bg_color(btn_diag_rtc, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_diag_rtc, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_diag_rtc, 1, 0);
    lv_obj_set_style_radius(btn_diag_rtc, 6, 0);
    lv_obj_add_event_cb(btn_diag_rtc, btn_diag_rtc_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_dr_rtc = lv_label_create(btn_diag_rtc);
    lv_obj_center(lbl_dr_rtc);
    lv_label_set_text(lbl_dr_rtc, "5. Test RTC");
    lv_obj_set_style_text_color(lbl_dr_rtc, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_dr_rtc, &lv_font_montserrat_14, 0);

    /* Kotak Hasil Diagnostik (Kanan) */
    cont_diag_box = lv_obj_create(tab_test);
    lv_obj_set_pos(cont_diag_box, 155, 4);
    lv_obj_set_size(cont_diag_box, 295, 202);
    lv_obj_set_style_bg_color(cont_diag_box, lv_color_hex(0x060E1A), 0);
    lv_obj_set_style_border_color(cont_diag_box, lv_color_hex(COLOR_DARK_BORDER), 0);
    lv_obj_set_style_border_width(cont_diag_box, 1, 0);
    lv_obj_set_style_radius(cont_diag_box, 8, 0);
    lv_obj_set_style_pad_all(cont_diag_box, 6, 0);

    lbl_diag_title = lv_label_create(cont_diag_box);
    lv_obj_set_pos(lbl_diag_title, 4, 2);
    lv_label_set_text(lbl_diag_title, "HASIL DIAGNOSTIK:");
    lv_obj_set_style_text_color(lbl_diag_title, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_diag_title, &lv_font_montserrat_14, 0);

    lbl_diag_summary = lv_label_create(cont_diag_box);
    lv_obj_set_pos(lbl_diag_summary, 4, 24);
    lv_obj_set_width(lbl_diag_summary, 275);
    lv_label_set_long_mode(lbl_diag_summary, LV_LABEL_LONG_MODE_WRAP);
    lv_label_set_text(lbl_diag_summary, "Silakan pilih salah satu tes atau klik 'UJI SEMUA'.");
    lv_obj_set_style_text_color(lbl_diag_summary, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_diag_summary, &lv_font_montserrat_14, 0);

    lbl_diag_detail = lv_label_create(cont_diag_box);
    lv_obj_set_pos(lbl_diag_detail, 4, 60);
    lv_obj_set_width(lbl_diag_detail, 275);
    lv_label_set_long_mode(lbl_diag_detail, LV_LABEL_LONG_MODE_WRAP);
    lv_label_set_text(lbl_diag_detail, "Pengujian memeriksa respon hardware, konektivitas kabel, register chip, dan file system.");
    lv_obj_set_style_text_color(lbl_diag_detail, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_diag_detail, &lv_font_montserrat_14, 0);
}

/**
  * @brief  Proses event kedatangan data RFID / Verifikasi Server
  */
void GUI_Attendance_ProcessEvent(const ui_event_t *evt)
{
    if (evt == NULL || screen_main == NULL) return;

    /* Setiap ada event absensi, bangunkan layar jika tidur */
    GUI_Attendance_Wakeup();

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
        if (ntp_timeout_timer) {
            lv_timer_delete(ntp_timeout_timer);
            ntp_timeout_timer = NULL;
        }
        if (lbl_admin_feedback) {
            lv_label_set_text_fmt(lbl_admin_feedback, "Status: Jam Berhasil Disinkronkan! (%s)", evt->extra);
            lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_GREEN_STATUS), 0);
        }
        /* Segera perbarui jam di header */
        RTC_TimeTypeDef sTime = {0};
        RTC_DateTypeDef sDate = {0};
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        GUI_Attendance_UpdateClock(&sTime, &sDate);
    }
    else if (evt->type == UI_EVENT_NTP_FAILED)
    {
        if (ntp_timeout_timer) {
            lv_timer_delete(ntp_timeout_timer);
            ntp_timeout_timer = NULL;
        }
        if (lbl_admin_feedback) {
            lv_label_set_text_fmt(lbl_admin_feedback, "Gagal NTP: %s", evt->extra);
            lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
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
  * @brief  Cek dan eksekusi screen sleep/dim jika idle timeout tercapai
  */
void GUI_Attendance_CheckSleep(void)
{
    if (g_screen_timeout_sec > 0 && !g_screen_is_sleeping) {
        if (HAL_GetTick() - g_last_activity_tick >= (g_screen_timeout_sec * 1000)) {
            g_screen_is_sleeping = true;
            /* Matikan backlight PWM */
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
            /* Masukkan display controller ILI9488 ke Sleep Mode (menghemat daya ~30mA) */
            ILI9488_SleepIn();
        }
    }
}

/**
  * @brief  Bangunkan layar dari mode sleep
  */
void GUI_Attendance_Wakeup(void)
{
    if (g_screen_is_sleeping) {
        /* Bangunkan display controller ILI9488 */
        ILI9488_SleepOut();
        g_screen_is_sleeping = false;
        g_last_activity_tick = HAL_GetTick();
        uint32_t compare = (g_current_brightness_val * 65535) / 100;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, compare);
    }
}

/**
  * @brief  Periksa apakah layar sedang dalam kondisi sleep
  */
bool GUI_Attendance_IsSleeping(void)
{
    return g_screen_is_sleeping;
}

/**
  * @brief  Reset timer aktivitas layar sentuh
  */
void GUI_Attendance_NotifyActivity(void)
{
    g_last_activity_tick = HAL_GetTick();
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
    uint32_t c_btn_bg   = is_dark ? COLOR_DARK_CARD     : 0xFFFFFF;

    /* 1. Layar Utama */
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

    /* 2. Layar Password (PIN Screen) */
    if (screen_password) lv_obj_set_style_bg_color(screen_password, lv_color_hex(c_bg), 0);
    if (box_pin_container) {
        lv_obj_set_style_bg_color(box_pin_container, lv_color_hex(c_card), 0);
        lv_obj_set_style_border_color(box_pin_container, lv_color_hex(is_dark ? COLOR_CYAN_ACCENT : c_border), 0);
    }
    if (lbl_pin_title) lv_obj_set_style_text_color(lbl_pin_title, lv_color_hex(is_dark ? COLOR_CYAN_TEXT : COLOR_CYAN_ACCENT), 0);
    if (lbl_pin_subtitle) lv_obj_set_style_text_color(lbl_pin_subtitle, lv_color_hex(c_text_mut), 0);
    if (box_pin_display) {
        lv_obj_set_style_bg_color(box_pin_display, lv_color_hex(is_dark ? 0x060E1A : 0xF8FAFC), 0);
        lv_obj_set_style_border_color(box_pin_display, lv_color_hex(is_dark ? COLOR_CYAN_ACCENT : c_border), 0);
    }
    if (lbl_pin_display) lv_obj_set_style_text_color(lbl_pin_display, lv_color_hex(c_text_pri), 0);

    for (int i = 0; i < 12; i++) {
        if (btn_keypad[i]) {
            if (i == 9) {
                /* BATAL */
                lv_obj_set_style_bg_color(btn_keypad[i], lv_color_hex(is_dark ? 0x450A0A : 0xFEE2E2), 0);
                lv_obj_set_style_border_color(btn_keypad[i], lv_color_hex(COLOR_RED_STATUS), 0);
                if (lbl_keypad[i]) lv_obj_set_style_text_color(lbl_keypad[i], lv_color_hex(is_dark ? 0xF8FAFC : 0xDC2626), 0);
            } else if (i == 11) {
                /* DEL */
                lv_obj_set_style_bg_color(btn_keypad[i], lv_color_hex(is_dark ? 0x451A03 : 0xFEF3C7), 0);
                lv_obj_set_style_border_color(btn_keypad[i], lv_color_hex(COLOR_AMBER_STATUS), 0);
                if (lbl_keypad[i]) lv_obj_set_style_text_color(lbl_keypad[i], lv_color_hex(is_dark ? 0xF8FAFC : 0xD97706), 0);
            } else {
                /* Angka 0-9 */
                lv_obj_set_style_bg_color(btn_keypad[i], lv_color_hex(is_dark ? 0x0F172A : 0xFFFFFF), 0);
                lv_obj_set_style_border_color(btn_keypad[i], lv_color_hex(is_dark ? COLOR_CYAN_ACCENT : c_border), 0);
                if (lbl_keypad[i]) lv_obj_set_style_text_color(lbl_keypad[i], lv_color_hex(c_text_pri), 0);
            }
        }
    }

    /* 3. Layar Admin */
    if (screen_admin) lv_obj_set_style_bg_color(screen_admin, lv_color_hex(c_bg), 0);
    if (header_admin) lv_obj_set_style_bg_color(header_admin, lv_color_hex(is_dark ? 0x111827 : 0xFFFFFF), 0);
    if (lbl_admin_title) lv_obj_set_style_text_color(lbl_admin_title, lv_color_hex(c_text_pri), 0);
    if (btn_admin_back) {
        lv_obj_set_style_bg_color(btn_admin_back, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(btn_admin_back, lv_color_hex(is_dark ? COLOR_CYAN_ACCENT : c_border), 0);
    }
    if (tabview_admin) {
        lv_obj_set_style_bg_color(tabview_admin, lv_color_hex(c_bg), 0);
        update_admin_tab_styles(is_dark);
    }

    /* Tab 1 Elements */
    if (lbl_bright_title) lv_obj_set_style_text_color(lbl_bright_title, lv_color_hex(c_text_pri), 0);
    if (lbl_theme_title) lv_obj_set_style_text_color(lbl_theme_title, lv_color_hex(c_text_pri), 0);
    if (lbl_timeout_title) lv_obj_set_style_text_color(lbl_timeout_title, lv_color_hex(c_text_pri), 0);
    if (dropdown_timeout) {
        lv_obj_set_style_bg_color(dropdown_timeout, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(dropdown_timeout, lv_color_hex(c_border), 0);
        lv_obj_set_style_text_color(dropdown_timeout, lv_color_hex(c_text_pri), 0);
    }
    if (lbl_theme_desc) {
        lv_label_set_text(lbl_theme_desc, is_dark ? "Mode Gelap (Dark)" : "Mode Terang (Light)");
        lv_obj_set_style_text_color(lbl_theme_desc, lv_color_hex(c_text_mut), 0);
    }
    if (btn_test_beep) {
        lv_obj_set_style_bg_color(btn_test_beep, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(btn_test_beep, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    }
    if (btn_reset_stats) {
        lv_obj_set_style_bg_color(btn_reset_stats, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(btn_reset_stats, lv_color_hex(COLOR_RED_STATUS), 0);
    }
    if (btn_ntp_sync) {
        lv_obj_set_style_bg_color(btn_ntp_sync, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(btn_ntp_sync, lv_color_hex(COLOR_GREEN_STATUS), 0);
    }

    /* Tab 2 Elements */
    if (lbl_ssid_t) lv_obj_set_style_text_color(lbl_ssid_t, lv_color_hex(c_text_pri), 0);
    if (lbl_pass_t) lv_obj_set_style_text_color(lbl_pass_t, lv_color_hex(c_text_pri), 0);
    if (ta_wifi_ssid) {
        lv_obj_set_style_bg_color(ta_wifi_ssid, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(ta_wifi_ssid, lv_color_hex(c_border), 0);
        lv_obj_set_style_text_color(ta_wifi_ssid, lv_color_hex(c_text_pri), 0);
    }
    if (ta_wifi_pass) {
        lv_obj_set_style_bg_color(ta_wifi_pass, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(ta_wifi_pass, lv_color_hex(c_border), 0);
        lv_obj_set_style_text_color(ta_wifi_pass, lv_color_hex(c_text_pri), 0);
    }

    /* Tab 3 Elements (Log Absensi) */
    if (lbl_log_title) lv_obj_set_style_text_color(lbl_log_title, lv_color_hex(c_text_pri), 0);
    if (btn_refresh_log) {
        lv_obj_set_style_bg_color(btn_refresh_log, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(btn_refresh_log, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    }
    if (cont_log_list) {
        lv_obj_set_style_bg_color(cont_log_list, lv_color_hex(is_dark ? 0x060E1A : 0xFFFFFF), 0);
        lv_obj_set_style_border_color(cont_log_list, lv_color_hex(c_border), 0);
    }
    if (table_log) {
        lv_obj_set_style_text_color(table_log, lv_color_hex(c_text_pri), 0);
    }

    /* Tab 4 Elements (Test Device) */
    if (cont_diag_box) {
        lv_obj_set_style_bg_color(cont_diag_box, lv_color_hex(is_dark ? 0x060E1A : 0xFFFFFF), 0);
        lv_obj_set_style_border_color(cont_diag_box, lv_color_hex(c_border), 0);
    }
    if (lbl_diag_title) lv_obj_set_style_text_color(lbl_diag_title, lv_color_hex(COLOR_CYAN_TEXT), 0);
    if (lbl_diag_summary) lv_obj_set_style_text_color(lbl_diag_summary, lv_color_hex(c_text_pri), 0);
    if (lbl_diag_detail) lv_obj_set_style_text_color(lbl_diag_detail, lv_color_hex(c_text_mut), 0);

    if (btn_diag_beep) lv_obj_set_style_bg_color(btn_diag_beep, lv_color_hex(c_btn_bg), 0);
    if (btn_diag_rfid) lv_obj_set_style_bg_color(btn_diag_rfid, lv_color_hex(c_btn_bg), 0);
    if (btn_diag_sd) lv_obj_set_style_bg_color(btn_diag_sd, lv_color_hex(c_btn_bg), 0);
    if (btn_diag_wifi) lv_obj_set_style_bg_color(btn_diag_wifi, lv_color_hex(c_btn_bg), 0);
    if (btn_diag_rtc) lv_obj_set_style_bg_color(btn_diag_rtc, lv_color_hex(c_btn_bg), 0);
}

/**
  * @brief  Pengaturan gaya Tab Button Admin agar proporsional dan selaras dengan tema
  */
static void update_admin_tab_styles(bool is_dark)
{
    if (tabview_admin == NULL) return;
    lv_obj_t * tab_bar = lv_tabview_get_tab_bar(tabview_admin);
    if (tab_bar == NULL) return;

    uint32_t bar_bg = is_dark ? 0x0F172A : 0xE2E8F0;
    lv_obj_set_style_bg_color(tab_bar, lv_color_hex(bar_bg), 0);
    lv_obj_set_style_pad_all(tab_bar, 2, 0);

    uint32_t count = lv_obj_get_child_count(tab_bar);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t * btn = lv_obj_get_child(tab_bar, i);
        if (btn == NULL) continue;

        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_pad_ver(btn, 4, 0);
        lv_obj_set_style_pad_hor(btn, 8, 0);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_14, 0);

        /* DEFAULT STATE (Unchecked) */
        if (is_dark) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E293B), LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(btn, lv_color_hex(0x94A3B8), LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x334155), LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xF1F5F9), LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(btn, lv_color_hex(0x475569), LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(btn, lv_color_hex(0xCBD5E1), LV_STATE_DEFAULT);
        }
        lv_obj_set_style_border_width(btn, 1, LV_STATE_DEFAULT);

        /* CHECKED STATE (Selected Tab) */
        if (is_dark) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_CYAN_ACCENT), LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), LV_STATE_CHECKED);
            lv_obj_set_style_border_color(btn, lv_color_hex(COLOR_CYAN_ACCENT), LV_STATE_CHECKED);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x0284C7), LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), LV_STATE_CHECKED);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x0284C7), LV_STATE_CHECKED);
        }
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

static void ntp_timeout_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    ntp_timeout_timer = NULL;
    if (lbl_admin_feedback) {
        lv_label_set_text(lbl_admin_feedback, "Error: Timeout! Gagal membaca server NTP");
        lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
    }
}

static void load_recent_logs(void)
{
    if (table_log == NULL) return;

    char log_buffer[12][64];
    uint16_t n = Storage_ReadRecentLogs(log_buffer, 12);

    /* Reset row count: Baris 0 adalah Header */
    lv_table_set_row_count(table_log, n > 0 ? (n + 1) : 2);
    lv_table_set_cell_value(table_log, 0, 0, "#");
    lv_table_set_cell_value(table_log, 0, 1, "WAKTU");
    lv_table_set_cell_value(table_log, 0, 2, "UID KARTU");
    lv_table_set_cell_value(table_log, 0, 3, "STATUS");

    if (n == 0) {
        lv_table_set_cell_value(table_log, 1, 0, "-");
        lv_table_set_cell_value(table_log, 1, 1, "--:--:--");
        lv_table_set_cell_value(table_log, 1, 2, "TIDAK ADA DATA");
        lv_table_set_cell_value(table_log, 1, 3, "KOSONG/ERR");
        return;
    }

    /* Format baris CSV: 2026-10-17 19:24:00,UID:12345678,LOG=TAP */
    for (uint16_t i = 0; i < n; i++) {
        uint16_t row = i + 1;
        char num_str[8];
        snprintf(num_str, sizeof(num_str), "%d", row);
        lv_table_set_cell_value(table_log, row, 0, num_str);

        char line[64];
        strncpy(line, log_buffer[n - 1 - i], sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';

        char *token_time = strtok(line, ",");
        char *token_uid  = strtok(NULL, ",");
        char *token_stat = strtok(NULL, ",");

        lv_table_set_cell_value(table_log, row, 1, token_time ? token_time : "-");
        lv_table_set_cell_value(table_log, row, 2, token_uid ? token_uid : "-");
        lv_table_set_cell_value(table_log, row, 3, token_stat ? token_stat : "OK");
    }
}

static void btn_refresh_log_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    load_recent_logs();
}

static void btn_keypad_click_cb(lv_event_t * e)
{
    GUI_Attendance_NotifyActivity();
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
            load_recent_logs();
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
    GUI_Attendance_NotifyActivity();
    int32_t val = lv_slider_get_value(slider_brightness);
    g_current_brightness_val = (uint32_t)val;
    uint32_t compare = (val * 65535) / 100;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, compare);
    lv_label_set_text_fmt(lbl_bright_val, "%ld%%", (long)val);

    /* Simpan ke Flash Sector 11 */
    g_app_settings.brightness = (uint8_t)val;
    App_Settings_Save(&g_app_settings);
}

static void sw_theme_cb(lv_event_t * e)
{
    GUI_Attendance_NotifyActivity();
    bool is_checked = lv_obj_has_state(sw_theme, LV_STATE_CHECKED);
    apply_theme(!is_checked); /* Checked = Terang, Unchecked = Gelap */

    /* Simpan ke Flash Sector 11 */
    g_app_settings.theme_mode = !is_checked ? 1 : 0;
    App_Settings_Save(&g_app_settings);
}

static void dropdown_timeout_cb(lv_event_t * e)
{
    GUI_Attendance_NotifyActivity();
    uint32_t sel = lv_dropdown_get_selected(dropdown_timeout);
    switch (sel) {
        case 0: g_screen_timeout_sec = 0; break;    /* Selalu Nyala (OFF) */
        case 1: g_screen_timeout_sec = 30; break;   /* 30 Detik */
        case 2: g_screen_timeout_sec = 60; break;   /* 1 Menit */
        case 3: g_screen_timeout_sec = 120; break;  /* 2 Menit */
        case 4: g_screen_timeout_sec = 300; break;  /* 5 Menit */
        default: g_screen_timeout_sec = 60; break;
    }

    /* Simpan ke Flash Sector 11 */
    g_app_settings.screen_timeout_sec = (uint16_t)g_screen_timeout_sec;
    App_Settings_Save(&g_app_settings);
}

static void btn_admin_back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    if (kb_wifi) lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
    lv_screen_load(screen_main);
}

static void btn_test_beep_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    Buzzer_TestDiagnostic();
    if (lbl_admin_feedback) {
        lv_label_set_text(lbl_admin_feedback, "Status: Test Beep Berhasil");
        lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_CYAN_TEXT), 0);
    }
}

static void btn_reset_stats_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    GUI_Attendance_ResetCounter();
    if (lbl_admin_feedback) {
        lv_label_set_text(lbl_admin_feedback, "Status: Counter Absensi Direset ke 0");
        lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_AMBER_STATUS), 0);
    }
}

static void btn_ntp_sync_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();

    /* Kirim request sinkronisasi jam ke ESP32 */
    esp_tx_item_t item = {0};
    snprintf(item.payload, sizeof(item.payload), "CMD:SYNC_TIME\n");
    osMessageQueuePut(espTxQueueHandle, &item, 0, 50);

    if (lbl_admin_feedback) {
        lv_label_set_text(lbl_admin_feedback, "Status: Menghubungi Server NTP...");
        lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_AMBER_STATUS), 0);
    }

    if (ntp_timeout_timer) lv_timer_delete(ntp_timeout_timer);
    ntp_timeout_timer = lv_timer_create(ntp_timeout_cb, 12000, NULL);
    lv_timer_set_repeat_count(ntp_timeout_timer, 1);
}

static void btn_save_wifi_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    const char * ssid = lv_textarea_get_text(ta_wifi_ssid);
    const char * pass = lv_textarea_get_text(ta_wifi_pass);

    if (strlen(ssid) == 0) {
        if (lbl_admin_feedback) {
            lv_label_set_text(lbl_admin_feedback, "Peringatan: SSID tidak boleh kosong!");
            lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
        }
        return;
    }

    /* Simpan ke Flash Sector 11 */
    strncpy(g_app_settings.wifi_ssid, ssid, sizeof(g_app_settings.wifi_ssid) - 1);
    strncpy(g_app_settings.wifi_pass, pass, sizeof(g_app_settings.wifi_pass) - 1);
    App_Settings_Save(&g_app_settings);

    /* Kirim paket konfigurasi Wi-Fi ke ESP32-C6 via USART1 */
    esp_tx_item_t item = {0};
    snprintf(item.payload, sizeof(item.payload), "WIFI:%s|%s|0.0.0.0\n", ssid, pass);
    osMessageQueuePut(espTxQueueHandle, &item, 0, 50);

    /* Sembunyikan keyboard */
    if (kb_wifi) lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);

    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
    osDelay(50);
    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);

    if (lbl_admin_feedback) {
        lv_label_set_text(lbl_admin_feedback, "Status: Konfigurasi Disimpan ke Flash & Dikirim!");
        lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_GREEN_STATUS), 0);
    }
}

static void ta_wifi_focus_cb(lv_event_t * e)
{
    GUI_Attendance_NotifyActivity();
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        if (kb_wifi) {
            lv_keyboard_set_textarea(kb_wifi, ta);
            lv_obj_clear_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(kb_wifi);
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        /* Biarkan keyboard aktif sampai tombol OK atau CANCEL ditekan */
    }
}

static void kb_wifi_event_cb(lv_event_t * e)
{
    GUI_Attendance_NotifyActivity();
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        if (kb_wifi) lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ========================================================================== */
/*                       DIAGNOSTIC TEST CALLBACKS (TAB 4)                    */
/* ========================================================================== */

static void btn_diag_beep_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    Buzzer_TestDiagnostic();
    if (lbl_diag_summary) {
        lv_label_set_text(lbl_diag_summary, "Buzzer: AKTIF [OK]");
        lv_obj_set_style_text_color(lbl_diag_summary, lv_color_hex(COLOR_GREEN_STATUS), 0);
    }
    if (lbl_diag_detail) {
        lv_label_set_text(lbl_diag_detail, "Buzzer berbunyi nada ganda. Jalur GPIO PB8 dan transistor driver normal.");
        lv_obj_set_style_text_color(lbl_diag_detail, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    }
}

static void btn_diag_rfid_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    char msg[128] = {0};
    bool ok = RFID_TestDiagnostic(msg, sizeof(msg));
    if (lbl_diag_summary) {
        lv_label_set_text(lbl_diag_summary, ok ? "RFID CLRC663: NORMAL [OK]" : "RFID CLRC663: GAGAL [ERROR]");
        lv_obj_set_style_text_color(lbl_diag_summary, lv_color_hex(ok ? COLOR_GREEN_STATUS : COLOR_RED_STATUS), 0);
    }
    if (lbl_diag_detail) {
        if (ok) {
            lv_label_set_text(lbl_diag_detail, msg);
            lv_obj_set_style_text_color(lbl_diag_detail, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
        } else {
            lv_label_set_text(lbl_diag_detail, "Chip RFID tidak merespon SPI3. Solusi:\n1. Cek jalur SPI3 (PB3, PB4, PB5, CS PD2).\n2. Pastikan pin 3.3V dan GND terhubung rapat.");
            lv_obj_set_style_text_color(lbl_diag_detail, lv_color_hex(COLOR_RED_STATUS), 0);
        }
    }
}

static void btn_diag_sd_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    char msg[128] = {0};
    bool ok = Storage_TestDiagnostic(msg, sizeof(msg));
    if (lbl_diag_summary) {
        lv_label_set_text(lbl_diag_summary, ok ? "MicroSD FatFS: NORMAL [OK]" : "MicroSD FatFS: GAGAL [ERROR]");
        lv_obj_set_style_text_color(lbl_diag_summary, lv_color_hex(ok ? COLOR_GREEN_STATUS : COLOR_RED_STATUS), 0);
    }
    if (lbl_diag_detail) {
        if (ok) {
            lv_label_set_text(lbl_diag_detail, msg);
            lv_obj_set_style_text_color(lbl_diag_detail, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
        } else {
            lv_label_set_text(lbl_diag_detail, "SD Card tidak dapat dibaca/tulis. Solusi:\n1. Pastikan MicroSD terpasang rapat di slot.\n2. Format ulang MicroSD dengan sistem file FAT32.\n3. Gunakan kapasitas maksimal 32GB.");
            lv_obj_set_style_text_color(lbl_diag_detail, lv_color_hex(COLOR_RED_STATUS), 0);
        }
    }
}

static void btn_diag_wifi_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    char msg[128] = {0};
    bool ok = WiFi_GetDiagnostic(msg, sizeof(msg));
    if (lbl_diag_summary) {
        lv_label_set_text(lbl_diag_summary, ok ? "Wi-Fi ESP32: TERHUBUNG [OK]" : "Wi-Fi ESP32: PERINGATAN");
        lv_obj_set_style_text_color(lbl_diag_summary, lv_color_hex(ok ? COLOR_GREEN_STATUS : COLOR_AMBER_STATUS), 0);
    }
    if (lbl_diag_detail) {
        if (ok) {
            lv_label_set_text(lbl_diag_detail, msg);
            lv_obj_set_style_text_color(lbl_diag_detail, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
        } else {
            lv_label_set_text(lbl_diag_detail, "Koneksi ke Wi-Fi belum aktif. Solusi:\n1. Buka tab WI-FI dan periksa nama SSID & Password.\n2. Pastikan router Wi-Fi dalam jangkauan sinyal.");
            lv_obj_set_style_text_color(lbl_diag_detail, lv_color_hex(COLOR_AMBER_STATUS), 0);
        }
    }
}

static void btn_diag_rtc_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    char msg[128] = {0};
    RTC_GetDiagnostic(msg, sizeof(msg));
    if (lbl_diag_summary) {
        lv_label_set_text(lbl_diag_summary, "RTC Clock: AKTIF [OK]");
        lv_obj_set_style_text_color(lbl_diag_summary, lv_color_hex(COLOR_GREEN_STATUS), 0);
    }
    if (lbl_diag_detail) {
        lv_label_set_text(lbl_diag_detail, msg);
        lv_obj_set_style_text_color(lbl_diag_detail, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    }
}

static void btn_diag_all_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    Buzzer_TestDiagnostic();

    char msg_rfid[128] = {0};
    bool ok_rfid = RFID_TestDiagnostic(msg_rfid, sizeof(msg_rfid));

    char msg_sd[128] = {0};
    bool ok_sd = Storage_TestDiagnostic(msg_sd, sizeof(msg_sd));

    char msg_wifi[128] = {0};
    bool ok_wifi = WiFi_GetDiagnostic(msg_wifi, sizeof(msg_wifi));

    char msg_rtc[128] = {0};
    bool ok_rtc = RTC_GetDiagnostic(msg_rtc, sizeof(msg_rtc));

    if (lbl_diag_summary) {
        char summary[128];
        snprintf(summary, sizeof(summary), "Hasil: RFID=%s | SD=%s | WiFi=%s | RTC=%s",
                 ok_rfid ? "OK" : "ERR", ok_sd ? "OK" : "ERR", ok_wifi ? "OK" : "ERR", ok_rtc ? "OK" : "ERR");
        lv_label_set_text(lbl_diag_summary, summary);
        lv_obj_set_style_text_color(lbl_diag_summary,
            lv_color_hex((ok_rfid && ok_sd && ok_wifi) ? COLOR_GREEN_STATUS : COLOR_AMBER_STATUS), 0);
    }

    if (lbl_diag_detail) {
        char detail[256];
        snprintf(detail, sizeof(detail),
                 "- RFID: %s\n- MicroSD: %s\n- Wi-Fi ESP32: %s\n- RTC Internal: %s",
                 ok_rfid ? "Reader Normal" : "Gagal Komunikasi SPI3",
                 ok_sd ? "FatFS R/W Sukses" : "Gagal Mount/Tulis",
                 ok_wifi ? "Terhubung ke AP" : "Belum Terhubung AP",
                 ok_rtc ? "Jam Berjalan Normal" : "Jam Belum Terkalibrasi");
        lv_label_set_text(lbl_diag_detail, detail);
        lv_obj_set_style_text_color(lbl_diag_detail, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    }
}

