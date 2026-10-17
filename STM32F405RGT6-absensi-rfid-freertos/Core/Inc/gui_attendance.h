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
    UI_EVENT_NTP_FAILED
} ui_event_type_t;

typedef struct {
    uint8_t type;             /* ui_event_type_t (1 byte) */
    uint8_t status;           /* Status flag: 0=error/fail, 1=success (1 byte) */
    uint8_t uid_len;          /* Card UID length: 4, 7, or 10 bytes (1 byte) */
    uint8_t uid[10];          /* Card UID buffer (10 bytes) */
    char    extra[19];        /* Employee name, reason, or status info (19 bytes) */
} ui_event_t;                 /* Exactly 32 bytes */
#endif

#ifndef ESP_TX_ITEM_DEFINED
#define ESP_TX_ITEM_DEFINED
typedef struct {
    char payload[64];         /* Formatted string packet to ESP32 (64 bytes) */
} esp_tx_item_t;              /* Exactly 64 bytes (matches espTxQueue item size) */
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
#define SETTINGS_VERSION            1

typedef struct {
    uint32_t magic;              /* 0xAA55F405 */
    uint16_t version;            /* Version 1 */
    uint16_t checksum;           /* Checksum */
    uint8_t  brightness;         /* 10 - 100 (%) */
    uint8_t  theme_mode;         /* 1 = Dark Mode, 0 = Light Mode */
    uint16_t screen_timeout_sec; /* 0 (Never), 30, 60, 120, 300 */
    char     wifi_ssid[34];      /* Wi-Fi SSID string */
    char     wifi_pass[66];      /* Wi-Fi Password string */
    char     server_ip[16];      /* Server IP if needed */
    uint8_t  reserved[134];      /* Padding to 256 bytes for future expansion */
} app_settings_t;

extern app_settings_t g_app_settings;

void App_Settings_Init(void);
bool App_Settings_Load(app_settings_t *out_settings);
bool App_Settings_Save(const app_settings_t *in_settings);
void App_Settings_ResetDefaults(app_settings_t *settings);

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
