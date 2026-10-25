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
extern bool g_wifi_connected;

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

/* Tab 1: Identitas Device & PIN Admin */
static lv_obj_t * tab_device          = NULL;
static lv_obj_t * lbl_dev_code_t      = NULL;
static lv_obj_t * ta_device_code      = NULL;
static lv_obj_t * lbl_dev_name_t      = NULL;
static lv_obj_t * ta_device_name      = NULL;
static lv_obj_t * lbl_admin_pin_t     = NULL;
static lv_obj_t * ta_admin_pin        = NULL;
static lv_obj_t * btn_save_device     = NULL;
static lv_obj_t * lbl_save_dev_btn    = NULL;
static lv_obj_t * lbl_device_feedback = NULL;

/* Tab 2: Sinkronisasi Server */
static lv_obj_t * tab_sync            = NULL;
static lv_obj_t * lbl_sync_mode_t     = NULL;
static lv_obj_t * dropdown_sync_mode  = NULL;
static lv_obj_t * lbl_pending_count   = NULL;
static lv_obj_t * btn_bulk_sync       = NULL;
static lv_obj_t * lbl_bulk_sync_btn   = NULL;
static lv_obj_t * lbl_sync_feedback   = NULL;

/* Tab 3: Tampilan & Sistem */
static lv_obj_t * lbl_bright_title    = NULL;
static lv_obj_t * slider_brightness   = NULL;
static lv_obj_t * lbl_bright_val      = NULL;
static lv_obj_t * lbl_theme_title     = NULL;
static lv_obj_t * sw_theme            = NULL;
static lv_obj_t * lbl_theme_desc      = NULL;
static lv_obj_t * lbl_timeout_title   = NULL;
static lv_obj_t * dropdown_timeout    = NULL;
static lv_obj_t * lbl_lpcd_title      = NULL;
static lv_obj_t * dropdown_lpcd_delta = NULL;
static lv_obj_t * lbl_lpcd_int_title  = NULL;
static lv_obj_t * dropdown_lpcd_interval = NULL;
static lv_obj_t * btn_test_beep       = NULL;
static lv_obj_t * lbl_t_beep          = NULL;
static lv_obj_t * btn_reset_stats     = NULL;
static lv_obj_t * lbl_t_reset         = NULL;
static lv_obj_t * btn_ntp_sync        = NULL;
static lv_obj_t * lbl_t_ntp           = NULL;
static lv_obj_t * lbl_admin_feedback  = NULL;

/* Tab: Pengguna MicroSD (USERS.CSV) */
static lv_obj_t * tab_users           = NULL;
static lv_obj_t * lbl_users_title     = NULL;
static lv_obj_t * lbl_users_count     = NULL;
static lv_obj_t * btn_refresh_users   = NULL;
static lv_obj_t * lbl_refresh_users   = NULL;
static lv_obj_t * cont_users_list     = NULL;
static lv_obj_t * table_users         = NULL;

/* Tab: Diagnostik & Informasi Perangkat */
static lv_obj_t * tab_info            = NULL;
static lv_obj_t * lbl_info_title      = NULL;
static lv_obj_t * btn_refresh_info    = NULL;
static lv_obj_t * lbl_refresh_info    = NULL;
static lv_obj_t * cont_info_box       = NULL;
static lv_obj_t * lbl_info_wifi_sec   = NULL;
static lv_obj_t * lbl_info_wifi_proto = NULL;
static lv_obj_t * lbl_info_wifi_status= NULL;
static lv_obj_t * lbl_info_wifi_ssid  = NULL;
static lv_obj_t * lbl_info_wifi_ip_mac= NULL;
static lv_obj_t * lbl_info_rfid_sec   = NULL;
static lv_obj_t * lbl_info_rfid_chip  = NULL;
static lv_obj_t * lbl_info_rfid_lpcd  = NULL;
static lv_obj_t * lbl_info_rfid_proto = NULL;
static lv_obj_t * lbl_info_sys_sec    = NULL;
static lv_obj_t * lbl_info_fw_ver     = NULL;
static lv_obj_t * lbl_info_mcu_info   = NULL;
static lv_obj_t * lbl_info_mem_info   = NULL;

/* Tab 4: Wi-Fi Setup */
static lv_obj_t * lbl_ssid_t          = NULL;
static lv_obj_t * ta_wifi_ssid        = NULL;
static lv_obj_t * lbl_pass_t          = NULL;
static lv_obj_t * ta_wifi_pass        = NULL;
static lv_obj_t * btn_save_wifi       = NULL;
static lv_obj_t * lbl_save_btn        = NULL;
static lv_obj_t * lbl_wifi_feedback   = NULL;
static lv_obj_t * kb_wifi             = NULL;

/* Tab 5: Log Absensi MicroSD */
static lv_obj_t * tab_log             = NULL;
static lv_obj_t * lbl_log_title       = NULL;
static lv_obj_t * btn_refresh_log     = NULL;
static lv_obj_t * lbl_refresh_log     = NULL;
static lv_obj_t * cont_log_list       = NULL;
static lv_obj_t * table_log           = NULL;

/* Tab 6: Test Device (Diagnostik Hardware) */
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

/* Warning & Info Modal Popups */
static lv_obj_t * modal_sd_warning    = NULL;
static lv_obj_t * modal_info_popup    = NULL;

/* Timer */
static lv_timer_t * revert_timer      = NULL;
static lv_timer_t * pwd_err_timer     = NULL;
static lv_timer_t * ntp_timeout_timer = NULL;
static lv_timer_t * popup_info_timer  = NULL;

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
static void btn_save_device_cb(lv_event_t * e);
static void dropdown_sync_cb(lv_event_t * e);
static void btn_bulk_sync_cb(lv_event_t * e);
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
static void dropdown_lpcd_delta_cb(lv_event_t * e);
static void dropdown_lpcd_interval_cb(lv_event_t * e);
static void btn_refresh_users_cb(lv_event_t * e);
static void btn_refresh_info_cb(lv_event_t * e);

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

    /* Generate kode & nama device unik berdasarkan 96-bit Hardware Unique ID STM32 */
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();
    uint16_t dev_id = (uint16_t)((uid0 ^ uid1 ^ uid2) & 0xFFFF);
    snprintf(s->device_code, sizeof(s->device_code), "GATE-%04X", (unsigned int)dev_id);
    snprintf(s->device_name, sizeof(s->device_name), "Gate %04X", (unsigned int)dev_id);

    strncpy(s->admin_pin, "1234", sizeof(s->admin_pin) - 1);
    s->sync_mode          = SYNC_MODE_REALTIME;
    s->lpcd_delta         = 3;   /* Default: 3 (Sensitivitas Optimal) */
    s->lpcd_interval_ms   = 300; /* Default: 300 ms */
}

bool App_Settings_Load(app_settings_t *out)
{
    const app_settings_t *flash_cfg = (const app_settings_t *)SETTINGS_FLASH_ADDR;
    if (flash_cfg->magic != SETTINGS_MAGIC) {
        return false;
    }

    if (flash_cfg->version == 1) {
        /* Upgrade dari versi 1 ke versi 2 */
        App_Settings_ResetDefaults(out);
        out->brightness = flash_cfg->brightness;
        out->theme_mode = flash_cfg->theme_mode;
        out->screen_timeout_sec = flash_cfg->screen_timeout_sec;
        strncpy(out->wifi_ssid, flash_cfg->wifi_ssid, sizeof(out->wifi_ssid) - 1);
        strncpy(out->wifi_pass, flash_cfg->wifi_pass, sizeof(out->wifi_pass) - 1);
        strncpy(out->server_ip, flash_cfg->server_ip, sizeof(out->server_ip) - 1);
        App_Settings_Save(out);
        return true;
    }

    if (flash_cfg->version != SETTINGS_VERSION) {
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

    /* Jika device_code masih berupa default lama GATE-001 atau kosong, otomatis perbarui ke UID unik */
    if (out->device_code[0] == '\0' || strcmp(out->device_code, "GATE-001") == 0) {
        uint32_t uid0 = HAL_GetUIDw0();
        uint32_t uid1 = HAL_GetUIDw1();
        uint32_t uid2 = HAL_GetUIDw2();
        uint16_t dev_id = (uint16_t)((uid0 ^ uid1 ^ uid2) & 0xFFFF);
        snprintf(out->device_code, sizeof(out->device_code), "GATE-%04X", (unsigned int)dev_id);
        if (out->device_name[0] == '\0' || strcmp(out->device_name, "Gate Utama") == 0) {
            snprintf(out->device_name, sizeof(out->device_name), "Gate %04X", (unsigned int)dev_id);
        }
        App_Settings_Save(out);
    }

    if (out->admin_pin[0] == '\0') strncpy(out->admin_pin, "1234", sizeof(out->admin_pin) - 1);
    if (out->lpcd_delta < 1 || out->lpcd_delta > 10) out->lpcd_delta = 3;
    if (out->lpcd_interval_ms < 100 || out->lpcd_interval_ms > 2000) out->lpcd_interval_ms = 300;
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
    if (ta_device_code && strlen(g_app_settings.device_code) > 0) {
        lv_textarea_set_text(ta_device_code, g_app_settings.device_code);
    }
    if (ta_device_name && strlen(g_app_settings.device_name) > 0) {
        lv_textarea_set_text(ta_device_name, g_app_settings.device_name);
    }
    if (ta_admin_pin && strlen(g_app_settings.admin_pin) > 0) {
        lv_textarea_set_text(ta_admin_pin, g_app_settings.admin_pin);
    }
    if (dropdown_sync_mode) {
        lv_dropdown_set_selected(dropdown_sync_mode, g_app_settings.sync_mode);
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

    /* Judul Sistem / Nama Device */
    lbl_header_title = lv_label_create(header_main);
    lv_obj_set_pos(lbl_header_title, 10, 4);
    if (strlen(g_app_settings.device_name) > 0) {
        lv_label_set_text(lbl_header_title, g_app_settings.device_name);
    } else {
        lv_label_set_text(lbl_header_title, "ABSENSI RFID");
    }
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

    tab_device            = lv_tabview_add_tab(tabview_admin, "DEVICE");
    lv_obj_t * tab_sistem = lv_tabview_add_tab(tabview_admin, "SISTEM");
    tab_users             = lv_tabview_add_tab(tabview_admin, "USERS");
    tab_info              = lv_tabview_add_tab(tabview_admin, "INFO");
    tab_sync              = lv_tabview_add_tab(tabview_admin, "SINKRON");
    lv_obj_t * tab_wifi   = lv_tabview_add_tab(tabview_admin, "WI-FI");
    tab_log               = lv_tabview_add_tab(tabview_admin, "LOG");
    tab_test              = lv_tabview_add_tab(tabview_admin, "TEST");

    /* ====================================================================== */
    /* TAB 1: IDENTITAS DEVICE & PIN ADMIN                                    */
    /* ====================================================================== */
    lv_obj_set_style_pad_all(tab_device, 8, 0);

    lbl_dev_code_t = lv_label_create(tab_device);
    lv_obj_set_pos(lbl_dev_code_t, 10, 8);
    lv_label_set_text(lbl_dev_code_t, "Kode Device:");
    lv_obj_set_style_text_color(lbl_dev_code_t, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_dev_code_t, &lv_font_montserrat_14, 0);

    ta_device_code = lv_textarea_create(tab_device);
    lv_obj_set_pos(ta_device_code, 120, 4);
    lv_obj_set_size(ta_device_code, 160, 32);
    lv_textarea_set_placeholder_text(ta_device_code, "Contoh: GATE-001");
    lv_textarea_set_one_line(ta_device_code, true);
    lv_obj_add_event_cb(ta_device_code, ta_wifi_focus_cb, LV_EVENT_ALL, NULL);

    lbl_dev_name_t = lv_label_create(tab_device);
    lv_obj_set_pos(lbl_dev_name_t, 10, 44);
    lv_label_set_text(lbl_dev_name_t, "Nama Device:");
    lv_obj_set_style_text_color(lbl_dev_name_t, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_dev_name_t, &lv_font_montserrat_14, 0);

    ta_device_name = lv_textarea_create(tab_device);
    lv_obj_set_pos(ta_device_name, 120, 40);
    lv_obj_set_size(ta_device_name, 160, 32);
    lv_textarea_set_placeholder_text(ta_device_name, "Contoh: Gate Utama");
    lv_textarea_set_one_line(ta_device_name, true);
    lv_obj_add_event_cb(ta_device_name, ta_wifi_focus_cb, LV_EVENT_ALL, NULL);

    lbl_admin_pin_t = lv_label_create(tab_device);
    lv_obj_set_pos(lbl_admin_pin_t, 10, 80);
    lv_label_set_text(lbl_admin_pin_t, "PIN Admin:");
    lv_obj_set_style_text_color(lbl_admin_pin_t, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_admin_pin_t, &lv_font_montserrat_14, 0);

    ta_admin_pin = lv_textarea_create(tab_device);
    lv_obj_set_pos(ta_admin_pin, 120, 76);
    lv_obj_set_size(ta_admin_pin, 160, 32);
    lv_textarea_set_password_mode(ta_admin_pin, true);
    lv_textarea_set_placeholder_text(ta_admin_pin, "Min 4 digit");
    lv_textarea_set_one_line(ta_admin_pin, true);
    lv_obj_add_event_cb(ta_admin_pin, ta_wifi_focus_cb, LV_EVENT_ALL, NULL);

    btn_save_device = lv_button_create(tab_device);
    lv_obj_set_pos(btn_save_device, 295, 12);
    lv_obj_set_size(btn_save_device, 145, 80);
    lv_obj_set_style_bg_color(btn_save_device, lv_color_hex(0x064E3B), 0);
    lv_obj_set_style_border_color(btn_save_device, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_border_width(btn_save_device, 1, 0);
    lv_obj_set_style_radius(btn_save_device, 8, 0);
    lv_obj_add_event_cb(btn_save_device, btn_save_device_cb, LV_EVENT_CLICKED, NULL);

    lbl_save_dev_btn = lv_label_create(btn_save_device);
    lv_obj_center(lbl_save_dev_btn);
    lv_label_set_text(lbl_save_dev_btn, "SIMPAN DEVICE\n& PIN ADMIN");
    lv_obj_set_style_text_align(lbl_save_dev_btn, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_save_dev_btn, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_text_font(lbl_save_dev_btn, &lv_font_montserrat_14, 0);

    lbl_device_feedback = lv_label_create(tab_device);
    lv_obj_set_pos(lbl_device_feedback, 10, 118);
    lv_obj_set_width(lbl_device_feedback, 430);
    lv_label_set_text(lbl_device_feedback, "Status: Siap (Master PIN darurat: 9999)");
    lv_obj_set_style_text_color(lbl_device_feedback, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_device_feedback, &lv_font_montserrat_14, 0);

    /* ====================================================================== */
    /* TAB 2: METODE SINKRONISASI SERVER (REALTIME / PERIODIK BULK)            */
    /* ====================================================================== */
    lv_obj_set_style_pad_all(tab_sync, 8, 0);

    lbl_sync_mode_t = lv_label_create(tab_sync);
    lv_obj_set_pos(lbl_sync_mode_t, 10, 8);
    lv_label_set_text(lbl_sync_mode_t, "Metode Sinkronisasi:");
    lv_obj_set_style_text_color(lbl_sync_mode_t, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_sync_mode_t, &lv_font_montserrat_14, 0);

    dropdown_sync_mode = lv_dropdown_create(tab_sync);
    lv_obj_set_pos(dropdown_sync_mode, 180, 4);
    lv_obj_set_size(dropdown_sync_mode, 250, 32);
    lv_dropdown_set_options(dropdown_sync_mode,
        "Realtime (Langsung Kirim)\n"
        "Periodik 10 Menit (Bulk)\n"
        "Periodik 30 Menit (Bulk)\n"
        "Periodik 1 Jam (Bulk)\n"
        "Periodik 1 Hari (Bulk)");
    lv_dropdown_set_selected(dropdown_sync_mode, g_app_settings.sync_mode);
    lv_obj_add_event_cb(dropdown_sync_mode, dropdown_sync_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lbl_pending_count = lv_label_create(tab_sync);
    lv_obj_set_pos(lbl_pending_count, 10, 48);
    lv_obj_set_width(lbl_pending_count, 420);
    lv_label_set_text(lbl_pending_count, "Antrian Belum Sinkron: 0 data");
    lv_obj_set_style_text_color(lbl_pending_count, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_pending_count, &lv_font_montserrat_14, 0);

    btn_bulk_sync = lv_button_create(tab_sync);
    lv_obj_set_pos(btn_bulk_sync, 10, 78);
    lv_obj_set_size(btn_bulk_sync, 260, 36);
    lv_obj_set_style_bg_color(btn_bulk_sync, lv_color_hex(0x064E3B), 0);
    lv_obj_set_style_border_color(btn_bulk_sync, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_border_width(btn_bulk_sync, 1, 0);
    lv_obj_set_style_radius(btn_bulk_sync, 8, 0);
    lv_obj_add_event_cb(btn_bulk_sync, btn_bulk_sync_cb, LV_EVENT_CLICKED, NULL);

    lbl_bulk_sync_btn = lv_label_create(btn_bulk_sync);
    lv_obj_center(lbl_bulk_sync_btn);
    lv_label_set_text(lbl_bulk_sync_btn, LV_SYMBOL_REFRESH " START SYNC MANUAL");
    lv_obj_set_style_text_color(lbl_bulk_sync_btn, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_text_font(lbl_bulk_sync_btn, &lv_font_montserrat_14, 0);

    lbl_sync_feedback = lv_label_create(tab_sync);
    lv_obj_set_pos(lbl_sync_feedback, 10, 122);
    lv_obj_set_width(lbl_sync_feedback, 440);
    lv_label_set_text(lbl_sync_feedback, "Status: Siap");
    lv_obj_set_style_text_color(lbl_sync_feedback, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_sync_feedback, &lv_font_montserrat_14, 0);

    /* ====================================================================== */
    /* TAB 3: SISTEM & DISPLAY                                                */
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
    lv_obj_set_pos(lbl_timeout_title, 10, 74);
    lv_label_set_text(lbl_timeout_title, "Timeout Layar:");
    lv_obj_set_style_text_color(lbl_timeout_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_timeout_title, &lv_font_montserrat_14, 0);

    dropdown_timeout = lv_dropdown_create(tab_sistem);
    lv_obj_set_pos(dropdown_timeout, 150, 68);
    lv_obj_set_size(dropdown_timeout, 290, 30);
    lv_dropdown_set_options(dropdown_timeout, "Selalu Nyala\n30 Detik\n1 Menit\n2 Menit\n5 Menit");
    lv_dropdown_set_selected(dropdown_timeout, 2); /* Default 1 Menit */
    lv_obj_add_event_cb(dropdown_timeout, dropdown_timeout_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Baris 4: Sensitivitas LPCD RFID (Delta Threshold) */
    lbl_lpcd_title = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_lpcd_title, 10, 106);
    lv_label_set_text(lbl_lpcd_title, "Sensitivitas LPCD:");
    lv_obj_set_style_text_color(lbl_lpcd_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_lpcd_title, &lv_font_montserrat_14, 0);

    dropdown_lpcd_delta = lv_dropdown_create(tab_sistem);
    lv_obj_set_pos(dropdown_lpcd_delta, 150, 100);
    lv_obj_set_size(dropdown_lpcd_delta, 290, 30);
    lv_dropdown_set_options(dropdown_lpcd_delta,
        "Delta 1 (Paling Peka)\n"
        "Delta 2 (Tinggi)\n"
        "Delta 3 (Optimal - Rekomendasi)\n"
        "Delta 4 (Sedang)\n"
        "Delta 5 (Rendah - Metal Besar)\n"
        "Delta 6 (Sangat Rendah)\n"
        "Delta 7 (Minimal)\n"
        "Delta 8 (Kasar)");
    uint8_t d_idx = (g_app_settings.lpcd_delta >= 1 && g_app_settings.lpcd_delta <= 8) ? (g_app_settings.lpcd_delta - 1) : 2;
    lv_dropdown_set_selected(dropdown_lpcd_delta, d_idx);
    lv_obj_add_event_cb(dropdown_lpcd_delta, dropdown_lpcd_delta_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Baris 5: Interval Sleep LPCD (ms) */
    lbl_lpcd_int_title = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_lpcd_int_title, 10, 138);
    lv_label_set_text(lbl_lpcd_int_title, "Interval Cek LPCD:");
    lv_obj_set_style_text_color(lbl_lpcd_int_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_lpcd_int_title, &lv_font_montserrat_14, 0);

    dropdown_lpcd_interval = lv_dropdown_create(tab_sistem);
    lv_obj_set_pos(dropdown_lpcd_interval, 150, 132);
    lv_obj_set_size(dropdown_lpcd_interval, 290, 30);
    lv_dropdown_set_options(dropdown_lpcd_interval,
        "150 ms (Sangat Cepat)\n"
        "250 ms (Cepat)\n"
        "300 ms (Optimal - Default)\n"
        "500 ms (Hemat Baterai)");
    uint8_t int_idx = 2;
    if (g_app_settings.lpcd_interval_ms == 150) int_idx = 0;
    else if (g_app_settings.lpcd_interval_ms == 250) int_idx = 1;
    else if (g_app_settings.lpcd_interval_ms == 300) int_idx = 2;
    else if (g_app_settings.lpcd_interval_ms == 500) int_idx = 3;
    lv_dropdown_set_selected(dropdown_lpcd_interval, int_idx);
    lv_obj_add_event_cb(dropdown_lpcd_interval, dropdown_lpcd_interval_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Baris 6: Tombol Aksi Hardware & Sinkronisasi */
    btn_test_beep = lv_button_create(tab_sistem);
    lv_obj_set_pos(btn_test_beep, 10, 168);
    lv_obj_set_size(btn_test_beep, 130, 34);
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
    lv_obj_set_pos(btn_reset_stats, 155, 168);
    lv_obj_set_size(btn_reset_stats, 130, 34);
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
    lv_obj_set_pos(btn_ntp_sync, 300, 168);
    lv_obj_set_size(btn_ntp_sync, 140, 34);
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

    /* Baris 7: Label Feedback Aksi */
    lbl_admin_feedback = lv_label_create(tab_sistem);
    lv_obj_set_pos(lbl_admin_feedback, 10, 208);
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

    /* Label Status & Umpan Balik Wi-Fi Real-time */
    lbl_wifi_feedback = lv_label_create(tab_wifi);
    lv_obj_set_pos(lbl_wifi_feedback, 10, 80);
    lv_obj_set_width(lbl_wifi_feedback, 430);
    lv_label_set_text(lbl_wifi_feedback, "Status Wi-Fi: Belum Terhubung [Mencoba konek...]");
    lv_obj_set_style_text_color(lbl_wifi_feedback, lv_color_hex(COLOR_AMBER_STATUS), 0);
    lv_obj_set_style_text_font(lbl_wifi_feedback, &lv_font_montserrat_14, 0);

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

    /* ====================================================================== */
    /* TAB: PENGGUNA TERDAFTAR (USERS.CSV DI MICROSD)                         */
    /* ====================================================================== */
    lv_obj_remove_flag(tab_users, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tab_users, 6, 0);

    lbl_users_title = lv_label_create(tab_users);
    lv_obj_set_pos(lbl_users_title, 10, 6);
    lv_label_set_text(lbl_users_title, "Akses Lokal (USERS.CSV):");
    lv_obj_set_style_text_color(lbl_users_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_users_title, &lv_font_montserrat_14, 0);

    lbl_users_count = lv_label_create(tab_users);
    lv_obj_set_pos(lbl_users_count, 220, 6);
    lv_label_set_text(lbl_users_count, "Total: 0 user");
    lv_obj_set_style_text_color(lbl_users_count, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_users_count, &lv_font_montserrat_14, 0);

    btn_refresh_users = lv_button_create(tab_users);
    lv_obj_set_pos(btn_refresh_users, 330, 2);
    lv_obj_set_size(btn_refresh_users, 110, 28);
    lv_obj_set_style_bg_color(btn_refresh_users, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_refresh_users, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_refresh_users, 1, 0);
    lv_obj_set_style_radius(btn_refresh_users, 6, 0);
    lv_obj_add_event_cb(btn_refresh_users, btn_refresh_users_cb, LV_EVENT_CLICKED, NULL);

    lbl_refresh_users = lv_label_create(btn_refresh_users);
    lv_obj_center(lbl_refresh_users);
    lv_label_set_text(lbl_refresh_users, LV_SYMBOL_REFRESH " REFRESH");
    lv_obj_set_style_text_color(lbl_refresh_users, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_refresh_users, &lv_font_montserrat_14, 0);

    cont_users_list = lv_obj_create(tab_users);
    lv_obj_set_pos(cont_users_list, 10, 36);
    lv_obj_set_size(cont_users_list, 440, 194);
    lv_obj_set_style_bg_color(cont_users_list, lv_color_hex(0x060E1A), 0);
    lv_obj_set_style_border_color(cont_users_list, lv_color_hex(COLOR_DARK_BORDER), 0);
    lv_obj_set_style_border_width(cont_users_list, 1, 0);
    lv_obj_set_style_radius(cont_users_list, 8, 0);
    lv_obj_set_style_pad_all(cont_users_list, 2, 0);
    lv_obj_add_flag(cont_users_list, LV_OBJ_FLAG_SCROLLABLE);

    table_users = lv_table_create(cont_users_list);
    lv_obj_remove_flag(table_users, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(table_users, &lv_font_montserrat_14, 0);
    lv_table_set_column_count(table_users, 4);
    lv_table_set_column_width(table_users, 0, 34);   /* No */
    lv_table_set_column_width(table_users, 1, 140);  /* UID Kartu */
    lv_table_set_column_width(table_users, 2, 160);  /* Nama Karyawan */
    lv_table_set_column_width(table_users, 3, 90);   /* Jabatan */

    lv_table_set_cell_value(table_users, 0, 0, "#");
    lv_table_set_cell_value(table_users, 0, 1, "UID KARTU");
    lv_table_set_cell_value(table_users, 0, 2, "NAMA KARYAWAN");
    lv_table_set_cell_value(table_users, 0, 3, "JABATAN");

    /* ====================================================================== */
    /* TAB: INFORMASI & DIAGNOSTIK DEVICE (WI-FI 6, RFID, FIRMWARE)           */
    /* ====================================================================== */
    lv_obj_remove_flag(tab_info, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tab_info, 6, 0);

    lbl_info_title = lv_label_create(tab_info);
    lv_obj_set_pos(lbl_info_title, 10, 6);
    lv_label_set_text(lbl_info_title, "Informasi Perangkat & Jaringan:");
    lv_obj_set_style_text_color(lbl_info_title, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_info_title, &lv_font_montserrat_14, 0);

    btn_refresh_info = lv_button_create(tab_info);
    lv_obj_set_pos(btn_refresh_info, 330, 2);
    lv_obj_set_size(btn_refresh_info, 110, 28);
    lv_obj_set_style_bg_color(btn_refresh_info, lv_color_hex(COLOR_DARK_CARD), 0);
    lv_obj_set_style_border_color(btn_refresh_info, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    lv_obj_set_style_border_width(btn_refresh_info, 1, 0);
    lv_obj_set_style_radius(btn_refresh_info, 6, 0);
    lv_obj_add_event_cb(btn_refresh_info, btn_refresh_info_cb, LV_EVENT_CLICKED, NULL);

    lbl_refresh_info = lv_label_create(btn_refresh_info);
    lv_obj_center(lbl_refresh_info);
    lv_label_set_text(lbl_refresh_info, LV_SYMBOL_REFRESH " REFRESH");
    lv_obj_set_style_text_color(lbl_refresh_info, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_refresh_info, &lv_font_montserrat_14, 0);

    cont_info_box = lv_obj_create(tab_info);
    lv_obj_set_pos(cont_info_box, 10, 36);
    lv_obj_set_size(cont_info_box, 440, 194);
    lv_obj_set_style_bg_color(cont_info_box, lv_color_hex(0x060E1A), 0);
    lv_obj_set_style_border_color(cont_info_box, lv_color_hex(COLOR_DARK_BORDER), 0);
    lv_obj_set_style_border_width(cont_info_box, 1, 0);
    lv_obj_set_style_radius(cont_info_box, 8, 0);
    lv_obj_set_style_pad_all(cont_info_box, 8, 0);
    lv_obj_add_flag(cont_info_box, LV_OBJ_FLAG_SCROLLABLE);

    /* Section 1: Wi-Fi */
    lbl_info_wifi_sec = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_wifi_sec, 4, 4);
    lv_label_set_text(lbl_info_wifi_sec, "[1] KONEKSI WI-FI & JARINGAN");
    lv_obj_set_style_text_color(lbl_info_wifi_sec, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_info_wifi_sec, &lv_font_montserrat_14, 0);

    lbl_info_wifi_proto = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_wifi_proto, 8, 24);
    lv_label_set_text(lbl_info_wifi_proto, "- Tipe: Memeriksa jaringan...");
    lv_obj_set_style_text_color(lbl_info_wifi_proto, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_text_font(lbl_info_wifi_proto, &lv_font_montserrat_14, 0);

    lbl_info_wifi_status = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_wifi_status, 8, 42);
    lv_label_set_text(lbl_info_wifi_status, "- Status: Belum Terhubung");
    lv_obj_set_style_text_color(lbl_info_wifi_status, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_info_wifi_status, &lv_font_montserrat_14, 0);

    lbl_info_wifi_ssid = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_wifi_ssid, 8, 60);
    lv_label_set_text(lbl_info_wifi_ssid, "- SSID: - | IP: -");
    lv_obj_set_style_text_color(lbl_info_wifi_ssid, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_info_wifi_ssid, &lv_font_montserrat_14, 0);

    lbl_info_wifi_ip_mac = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_wifi_ip_mac, 8, 78);
    lv_label_set_text(lbl_info_wifi_ip_mac, "- MAC: -");
    lv_obj_set_style_text_color(lbl_info_wifi_ip_mac, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_info_wifi_ip_mac, &lv_font_montserrat_14, 0);

    /* Section 2: RFID Reader */
    lbl_info_rfid_sec = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_rfid_sec, 4, 102);
    lv_label_set_text(lbl_info_rfid_sec, "[2] RFID READER CLRC66303HNY");
    lv_obj_set_style_text_color(lbl_info_rfid_sec, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_info_rfid_sec, &lv_font_montserrat_14, 0);

    lbl_info_rfid_chip = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_rfid_chip, 8, 122);
    lv_label_set_text(lbl_info_rfid_chip, "- Chip IC: NXP CLRC66303HNY (Plus Rev 3, 0x1A)");
    lv_obj_set_style_text_color(lbl_info_rfid_chip, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_info_rfid_chip, &lv_font_montserrat_14, 0);

    lbl_info_rfid_lpcd = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_rfid_lpcd, 8, 140);
    lv_label_set_text(lbl_info_rfid_lpcd, "- Mode LPCD: Aktif (PA15 Wakeup, Delta: 3, 300 ms)");
    lv_obj_set_style_text_color(lbl_info_rfid_lpcd, lv_color_hex(COLOR_GREEN_STATUS), 0);
    lv_obj_set_style_text_font(lbl_info_rfid_lpcd, &lv_font_montserrat_14, 0);

    lbl_info_rfid_proto = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_rfid_proto, 8, 158);
    lv_label_set_text(lbl_info_rfid_proto, "- Protokol: ISO14443A (Mifare/e-KTP), ISO14443B, ISO15693");
    lv_obj_set_style_text_color(lbl_info_rfid_proto, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_info_rfid_proto, &lv_font_montserrat_14, 0);

    /* Section 3: Sistem & Firmware */
    lbl_info_sys_sec = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_sys_sec, 4, 182);
    lv_label_set_text(lbl_info_sys_sec, "[3] FIRMWARE & SISTEM HARDWARE");
    lv_obj_set_style_text_color(lbl_info_sys_sec, lv_color_hex(COLOR_CYAN_TEXT), 0);
    lv_obj_set_style_text_font(lbl_info_sys_sec, &lv_font_montserrat_14, 0);

    lbl_info_fw_ver = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_fw_ver, 8, 202);
    lv_label_set_text(lbl_info_fw_ver, "- Versi Firmware: v2.5.0-LPCD-AX (Build: 2026-10-26)");
    lv_obj_set_style_text_color(lbl_info_fw_ver, lv_color_hex(COLOR_DARK_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(lbl_info_fw_ver, &lv_font_montserrat_14, 0);

    lbl_info_mcu_info = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_mcu_info, 8, 220);
    lv_label_set_text(lbl_info_mcu_info, "- MCU: STM32F405RGT6 (168MHz) | Co-MCU: ESP32-C6 RISC-V");
    lv_obj_set_style_text_color(lbl_info_mcu_info, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_info_mcu_info, &lv_font_montserrat_14, 0);

    lbl_info_mem_info = lv_label_create(cont_info_box);
    lv_obj_set_pos(lbl_info_mem_info, 8, 238);
    lv_label_set_text(lbl_info_mem_info, "- Penyimpanan: MicroSD FatFS | Config: Flash Sector 11");
    lv_obj_set_style_text_color(lbl_info_mem_info, lv_color_hex(COLOR_DARK_TEXT_MUTED), 0);
    lv_obj_set_style_text_font(lbl_info_mem_info, &lv_font_montserrat_14, 0);
}

/**
  * @brief  Proses event kedatangan data RFID / Verifikasi Server
  */
void GUI_Attendance_ProcessEvent(const ui_event_t *evt)
{
    if (evt == NULL || screen_main == NULL) return;

    if (evt->type == UI_EVENT_RFID_CARD_DETECTED)
    {
        /* Bangunkan layar hanya saat ada interaksi fisik / kartu di-tap */
        GUI_Attendance_Wakeup();

        /* Format UID string secara dinamis untuk kartu 4, 7 (e-KTP), atau 10 byte */
        char uid_str[40] = "UID: ";
        int offset = 5;
        for (uint8_t i = 0; i < evt->uid_len && i < 10 && offset < (int)sizeof(uid_str) - 3; i++) {
            offset += snprintf(uid_str + offset, sizeof(uid_str) - offset, (i == 0) ? "%02X" : " %02X", evt->uid[i]);
        }
        if (evt->uid_len == 0) {
            snprintf(uid_str, sizeof(uid_str), "UID: UNKNOWN");
        }
        if (evt->uid_len > 4) {
            lv_obj_set_style_text_font(lbl_uid, &lv_font_montserrat_14, 0);
        } else {
            lv_obj_set_style_text_font(lbl_uid, &lv_font_montserrat_16, 0);
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
        /* Sukses verifikasi kehadiran dari ESP32 atau database offline MicroSD */
        char msg[72];
        if (evt->extra[0] != '\0') {
            snprintf(msg, sizeof(msg), "HADIR: %.60s", evt->extra);
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
        char msg[72];
        if (evt->extra[0] != '\0') {
            snprintf(msg, sizeof(msg), "DITOLAK: %.58s", evt->extra);
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
    else if (evt->type == UI_EVENT_BULK_SYNC_DONE)
    {
        uint32_t pending = Storage_GetPendingCount();
        GUI_Attendance_UpdatePendingCount(pending);

        if (evt->status == 1) {
            char pop[96];
            snprintf(pop, sizeof(pop), "%s\nSisa antrean: %lu transaksi.", evt->extra, (unsigned long)pending);
            GUI_Attendance_ShowPopup("SINKRONISASI BERHASIL", pop, true);
            if (lbl_sync_feedback) {
                lv_label_set_text_fmt(lbl_sync_feedback, "%s (Sisa: %lu)", evt->extra, (unsigned long)pending);
                lv_obj_set_style_text_color(lbl_sync_feedback, lv_color_hex(COLOR_GREEN_STATUS), 0);
            }
        } else if (evt->status == 2) {
            char pop[96];
            snprintf(pop, sizeof(pop), "%s\nSisa antrean: %lu transaksi.", evt->extra, (unsigned long)pending);
            GUI_Attendance_ShowPopup("SINKRONISASI SEBAGIAN", pop, true);
            if (lbl_sync_feedback) {
                lv_label_set_text_fmt(lbl_sync_feedback, "%s (Sisa: %lu)", evt->extra, (unsigned long)pending);
                lv_obj_set_style_text_color(lbl_sync_feedback, lv_color_hex(COLOR_AMBER_STATUS), 0);
            }
        } else {
            char pop[96];
            snprintf(pop, sizeof(pop), "%s\nData tetap aman di SD card (%lu antrean).", evt->extra, (unsigned long)pending);
            GUI_Attendance_ShowPopup("SINKRONISASI GAGAL", pop, false);
            if (lbl_sync_feedback) {
                lv_label_set_text_fmt(lbl_sync_feedback, "%s", evt->extra);
                lv_obj_set_style_text_color(lbl_sync_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
            }
        }
    }
    else if (evt->type == UI_EVENT_DEVICE_REG_RESULT)
    {
        if (evt->status == 1) {
            GUI_Attendance_ShowPopup("REGISTRASI BERHASIL", evt->extra, true);
            if (lbl_device_feedback) {
                lv_label_set_text_fmt(lbl_device_feedback, "Server: %s", evt->extra);
                lv_obj_set_style_text_color(lbl_device_feedback, lv_color_hex(COLOR_GREEN_STATUS), 0);
            }
        } else {
            GUI_Attendance_ShowPopup("REGISTRASI GAGAL", evt->extra, false);
            if (lbl_device_feedback) {
                lv_label_set_text_fmt(lbl_device_feedback, "Server: %s", evt->extra);
                lv_obj_set_style_text_color(lbl_device_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
            }
        }
    }
    else if (evt->type == UI_EVENT_NTP_SYNCED)
    {
        if (ntp_timeout_timer) {
            lv_timer_delete(ntp_timeout_timer);
            ntp_timeout_timer = NULL;
        }
        GUI_Attendance_ShowPopup("JAM DISINKRONKAN", "Waktu RTC internal berhasil disesuaikan dengan Server Backend.", true);
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
        GUI_Attendance_ShowPopup("SINKRON JAM GAGAL", evt->extra[0] != '\0' ? evt->extra : "Gagal sinkronisasi waktu ke server.", false);
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

    if (lbl_wifi_feedback != NULL) {
        if (wifi_ok) {
            lv_label_set_text(lbl_wifi_feedback, "Status Wi-Fi: Terhubung ke AP [OK]");
            lv_obj_set_style_text_color(lbl_wifi_feedback, lv_color_hex(COLOR_GREEN_STATUS), 0);
        } else {
            lv_label_set_text(lbl_wifi_feedback, "Status Wi-Fi: Belum Terhubung [Periksa SSID/Password]");
            lv_obj_set_style_text_color(lbl_wifi_feedback, lv_color_hex(COLOR_AMBER_STATUS), 0);
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
    uint32_t now = HAL_GetTick();
    uint32_t idle_ms = now - g_last_activity_tick;

    /* 1. Auto-timeout Halaman Admin & Halaman PIN: 30 detik tanpa aktivitas sentuh */
    lv_obj_t *act_scr = lv_screen_active();
    if (act_scr == screen_admin || act_scr == screen_password) {
        if (idle_ms >= 30000) {
            /* Sembunyikan keyboard virtual jika terbuka */
            if (kb_wifi) lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
            /* Reset digit PIN */
            g_pin_len = 0;
            g_entered_pin[0] = '\0';
            /* Kembali ke layar utama absensi */
            lv_screen_load(screen_main);
            /* Reset timer aktivitas agar tidak langsung memicu sleep display */
            g_last_activity_tick = now;
            /* Tampilkan notifikasi popup timeout demi keamanan */
            GUI_Attendance_ShowPopup("SESI ADMIN BERAKHIR", "Otomatis kembali ke menu absensi karena tidak ada interaksi selama 30 detik.", true);
            return;
        }
    }

    /* 2. Cek dan eksekusi screen sleep/dim jika idle timeout tercapai */
    if (g_screen_timeout_sec > 0 && !g_screen_is_sleeping) {
        if (idle_ms >= (g_screen_timeout_sec * 1000)) {
            g_screen_is_sleeping = true;
            if (act_scr == screen_admin || act_scr == screen_password) {
                if (kb_wifi) lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
                g_pin_len = 0;
                g_entered_pin[0] = '\0';
                lv_screen_load(screen_main);
            }
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

static void btn_dismiss_sd_warning_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if (modal_sd_warning != NULL) {
        lv_obj_delete(modal_sd_warning);
        modal_sd_warning = NULL;
    }
}

/**
  * @brief  Tampilkan atau sembunyikan popup modal peringatan MicroSD dicabut
  */
void GUI_Attendance_ShowSDWarning(bool show)
{
    if (!show) {
        if (modal_sd_warning != NULL) {
            lv_obj_delete(modal_sd_warning);
            modal_sd_warning = NULL;
        }
        return;
    }

    /* Jangan buat popup ganda jika sudah aktif */
    if (modal_sd_warning != NULL) return;

    /* Tampilkan modal di lv_layer_top() agar selalu berada di paling depan layar */
    modal_sd_warning = lv_obj_create(lv_layer_top());
    lv_obj_set_size(modal_sd_warning, 430, 165);
    lv_obj_center(modal_sd_warning);
    lv_obj_set_style_bg_color(modal_sd_warning, lv_color_hex(0x450A0A), 0);
    lv_obj_set_style_border_color(modal_sd_warning, lv_color_hex(COLOR_RED_STATUS), 0);
    lv_obj_set_style_border_width(modal_sd_warning, 2, 0);
    lv_obj_set_style_radius(modal_sd_warning, 12, 0);
    lv_obj_set_style_pad_all(modal_sd_warning, 10, 0);
    lv_obj_remove_flag(modal_sd_warning, LV_OBJ_FLAG_SCROLLABLE);

    /* Judul Peringatan */
    lv_obj_t * lbl_warn_title = lv_label_create(modal_sd_warning);
    lv_obj_set_pos(lbl_warn_title, 0, 4);
    lv_obj_set_width(lbl_warn_title, 410);
    lv_label_set_text(lbl_warn_title, LV_SYMBOL_WARNING " PERINGATAN: MICROSD DICABUT!");
    lv_obj_set_style_text_align(lbl_warn_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_warn_title, lv_color_hex(0xFCA5A5), 0);
    lv_obj_set_style_text_font(lbl_warn_title, &lv_font_montserrat_16, 0);

    /* Pesan Penjelasan */
    lv_obj_t * lbl_warn_desc = lv_label_create(modal_sd_warning);
    lv_obj_set_pos(lbl_warn_desc, 10, 36);
    lv_obj_set_width(lbl_warn_desc, 390);
    lv_label_set_long_mode(lbl_warn_desc, LV_LABEL_LONG_MODE_WRAP);
    lv_label_set_text(lbl_warn_desc,
        "Kartu MicroSD terlepas saat sistem sedang beroperasi!\n"
        "Data riwayat kehadiran tidak dapat dicatat ke MicroSD.\n"
        "Harap segera pasang kembali MicroSD berformat FAT32.");
    lv_obj_set_style_text_color(lbl_warn_desc, lv_color_hex(0xFEE2E2), 0);
    lv_obj_set_style_text_font(lbl_warn_desc, &lv_font_montserrat_14, 0);

    /* Tombol Tutup */
    lv_obj_t * btn_close_warn = lv_button_create(modal_sd_warning);
    lv_obj_set_pos(btn_close_warn, 155, 115);
    lv_obj_set_size(btn_close_warn, 100, 32);
    lv_obj_set_style_bg_color(btn_close_warn, lv_color_hex(COLOR_RED_STATUS), 0);
    lv_obj_set_style_radius(btn_close_warn, 6, 0);
    lv_obj_add_event_cb(btn_close_warn, btn_dismiss_sd_warning_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_btn = lv_label_create(btn_close_warn);
    lv_obj_center(lbl_btn);
    lv_label_set_text(lbl_btn, "TUTUP");
    lv_obj_set_style_text_color(lbl_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_btn, &lv_font_montserrat_14, 0);
}

static void btn_dismiss_info_popup_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if (popup_info_timer != NULL) {
        lv_timer_delete(popup_info_timer);
        popup_info_timer = NULL;
    }
    if (modal_info_popup != NULL) {
        lv_obj_delete(modal_info_popup);
        modal_info_popup = NULL;
    }
}

static void popup_info_timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    popup_info_timer = NULL;
    if (modal_info_popup != NULL) {
        lv_obj_delete(modal_info_popup);
        modal_info_popup = NULL;
    }
}

/**
  * @brief  Tampilkan popup modal informasi / status hasil aksi (Sukses / Gagal)
  */
void GUI_Attendance_ShowPopup(const char *title, const char *desc, bool is_success)
{
    if (popup_info_timer != NULL) {
        lv_timer_delete(popup_info_timer);
        popup_info_timer = NULL;
    }
    if (modal_info_popup != NULL) {
        lv_obj_delete(modal_info_popup);
        modal_info_popup = NULL;
    }
    if (title == NULL && desc == NULL) return;

    modal_info_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(modal_info_popup, 420, 180);
    lv_obj_center(modal_info_popup);

    uint32_t bg_col    = is_success ? 0x064E3B : 0x450A0A;
    uint32_t bdr_col   = is_success ? COLOR_GREEN_STATUS : COLOR_RED_STATUS;
    uint32_t title_col = is_success ? 0x6EE7B7 : 0xFCA5A5;

    lv_obj_set_style_bg_color(modal_info_popup, lv_color_hex(bg_col), 0);
    lv_obj_set_style_border_color(modal_info_popup, lv_color_hex(bdr_col), 0);
    lv_obj_set_style_border_width(modal_info_popup, 2, 0);
    lv_obj_set_style_radius(modal_info_popup, 12, 0);
    lv_obj_set_style_pad_all(modal_info_popup, 10, 0);
    lv_obj_remove_flag(modal_info_popup, LV_OBJ_FLAG_SCROLLABLE);

    /* Judul Modal */
    lv_obj_t * lbl_title = lv_label_create(modal_info_popup);
    lv_obj_set_pos(lbl_title, 0, 4);
    lv_obj_set_width(lbl_title, 400);
    lv_label_set_text_fmt(lbl_title, "%s %s", is_success ? LV_SYMBOL_OK : LV_SYMBOL_WARNING, title ? title : "INFO");
    lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(title_col), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);

    /* Deskripsi Info */
    lv_obj_t * lbl_msg = lv_label_create(modal_info_popup);
    lv_obj_set_pos(lbl_msg, 10, 36);
    lv_obj_set_width(lbl_msg, 380);
    lv_label_set_long_mode(lbl_msg, LV_LABEL_LONG_MODE_WRAP);
    lv_label_set_text(lbl_msg, desc ? desc : "");
    lv_obj_set_style_text_align(lbl_msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_msg, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_text_font(lbl_msg, &lv_font_montserrat_14, 0);

    /* Tombol Tutup */
    lv_obj_t * btn_close = lv_button_create(modal_info_popup);
    lv_obj_set_pos(btn_close, 150, 130);
    lv_obj_set_size(btn_close, 100, 32);
    lv_obj_set_style_bg_color(btn_close, lv_color_hex(bdr_col), 0);
    lv_obj_set_style_radius(btn_close, 6, 0);
    lv_obj_add_event_cb(btn_close, btn_dismiss_info_popup_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_btn_txt = lv_label_create(btn_close);
    lv_obj_center(lbl_btn_txt);
    lv_label_set_text(lbl_btn_txt, "TUTUP");
    lv_obj_set_style_text_color(lbl_btn_txt, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_btn_txt, &lv_font_montserrat_14, 0);

    /* Auto-dismiss setelah 4.5 detik jika tidak disentuh */
    popup_info_timer = lv_timer_create(popup_info_timer_cb, 4500, NULL);
    lv_timer_set_repeat_count(popup_info_timer, 1);
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

    /* Tab Device Elements */
    if (lbl_dev_code_t) lv_obj_set_style_text_color(lbl_dev_code_t, lv_color_hex(c_text_pri), 0);
    if (lbl_dev_name_t) lv_obj_set_style_text_color(lbl_dev_name_t, lv_color_hex(c_text_pri), 0);
    if (lbl_admin_pin_t) lv_obj_set_style_text_color(lbl_admin_pin_t, lv_color_hex(c_text_pri), 0);
    if (ta_device_code) {
        lv_obj_set_style_bg_color(ta_device_code, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(ta_device_code, lv_color_hex(c_border), 0);
        lv_obj_set_style_text_color(ta_device_code, lv_color_hex(c_text_pri), 0);
    }
    if (ta_device_name) {
        lv_obj_set_style_bg_color(ta_device_name, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(ta_device_name, lv_color_hex(c_border), 0);
        lv_obj_set_style_text_color(ta_device_name, lv_color_hex(c_text_pri), 0);
    }
    if (ta_admin_pin) {
        lv_obj_set_style_bg_color(ta_admin_pin, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(ta_admin_pin, lv_color_hex(c_border), 0);
        lv_obj_set_style_text_color(ta_admin_pin, lv_color_hex(c_text_pri), 0);
    }

    /* Tab Sinkron Elements */
    if (lbl_sync_mode_t) lv_obj_set_style_text_color(lbl_sync_mode_t, lv_color_hex(c_text_pri), 0);
    if (dropdown_sync_mode) {
        lv_obj_set_style_bg_color(dropdown_sync_mode, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(dropdown_sync_mode, lv_color_hex(c_border), 0);
        lv_obj_set_style_text_color(dropdown_sync_mode, lv_color_hex(c_text_pri), 0);
    }
    if (lbl_pending_count) lv_obj_set_style_text_color(lbl_pending_count, lv_color_hex(COLOR_CYAN_TEXT), 0);

    /* Tab Sistem Elements */
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

    /* Tab LPCD Delta & Interval */
    if (lbl_lpcd_title) lv_obj_set_style_text_color(lbl_lpcd_title, lv_color_hex(c_text_pri), 0);
    if (lbl_lpcd_int_title) lv_obj_set_style_text_color(lbl_lpcd_int_title, lv_color_hex(c_text_pri), 0);
    if (dropdown_lpcd_delta) {
        lv_obj_set_style_bg_color(dropdown_lpcd_delta, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(dropdown_lpcd_delta, lv_color_hex(c_border), 0);
        lv_obj_set_style_text_color(dropdown_lpcd_delta, lv_color_hex(c_text_pri), 0);
    }
    if (dropdown_lpcd_interval) {
        lv_obj_set_style_bg_color(dropdown_lpcd_interval, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(dropdown_lpcd_interval, lv_color_hex(c_border), 0);
        lv_obj_set_style_text_color(dropdown_lpcd_interval, lv_color_hex(c_text_pri), 0);
    }

    /* Tab USERS (USERS.CSV) */
    if (lbl_users_title) lv_obj_set_style_text_color(lbl_users_title, lv_color_hex(c_text_pri), 0);
    if (lbl_users_count) lv_obj_set_style_text_color(lbl_users_count, lv_color_hex(COLOR_CYAN_TEXT), 0);
    if (btn_refresh_users) {
        lv_obj_set_style_bg_color(btn_refresh_users, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(btn_refresh_users, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    }
    if (cont_users_list) {
        lv_obj_set_style_bg_color(cont_users_list, lv_color_hex(is_dark ? 0x060E1A : 0xFFFFFF), 0);
        lv_obj_set_style_border_color(cont_users_list, lv_color_hex(c_border), 0);
    }
    if (table_users) {
        lv_obj_set_style_text_color(table_users, lv_color_hex(c_text_pri), 0);
    }

    /* Tab INFO (Diagnostik & Wi-Fi 6) */
    if (lbl_info_title) lv_obj_set_style_text_color(lbl_info_title, lv_color_hex(c_text_pri), 0);
    if (btn_refresh_info) {
        lv_obj_set_style_bg_color(btn_refresh_info, lv_color_hex(c_btn_bg), 0);
        lv_obj_set_style_border_color(btn_refresh_info, lv_color_hex(COLOR_CYAN_ACCENT), 0);
    }
    if (cont_info_box) {
        lv_obj_set_style_bg_color(cont_info_box, lv_color_hex(is_dark ? 0x060E1A : 0xFFFFFF), 0);
        lv_obj_set_style_border_color(cont_info_box, lv_color_hex(c_border), 0);
    }
    if (lbl_info_wifi_status) lv_obj_set_style_text_color(lbl_info_wifi_status, lv_color_hex(c_text_pri), 0);
    if (lbl_info_wifi_ssid) lv_obj_set_style_text_color(lbl_info_wifi_ssid, lv_color_hex(c_text_mut), 0);
    if (lbl_info_wifi_ip_mac) lv_obj_set_style_text_color(lbl_info_wifi_ip_mac, lv_color_hex(c_text_mut), 0);
    if (lbl_info_rfid_chip) lv_obj_set_style_text_color(lbl_info_rfid_chip, lv_color_hex(c_text_pri), 0);
    if (lbl_info_rfid_proto) lv_obj_set_style_text_color(lbl_info_rfid_proto, lv_color_hex(c_text_mut), 0);
    if (lbl_info_fw_ver) lv_obj_set_style_text_color(lbl_info_fw_ver, lv_color_hex(c_text_pri), 0);
    if (lbl_info_mcu_info) lv_obj_set_style_text_color(lbl_info_mcu_info, lv_color_hex(c_text_mut), 0);
    if (lbl_info_mem_info) lv_obj_set_style_text_color(lbl_info_mem_info, lv_color_hex(c_text_mut), 0);
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
        lv_obj_set_style_text_font(lbl_uid, &lv_font_montserrat_16, 0);
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

    /* Cek jika sudah memasukkan digit yang cukup */
    const char *target_pin = (g_app_settings.admin_pin[0] != '\0') ? g_app_settings.admin_pin : "1234";
    size_t target_len = strlen(target_pin);
    if (g_pin_len >= target_len) {
        if (strcmp(g_entered_pin, target_pin) == 0) {
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
            GUI_Attendance_UpdatePendingCount(Storage_GetPendingCount());
            GUI_Attendance_RefreshUsersTable();
            GUI_Attendance_RefreshInfoTab();
        } else {
            /* PIN Salah! */
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
            osDelay(250);
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);

            GUI_Attendance_ShowPopup("AKSES DITOLAK", "PIN yang Anda masukkan salah!\nAkses ke menu admin ditolak.", false);
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

static void btn_save_device_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    const char * code = lv_textarea_get_text(ta_device_code);
    const char * name = lv_textarea_get_text(ta_device_name);
    const char * pin  = lv_textarea_get_text(ta_admin_pin);

    if (strlen(code) == 0 || strlen(name) == 0) {
        GUI_Attendance_ShowPopup("DATA TIDAK LENGKAP", "Kode Device dan Nama Device tidak boleh kosong!", false);
        if (lbl_device_feedback) {
            lv_label_set_text(lbl_device_feedback, "Peringatan: Kode & Nama Device tidak boleh kosong!");
            lv_obj_set_style_text_color(lbl_device_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
        }
        return;
    }

    if (strlen(pin) < 4) {
        GUI_Attendance_ShowPopup("PIN TIDAK VALID", "PIN Admin minimal harus 4 digit angka!", false);
        if (lbl_device_feedback) {
            lv_label_set_text(lbl_device_feedback, "Peringatan: PIN Admin minimal 4 digit!");
            lv_obj_set_style_text_color(lbl_device_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
        }
        return;
    }

    strncpy(g_app_settings.device_code, code, sizeof(g_app_settings.device_code) - 1);
    strncpy(g_app_settings.device_name, name, sizeof(g_app_settings.device_name) - 1);
    strncpy(g_app_settings.admin_pin, pin, sizeof(g_app_settings.admin_pin) - 1);
    App_Settings_Save(&g_app_settings);

    /* Update header title on main screen */
    GUI_Attendance_UpdateDeviceHeader(name, code);

    /* Kirim perintah pendaftaran / registrasi device ke ESP32 */
    esp_tx_item_t item = {0};
    snprintf(item.payload, sizeof(item.payload), "CMD:REGISTER_DEVICE:%s|%s\n", code, name);
    osMessageQueuePut(espTxQueueHandle, &item, 0, 50);

    if (kb_wifi) lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);

    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
    osDelay(50);
    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);

    if (g_wifi_connected) {
        GUI_Attendance_ShowPopup("MENDAFTARKAN DEVICE", "Identitas & PIN disimpan ke Flash.\nSedang mendaftarkan device ke server backend...", true);
        if (lbl_device_feedback) {
            lv_label_set_text(lbl_device_feedback, "Status: Disimpan! Mendaftarkan ke server...");
            lv_obj_set_style_text_color(lbl_device_feedback, lv_color_hex(COLOR_AMBER_STATUS), 0);
        }
    } else {
        GUI_Attendance_ShowPopup("DISIMPAN LOKAL", "Identitas & PIN tersimpan ke Flash.\nRegistrasi server tertunda: Wi-Fi belum terhubung.", false);
        if (lbl_device_feedback) {
            lv_label_set_text(lbl_device_feedback, "Tersimpan Lokal. Registrasi butuh Wi-Fi.");
            lv_obj_set_style_text_color(lbl_device_feedback, lv_color_hex(COLOR_AMBER_STATUS), 0);
        }
    }
}

static void dropdown_sync_cb(lv_event_t * e)
{
    GUI_Attendance_NotifyActivity();
    uint32_t sel = lv_dropdown_get_selected(dropdown_sync_mode);
    g_app_settings.sync_mode = (uint8_t)sel;
    App_Settings_Save(&g_app_settings);

    /* Kirim mode sinkronisasi ke ESP32 */
    esp_tx_item_t item = {0};
    snprintf(item.payload, sizeof(item.payload), "CMD:SET_SYNC_MODE:%d\n", (int)sel);
    osMessageQueuePut(espTxQueueHandle, &item, 0, 50);

    const char *modes[] = {"Realtime (Langsung)", "Periodik 10 Menit", "Periodik 30 Menit", "Periodik 1 Jam", "Periodik 1 Hari"};
    char pop_msg[64];
    snprintf(pop_msg, sizeof(pop_msg), "Mode aktif: %s", (sel < 5) ? modes[sel] : "Realtime");
    GUI_Attendance_ShowPopup("MODE SINKRONISASI", pop_msg, true);

    if (lbl_sync_feedback) {
        lv_label_set_text(lbl_sync_feedback, "Mode sinkronisasi berhasil diubah dan disimpan!");
        lv_obj_set_style_text_color(lbl_sync_feedback, lv_color_hex(COLOR_GREEN_STATUS), 0);
    }
}

static void btn_bulk_sync_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();

    uint32_t pending = Storage_GetPendingCount();
    if (pending == 0) {
        GUI_Attendance_ShowPopup("ANTREAN KOSONG", "Tidak ada data antrean offline di memori (0 data).", true);
        if (lbl_sync_feedback) {
            lv_label_set_text(lbl_sync_feedback, "Antrean transaksi lokal kosong (0 data).");
            lv_obj_set_style_text_color(lbl_sync_feedback, lv_color_hex(COLOR_CYAN_TEXT), 0);
        }
        return;
    }

    if (!g_wifi_connected) {
        GUI_Attendance_ShowPopup("SYNC MANUAL GAGAL", "Gagal sync manual: Wi-Fi belum terhubung!\nHubungkan ke Wi-Fi terlebih dahulu.", false);
        if (lbl_sync_feedback) {
            lv_label_set_text(lbl_sync_feedback, "Gagal: Wi-Fi belum terhubung ke jaringan.");
            lv_obj_set_style_text_color(lbl_sync_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
        }
        return;
    }

    /* Minta Task_Storage untuk memproses bulk sync secara asynchronous di background */
    g_bulk_sync_requested = true;

    char pop_msg[96];
    snprintf(pop_msg, sizeof(pop_msg), "START SYNC MANUAL: Memaksa kirim semua %lu antrean ke server sampai habis...", (unsigned long)pending);
    GUI_Attendance_ShowPopup("SYNC MANUAL BERJALAN", pop_msg, true);

    if (lbl_sync_feedback) {
        lv_label_set_text(lbl_sync_feedback, "Sync Manual: Mengirim antrean sampai habis...");
        lv_obj_set_style_text_color(lbl_sync_feedback, lv_color_hex(COLOR_AMBER_STATUS), 0);
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

static void dropdown_lpcd_delta_cb(lv_event_t * e)
{
    GUI_Attendance_NotifyActivity();
    uint32_t sel = lv_dropdown_get_selected(dropdown_lpcd_delta);
    uint8_t delta = (uint8_t)(sel + 1);
    if (delta < 1) delta = 1;
    if (delta > 8) delta = 8;
    g_app_settings.lpcd_delta = delta;
    App_Settings_Save(&g_app_settings);

    char pop_msg[96];
    snprintf(pop_msg, sizeof(pop_msg), "Delta LPCD diubah ke: %u\n(Sensitivitas Deteksi Kartu)", (unsigned int)delta);
    GUI_Attendance_ShowPopup("SENSITIVITAS LPCD", pop_msg, true);
    if (lbl_admin_feedback) {
        lv_label_set_text_fmt(lbl_admin_feedback, "Status: Delta LPCD diatur ke %u", (unsigned int)delta);
        lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_GREEN_STATUS), 0);
    }
}

static void dropdown_lpcd_interval_cb(lv_event_t * e)
{
    GUI_Attendance_NotifyActivity();
    uint32_t sel = lv_dropdown_get_selected(dropdown_lpcd_interval);
    uint16_t intervals[] = {150, 250, 300, 500};
    uint16_t val = (sel < 4) ? intervals[sel] : 300;
    g_app_settings.lpcd_interval_ms = val;
    App_Settings_Save(&g_app_settings);

    char pop_msg[96];
    snprintf(pop_msg, sizeof(pop_msg), "Interval LPCD diubah ke: %u ms\n(Timer4 Wakeup Interval)", (unsigned int)val);
    GUI_Attendance_ShowPopup("INTERVAL LPCD", pop_msg, true);
    if (lbl_admin_feedback) {
        lv_label_set_text_fmt(lbl_admin_feedback, "Status: LPCD Interval diatur ke %u ms", (unsigned int)val);
        lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_GREEN_STATUS), 0);
    }
}

static void btn_refresh_users_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    GUI_Attendance_RefreshUsersTable();
}

static void btn_refresh_info_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    GUI_Attendance_NotifyActivity();
    GUI_Attendance_RefreshInfoTab();
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

    if (!g_wifi_connected) {
        GUI_Attendance_ShowPopup("SINKRON JAM GAGAL", "Wi-Fi belum terhubung ke jaringan!\nHubungkan ke Wi-Fi terlebih dahulu.", false);
        if (lbl_admin_feedback) {
            lv_label_set_text(lbl_admin_feedback, "Gagal: Wi-Fi belum terhubung!");
            lv_obj_set_style_text_color(lbl_admin_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
        }
        return;
    }

    /* Kirim request sinkronisasi jam ke ESP32 */
    esp_tx_item_t item = {0};
    snprintf(item.payload, sizeof(item.payload), "CMD:SYNC_TIME\n");
    osMessageQueuePut(espTxQueueHandle, &item, 0, 50);

    GUI_Attendance_ShowPopup("SINKRONISASI JAM", "Menghubungi server NTP / Backend...\nMohon tunggu pembacaan waktu server.", true);

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
        GUI_Attendance_ShowPopup("SSID KOSONG", "Nama SSID Wi-Fi tidak boleh kosong!", false);
        if (lbl_wifi_feedback) {
            lv_label_set_text(lbl_wifi_feedback, "Peringatan: SSID tidak boleh kosong!");
            lv_obj_set_style_text_color(lbl_wifi_feedback, lv_color_hex(COLOR_RED_STATUS), 0);
        }
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

    /* Kirim paket konfigurasi Wi-Fi ke ESP32-C6 via USART1 (Gunakan DHCP agar router otomatis memberikan IP) */
    esp_tx_item_t item = {0};
    snprintf(item.payload, sizeof(item.payload), "WIFI:%s|%s|DHCP\n", ssid, pass);
    osMessageQueuePut(espTxQueueHandle, &item, 0, 50);

    /* Sembunyikan keyboard */
    if (kb_wifi) lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);

    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
    osDelay(50);
    HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);

    GUI_Attendance_ShowPopup("WIFI DISIMPAN", "SSID & Password tersimpan ke Flash.\nMengirim konfigurasi ke modul ESP32-C6...", true);

    if (lbl_wifi_feedback) {
        lv_label_set_text(lbl_wifi_feedback, "Status: Konfigurasi Disimpan ke Flash & Dikirim!");
        lv_obj_set_style_text_color(lbl_wifi_feedback, lv_color_hex(COLOR_GREEN_STATUS), 0);
    }
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
    if (code == LV_EVENT_READY) {
        if (kb_wifi) lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
        /* User menekan tombol OK pada keyboard: langsung simpan dan kirim ke ESP32 */
        btn_save_wifi_cb(NULL);
    } else if (code == LV_EVENT_CANCEL) {
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

    char pop_diag[128];
    snprintf(pop_diag, sizeof(pop_diag), "RFID: %s | SD: %s\nWiFi: %s | RTC: %s",
             ok_rfid ? "OK" : "ERR", ok_sd ? "OK" : "ERR",
             ok_wifi ? "OK" : "ERR", ok_rtc ? "OK" : "ERR");
    GUI_Attendance_ShowPopup("HASIL DIAGNOSTIK", pop_diag, (ok_rfid && ok_sd && ok_wifi && ok_rtc));
}

void GUI_Attendance_UpdateDeviceHeader(const char *name, const char *code)
{
    if (lbl_header_title) {
        if (name && name[0] != '\0') {
            lv_label_set_text(lbl_header_title, name);
        } else if (code && code[0] != '\0') {
            lv_label_set_text(lbl_header_title, code);
        } else {
            lv_label_set_text(lbl_header_title, "ABSENSI RFID");
        }
    }
}

void GUI_Attendance_UpdatePendingCount(uint32_t count)
{
    if (lbl_pending_count) {
        lv_label_set_text_fmt(lbl_pending_count, "Antrian Belum Sinkron: %lu data", (unsigned long)count);
    }
}

void GUI_Attendance_SetBulkStatus(const char *status, bool is_ok)
{
    if (lbl_sync_feedback && status) {
        lv_label_set_text(lbl_sync_feedback, status);
        lv_obj_set_style_text_color(lbl_sync_feedback,
            lv_color_hex(is_ok ? COLOR_GREEN_STATUS : COLOR_RED_STATUS), 0);
    }
}

void GUI_Attendance_RefreshUsersTable(void)
{
    if (table_users == NULL) return;

    user_entry_t users[32];
    uint16_t count = Storage_GetRegisteredUsers(users, 32);

    if (lbl_users_count) {
        lv_label_set_text_fmt(lbl_users_count, "Total: %u user", (unsigned int)count);
    }

    lv_table_set_row_count(table_users, count > 0 ? (count + 1) : 2);
    lv_table_set_cell_value(table_users, 0, 0, "#");
    lv_table_set_cell_value(table_users, 0, 1, "UID KARTU");
    lv_table_set_cell_value(table_users, 0, 2, "NAMA KARYAWAN");
    lv_table_set_cell_value(table_users, 0, 3, "JABATAN");

    if (count == 0) {
        lv_table_set_cell_value(table_users, 1, 0, "-");
        lv_table_set_cell_value(table_users, 1, 1, "--");
        lv_table_set_cell_value(table_users, 1, 2, "TIDAK ADA USER LOKAL");
        lv_table_set_cell_value(table_users, 1, 3, "-");
        return;
    }

    for (uint16_t i = 0; i < count; i++) {
        uint16_t row = i + 1;
        char num_str[8];
        snprintf(num_str, sizeof(num_str), "%u", (unsigned int)row);
        lv_table_set_cell_value(table_users, row, 0, num_str);
        lv_table_set_cell_value(table_users, row, 1, users[i].uid);
        lv_table_set_cell_value(table_users, row, 2, users[i].name);
        lv_table_set_cell_value(table_users, row, 3, (users[i].pos[0] != '\0') ? users[i].pos : "-");
    }
}

void GUI_Attendance_UpdateWifiInfo(const wifi_info_t *info)
{
    if (info == NULL) return;

    if (lbl_info_wifi_proto) {
        if (info->is_wifi6) {
            lv_label_set_text(lbl_info_wifi_proto, "- Tipe: Wi-Fi 6 (802.11ax) [OFDMA High-Efficiency]");
            lv_obj_set_style_text_color(lbl_info_wifi_proto, lv_color_hex(COLOR_GREEN_STATUS), 0);
        } else if (info->protocol[0] != '\0') {
            lv_label_set_text_fmt(lbl_info_wifi_proto, "- Tipe: %s", info->protocol);
            lv_obj_set_style_text_color(lbl_info_wifi_proto, lv_color_hex(COLOR_CYAN_TEXT), 0);
        } else {
            lv_label_set_text(lbl_info_wifi_proto, "- Tipe: Memeriksa modul Wi-Fi ESP32-C6...");
            lv_obj_set_style_text_color(lbl_info_wifi_proto, lv_color_hex(COLOR_AMBER_STATUS), 0);
        }
    }

    if (lbl_info_wifi_status) {
        if (strcmp(info->status, "CONNECTED") == 0) {
            lv_label_set_text_fmt(lbl_info_wifi_status, "- Status: CONNECTED (RSSI: %s)", info->rssi);
            lv_obj_set_style_text_color(lbl_info_wifi_status, lv_color_hex(COLOR_GREEN_STATUS), 0);
        } else {
            lv_label_set_text_fmt(lbl_info_wifi_status, "- Status: %s", (info->status[0] != '\0') ? info->status : "DISCONNECTED");
            lv_obj_set_style_text_color(lbl_info_wifi_status, lv_color_hex(COLOR_AMBER_STATUS), 0);
        }
    }

    if (lbl_info_wifi_ssid) {
        lv_label_set_text_fmt(lbl_info_wifi_ssid, "- SSID: %s | IP: %s",
                              (info->ssid[0] != '\0') ? info->ssid : "-",
                              (info->ip[0] != '\0') ? info->ip : "-");
    }

    if (lbl_info_wifi_ip_mac) {
        lv_label_set_text_fmt(lbl_info_wifi_ip_mac, "- MAC: %s",
                              (info->mac[0] != '\0') ? info->mac : "-");
    }

    if (lbl_info_rfid_lpcd) {
        lv_label_set_text_fmt(lbl_info_rfid_lpcd, "- Mode LPCD: Aktif (PA15 Wakeup, Delta: %u, %u ms)",
                              (unsigned int)g_app_settings.lpcd_delta,
                              (unsigned int)g_app_settings.lpcd_interval_ms);
    }
}

void GUI_Attendance_RefreshInfoTab(void)
{
    /* Kirim request query info Wi-Fi ke ESP32 */
    esp_tx_item_t item = {0};
    snprintf(item.payload, sizeof(item.payload), "CMD:GET_WIFI_INFO\n");
    osMessageQueuePut(espTxQueueHandle, &item, 0, 50);

    /* Perbarui label dengan data yang saat ini ada */
    GUI_Attendance_UpdateWifiInfo(&g_wifi_info);
}

