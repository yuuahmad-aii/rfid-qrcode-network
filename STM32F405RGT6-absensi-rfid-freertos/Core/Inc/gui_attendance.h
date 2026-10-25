/**
  ******************************************************************************
  * @file    gui_attendance.h
  * @brief   LVGL Modern GUI for RFID Attendance System & Admin Settings
  ******************************************************************************
  */

#ifndef __GUI_ATTENDANCE_H
#define __GUI_ATTENDANCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef UI_EVENT_DEFINED
#define UI_EVENT_DEFINED
typedef enum {
    UI_EVENT_NONE = 0,
    UI_EVENT_RFID_CARD_DETECTED,
    UI_EVENT_ATTENDANCE_SUCCESS,
    UI_EVENT_ATTENDANCE_REJECTED,
    UI_EVENT_WIFI_STATUS,
    UI_EVENT_SD_STATUS,
    UI_EVENT_NTP_SYNCED,
    UI_EVENT_NTP_FAILED,
    UI_EVENT_BULK_SYNC_DONE,
    UI_EVENT_DEVICE_REG_RESULT
} ui_event_type_t;

typedef struct {
    uint8_t type;             /* ui_event_type_t (1 byte) */
    uint8_t status;           /* Status flag: 0=error/fail, 1=success (1 byte) */
    uint8_t uid_len;          /* Card UID length: 4, 7, or 10 bytes (1 byte) */
    uint8_t uid[10];          /* Card UID buffer (10 bytes) */
    char    extra[51];        /* Employee name, reason, or status info (51 bytes) */
} ui_event_t;                 /* Exactly 64 bytes */
#endif

#ifndef ESP_TX_ITEM_DEFINED
#define ESP_TX_ITEM_DEFINED
typedef struct {
    char payload[128];        /* Formatted string packet to ESP32 (128 bytes) */
} esp_tx_item_t;              /* Exactly 128 bytes (matches espTxQueue item size) */
#endif

/**
  * @brief  Inisialisasi antarmuka pengguna grafis LVGL untuk sistem absensi & menu admin
  */
void GUI_Attendance_Init(void);

/**
  * @brief  Proses event dari queue (misal dari Task_RFID, Task_ESP32)
  * @param  evt Pointer ke struct ui_event_t
  */
void GUI_Attendance_ProcessEvent(const ui_event_t *evt);

/**
  * @brief  Perbarui tampilan jam digital dan tanggal pada header
  * @param  sTime Pointer ke RTC_TimeTypeDef
  * @param  sDate Pointer ke RTC_DateTypeDef
  */
void GUI_Attendance_UpdateClock(const RTC_TimeTypeDef *sTime, const RTC_DateTypeDef *sDate);

/**
  * @brief  Perbarui status badge peripheral (RFID, Wi-Fi, MicroSD) secara real-time
  * @param  rfid_ok Status hardware reader CLRC663
  * @param  wifi_ok Status koneksi Wi-Fi ESP32-C6
  * @param  sd_ok   Status filesystem MicroSD FatFS
  */
void GUI_Attendance_UpdateBadges(bool rfid_ok, bool wifi_ok, bool sd_ok);

/**
  * @brief  Deteksi sentuhan tahan pojok layar (3 detik) untuk navigasi ke menu admin
  * @param  x Koordinat sentuh X (0 jika dilepas)
  * @param  y Koordinat sentuh Y (0 jika dilepas)
  */
void GUI_Attendance_CheckCornerTouch(uint16_t x, uint16_t y);

/**
  * @brief  Reset hitungan total absensi
  */
void GUI_Attendance_ResetCounter(void);

/**
  * @brief  Cek dan eksekusi screen sleep/dim jika idle timeout tercapai
  */
void GUI_Attendance_CheckSleep(void);

/**
  * @brief  Bangunkan layar dari mode sleep
  */
void GUI_Attendance_Wakeup(void);

/**
  * @brief  Periksa apakah layar sedang dalam kondisi sleep/mati
  */
bool GUI_Attendance_IsSleeping(void);

/**
  * @brief  Reset timer aktivitas layar sentuh
  */
void GUI_Attendance_NotifyActivity(void);

/**
  * @brief  Tampilkan atau sembunyikan popup modal peringatan MicroSD dicabut
  * @param  show true untuk menampilkan peringatan, false untuk menyembunyikan
  */
void GUI_Attendance_ShowSDWarning(bool show);

/**
  * @brief  Fungsi pembaca riwayat log absensi terakhir dari MicroSD (ATTEND.CSV)
  * @param  logs Array string buffer untuk menampung baris log
  * @param  max_logs Jumlah baris log maksimum yang dibaca
  * @return Jumlah baris log yang berhasil dibaca
  */
uint16_t Storage_ReadRecentLogs(char logs[][64], uint16_t max_logs);

/* ========================================================================== */
/*                PERSISTENT CONFIGURATION (STM32 FLASH SECTOR 11)            */
/* ========================================================================== */
#define SETTINGS_FLASH_SECTOR       FLASH_SECTOR_11
#define SETTINGS_FLASH_ADDR         0x080E0000U
#define SETTINGS_MAGIC              0xAA55F405U
#define SETTINGS_VERSION            2

#define SYNC_MODE_REALTIME          0
#define SYNC_MODE_10_MIN            1
#define SYNC_MODE_30_MIN            2
#define SYNC_MODE_1_HOUR            3
#define SYNC_MODE_1_DAY             4

typedef struct {
    uint32_t magic;              /* 0xAA55F405 (4 bytes) */
    uint16_t version;            /* Version 2 (2 bytes) */
    uint16_t checksum;           /* Checksum (2 bytes) */
    uint8_t  brightness;         /* 10 - 100 (%) (1 byte) */
    uint8_t  theme_mode;         /* 1 = Dark Mode, 0 = Light Mode (1 byte) */
    uint16_t screen_timeout_sec; /* 0 (Never), 30, 60, 120, 300 (2 bytes) */
    char     wifi_ssid[34];      /* Wi-Fi SSID string (34 bytes) */
    char     wifi_pass[66];      /* Wi-Fi Password string (66 bytes) */
    char     server_ip[16];      /* Server IP if needed (16 bytes) */
    char     device_code[24];    /* e.g. "GATE-001" (24 bytes) */
    char     device_name[32];    /* e.g. "Gate Utama" (32 bytes) */
    char     admin_pin[10];      /* e.g. "1234" (10 bytes) */
    uint8_t  sync_mode;          /* SYNC_MODE_REALTIME..SYNC_MODE_1_DAY (1 byte) */
    uint8_t  lpcd_delta;         /* LPCD threshold delta: 1 - 10 (default: 3) (1 byte) */
    uint16_t lpcd_interval_ms;   /* LPCD sleep interval: 150, 250, 300, 500 ms (default: 300) (2 bytes) */
    uint8_t  reserved[58];       /* Padding to exactly 256 bytes total */
} app_settings_t;

extern app_settings_t g_app_settings;

void App_Settings_Init(void);
bool App_Settings_Load(app_settings_t *out_settings);
bool App_Settings_Save(const app_settings_t *in_settings);
void App_Settings_ResetDefaults(app_settings_t *settings);

/* ========================================================================== */
/*                       WI-FI 6 & DEVICE DIAGNOSTICS INFO                    */
/* ========================================================================== */
typedef struct {
    char status[16];     /* "CONNECTED" / "DISCONNECTED" */
    char ssid[34];       /* SSID */
    char ip[16];         /* IP Address */
    char rssi[16];       /* RSSI */
    char protocol[32];   /* "Wi-Fi 6 (802.11ax)" / "Wi-Fi 4 (802.11n)" */
    char mac[20];        /* MAC Address */
    bool is_wifi6;       /* true jika Wi-Fi 6 */
} wifi_info_t;

extern wifi_info_t g_wifi_info;

typedef struct {
    char uid[20];
    char name[36];
    char pos[24];
} user_entry_t;

uint16_t Storage_GetRegisteredUsers(user_entry_t *out_users, uint16_t max_users);
void     Storage_SanitizeUsersFile(void);
void     GUI_Attendance_UpdateWifiInfo(const wifi_info_t *info);
void     GUI_Attendance_RefreshUsersTable(void);
void     GUI_Attendance_RefreshInfoTab(void);

/* ========================================================================== */
/*                       OFFLINE-FIRST & SYNC HELPERS                         */
/* ========================================================================== */
extern volatile bool g_bulk_sync_requested;
void GUI_Attendance_ShowPopup(const char *title, const char *desc, bool is_success);
void GUI_Attendance_UpdateDeviceHeader(const char *name, const char *code);
void GUI_Attendance_UpdatePendingCount(uint32_t count);
void GUI_Attendance_SetBulkStatus(const char *msg, bool is_ok);
uint32_t Storage_GetPendingCount(void);
bool Storage_LookupEmployee(const char *uid_str, char *out_name, size_t name_len, char *out_pos, size_t pos_len);
bool Storage_SaveEmployee(const char *uid_str, const char *emp_name, const char *emp_pos);
void Storage_WarmupCache(void);
void Storage_TriggerBulkSync(void);

/* ========================================================================== */
/*                       HARDWARE DIAGNOSTIC TESTS                            */
/* ========================================================================== */
bool Storage_TestDiagnostic(char *out_msg, size_t max_len);
bool RFID_TestDiagnostic(char *out_msg, size_t max_len);
bool WiFi_GetDiagnostic(char *out_msg, size_t max_len);
bool RTC_GetDiagnostic(char *out_msg, size_t max_len);
void Buzzer_TestDiagnostic(void);

#ifdef __cplusplus
}
#endif

#endif /* __GUI_ATTENDANCE_H */
