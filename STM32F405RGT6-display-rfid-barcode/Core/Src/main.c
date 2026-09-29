/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ili9488.h"
#include "xpt2046.h"
#include "clrc663.h"
#include "fonts.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "lvgl.h"
#include "eez/ui.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define COLOR_BG          0x0821   // Dark Charcoal / Navy Background
#define COLOR_CARD_HERO   0x18C3   // Dark Slate Hero Card
#define COLOR_CARD_TOUCH  0x10A2   // Dark Slate Touch Card
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

CAN_HandleTypeDef hcan1;

RTC_HandleTypeDef hrtc;

SD_HandleTypeDef hsd;
DMA_HandleTypeDef hdma_sdio;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;
DMA_HandleTypeDef hdma_spi3_rx;
DMA_HandleTypeDef hdma_spi3_tx;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim14;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_CAN1_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_SPI3_Init(void);
static void MX_UART4_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_RTC_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM14_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);

void App_DrawMainUI(void);
void App_RFIDProcess(void);
void App_TouchProcess(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  ILI9488_DrawBitmapLVGL(area->x1, area->y1, area->x2, area->y2, px_map);
  lv_display_flush_ready(disp);
}

// Forward declarations for EEZ Studio functions
extern void loadScreen(enum ScreensEnum screenId);
extern objects_t objects;

// Timer callback to switch to Admin Password screen
static lv_timer_t *admin_timer = NULL;
static void go_admin_cb(lv_timer_t *t) {
  if (objects.textarea_input_password != NULL) {
    lv_textarea_set_text(objects.textarea_input_password, "");
  }
  loadScreen(SCREEN_ID_ADMIN_PASSWORD);
}

// Admin login button pressed on Main Screen
void action_btn_admin_on_pressed(lv_event_t *e) {
  if (admin_timer == NULL) {
    admin_timer = lv_timer_create(go_admin_cb, 50, NULL);
    lv_timer_set_repeat_count(admin_timer, 1);
  } else {
    lv_timer_resume(admin_timer);
    lv_timer_reset(admin_timer);
  }
}

// Dummy for unused actions
void action_button_matrix_password_pressed(lv_event_t * e) {}

// Keypad Actions
static void add_pwd_char(const char *c) {
    if (objects.textarea_input_password != NULL) {
        lv_textarea_add_text(objects.textarea_input_password, c);
    }
}
void action_pwd_btn_1_pressed(lv_event_t * e) { add_pwd_char("1"); }
void action_pwd_btn_2_pressed(lv_event_t * e) { add_pwd_char("2"); }
void action_pwd_btn_3_pressed(lv_event_t * e) { add_pwd_char("3"); }
void action_pwd_btn_4_pressed(lv_event_t * e) { add_pwd_char("4"); }
void action_pwd_btn_5_pressed(lv_event_t * e) { add_pwd_char("5"); }
void action_pwd_btn_6_pressed(lv_event_t * e) { add_pwd_char("6"); }
void action_pwd_btn_7_pressed(lv_event_t * e) { add_pwd_char("7"); }
void action_pwd_btn_8_pressed(lv_event_t * e) { add_pwd_char("8"); }
void action_pwd_btn_9_pressed(lv_event_t * e) { add_pwd_char("9"); }
void action_pwd_btn_0_pressed(lv_event_t * e) { add_pwd_char("0"); }
void action_pwd_btn_del_pressed(lv_event_t * e) {
    if (objects.textarea_input_password != NULL) {
        lv_textarea_delete_char(objects.textarea_input_password);
    }
}
void action_pwd_btn_ok_pressed(lv_event_t * e) {
    if (objects.textarea_input_password != NULL) {
        const char *pwd = lv_textarea_get_text(objects.textarea_input_password);
        if (strcmp(pwd, "123456") == 0) { // Contoh password "123456"
            loadScreen(SCREEN_ID_ADMIN_USERS);
        } else {
            // Wrong password, clear textarea
            lv_textarea_set_text(objects.textarea_input_password, "");
        }
    }
}

// Load Screens
void action_load_admin_users(lv_event_t * e) { loadScreen(SCREEN_ID_ADMIN_USERS); }
void action_load_admin_history(lv_event_t * e) { loadScreen(SCREEN_ID_ADMIN_HISTORY); }
void action_load_admin_settings(lv_event_t * e) { loadScreen(SCREEN_ID_ADMIN_SETTINGS); }
void action_load_admin_test(lv_event_t * e) { loadScreen(SCREEN_ID_ADMIN_TEST); }
void action_load_main(lv_event_t * e) { loadScreen(SCREEN_ID_MAIN_SCREEN); }


static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  uint16_t touch_x = 0, touch_y = 0;
  if (XPT2046_GetTouch(&touch_x, &touch_y)) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = touch_x;
    data->point.y = touch_y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/* Global RFID & System states */
bool g_clrc663_ready = false;
uint8_t g_clrc663_version = 0;
uint32_t g_card_read_count = 0;
bool g_card_active = false;

/**
 * @brief  Menggambar antarmuka utama RFID & Display pada layar ILI9488
 */
void App_DrawMainUI(void) {
	// 1. Bersihkan layar dengan warna latar gelap modern
	ILI9488_FillScreen(COLOR_BG);

	// 2. Header Bar di bagian atas
	ILI9488_FillRectangle(0, 0, ILI9488_WIDTH, 38, ILI9488_NAVY);
	ILI9488_DrawLine(0, 38, ILI9488_WIDTH - 1, 38, ILI9488_CYAN);
	ILI9488_WriteStringScaled(18, 10, "STM32F405 - RFID & DISPLAY SYSTEM", Font_7x10,
			ILI9488_WHITE, ILI9488_NAVY, 2);

	// 3. Card Panel RFID Reader (NXP CLRC66303 - SPI3)
	ILI9488_FillRectangle(15, 44, 450, 130, COLOR_CARD_HERO);
	ILI9488_DrawRectangle(15, 44, 450, 130, ILI9488_CYAN);

	// Judul modul RFID & Status Chip
	ILI9488_WriteStringScaled(25, 52, "RFID CLRC66303 (SPI3)", Font_7x10,
			ILI9488_CYAN, COLOR_CARD_HERO, 2);

	if (g_clrc663_ready) {
		char ver_buf[20];
		snprintf(ver_buf, sizeof(ver_buf), "[READY v0x%02X]", g_clrc663_version);
		ILI9488_WriteString(320, 54, ver_buf, Font_7x10, ILI9488_GREEN, COLOR_CARD_HERO);
	} else {
		ILI9488_WriteString(320, 54, "[CHIP NOT FOUND]", Font_7x10, ILI9488_RED, COLOR_CARD_HERO);
	}

	ILI9488_DrawLine(25, 72, 450, 72, ILI9488_DARKGREY);

	// Informasi status pembacaan RFID
	ILI9488_WriteString(25, 78, "Status  : Dekatkan Kartu RFID (ISO14443A)...",
			Font_7x10, ILI9488_YELLOW, COLOR_CARD_HERO);
	ILI9488_WriteStringScaled(25, 96, "UID: -- -- -- --          ", Font_7x10,
			ILI9488_WHITE, COLOR_CARD_HERO, 2);
	ILI9488_WriteString(25, 126, "Tipe    : - (Menunggu kartu...)               ",
			Font_7x10, ILI9488_LIGHTGREY, COLOR_CARD_HERO);
	ILI9488_WriteString(25, 146, "Baca    : 0 kali  |  ATQA: --  |  SAK: --     ",
			Font_7x10, ILI9488_LIGHTGREY, COLOR_CARD_HERO);

	// 4. Card Panel Touch Screen (XPT2046 - SPI2)
	ILI9488_FillRectangle(15, 182, 450, 116, COLOR_CARD_TOUCH);
	ILI9488_DrawRectangle(15, 182, 450, 116, ILI9488_DARKGREY);

	// Header panel sentuh
	ILI9488_WriteStringScaled(25, 190, "TOUCH SCREEN (XPT2046)", Font_7x10,
			ILI9488_CYAN, COLOR_CARD_TOUCH, 2);

	// Status & Koordinat awal
	ILI9488_WriteString(25, 216, "Status  : Menunggu Sentuhan...", Font_7x10,
			ILI9488_WHITE, COLOR_CARD_TOUCH);
	ILI9488_WriteStringScaled(25, 236, "X : ---   |   Y : ---", Font_7x10,
			ILI9488_GREEN, COLOR_CARD_TOUCH, 2);
	ILI9488_WriteString(25, 276, "Sentuh layar atau dekatkan kartu RFID!",
			Font_7x10, ILI9488_LIGHTGREY, COLOR_CARD_TOUCH);

	// Tombol Interaktif di sebelah kanan
	ILI9488_FillRectangle(325, 195, 125, 88, ILI9488_BLUE);
	ILI9488_DrawRectangle(325, 195, 125, 88, ILI9488_WHITE);
	ILI9488_WriteStringScaled(340, 216, "SENTUH", Font_7x10, ILI9488_WHITE,
			ILI9488_BLUE, 2);
	ILI9488_WriteStringScaled(340, 242, "DISINI", Font_7x10, ILI9488_WHITE,
			ILI9488_BLUE, 2);

	// 5. Footer status bar
	ILI9488_WriteString(18, 305, "SPI1: LCD ILI9488 | SPI2: XPT2046 Touch | SPI3: CLRC663 RFID",
			Font_7x10, ILI9488_DARKGREY, COLOR_BG);
}

/**
 * @brief  Membaca dan memproses kartu RFID dari chip NXP CLRC66303 (SPI3)
 */
void App_RFIDProcess(void) {
	static uint32_t last_scan_time = 0;
	static uint32_t last_card_seen = 0;
	static uint8_t prev_uid[10] = {0};
	static uint8_t prev_uid_len = 0;

	// Jalankan pembacaan setiap 80 ms
	if (HAL_GetTick() - last_scan_time < 80) {
		return;
	}
	last_scan_time = HAL_GetTick();

	// Jika modul belum siap, coba inisialisasi ulang
	if (!g_clrc663_ready) {
		if (CLRC663_Init(&hspi3) == CLRC663_OK) {
			g_clrc663_ready = true;
			g_clrc663_version = CLRC663_ReadVersion();
			char ver_buf[20];
			snprintf(ver_buf, sizeof(ver_buf), "[READY v0x%02X]", g_clrc663_version);
			ILI9488_WriteString(320, 54, ver_buf, Font_7x10, ILI9488_GREEN, COLOR_CARD_HERO);
		}
		return;
	}

	CLRC663_Card_t card;
	if (CLRC663_ReadCard(&card)) {
		last_card_seen = HAL_GetTick();

		// Periksa apakah ini kartu baru atau UID berbeda
		bool is_new_card = false;
		if (!g_card_active || prev_uid_len != card.uid_len ||
			memcmp(prev_uid, card.uid, card.uid_len) != 0) {
			is_new_card = true;
			memcpy(prev_uid, card.uid, card.uid_len);
			prev_uid_len = card.uid_len;
			g_card_read_count++;
		}

		if (is_new_card || !g_card_active) {
			g_card_active = true;
			HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_SET);

			// Berikan highlight bingkai Hijau Neon saat kartu terdeteksi
			ILI9488_DrawRectangle(15, 44, 450, 130, ILI9488_GREENYELLOW);

			// Tampilkan status terdeteksi
			ILI9488_WriteString(25, 78, "Status  : KARTU TERDETEKSI!                  ",
					Font_7x10, ILI9488_GREENYELLOW, COLOR_CARD_HERO);

			// Format UID ke string Hex
			char uid_str[48] = "UID: ";
			for (uint8_t i = 0; i < card.uid_len; i++) {
				char byte_buf[6];
				snprintf(byte_buf, sizeof(byte_buf), "%02X ", card.uid[i]);
				strcat(uid_str, byte_buf);
			}
			size_t len = strlen(uid_str);
			while (len < 26) {
				uid_str[len++] = ' ';
			}
			uid_str[len] = '\0';

			ILI9488_WriteStringScaled(25, 96, uid_str, Font_7x10,
					ILI9488_GREEN, COLOR_CARD_HERO, 2);

			// Tampilkan tipe kartu
			char type_buf[48];
			snprintf(type_buf, sizeof(type_buf), "Tipe    : %-32s", card.type_name);
			ILI9488_WriteString(25, 126, type_buf, Font_7x10,
					ILI9488_WHITE, COLOR_CARD_HERO);

			// Tampilkan rincian ATQA, SAK, dan counter pembacaan
			char info_buf[64];
			snprintf(info_buf, sizeof(info_buf), "Baca    : %lu kali | ATQA: 0x%04X | SAK: 0x%02X    ",
					(unsigned long)g_card_read_count, card.atqa, card.sak);
			ILI9488_WriteString(25, 146, info_buf, Font_7x10,
					ILI9488_CYAN, COLOR_CARD_HERO);
		}
	} else {
		// Jika kartu sudah diangkat setelah delay 400ms, kembalikan ke status siap
		if (g_card_active && (HAL_GetTick() - last_card_seen > 400)) {
			g_card_active = false;
			prev_uid_len = 0;
			memset(prev_uid, 0, sizeof(prev_uid));
			HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

			// Kembalikan bingkai ke Cyan
			ILI9488_DrawRectangle(15, 44, 450, 130, ILI9488_CYAN);

			// Kembalikan teks status
			ILI9488_WriteString(25, 78, "Status  : Dekatkan Kartu RFID (ISO14443A)...",
					Font_7x10, ILI9488_YELLOW, COLOR_CARD_HERO);
			ILI9488_WriteStringScaled(25, 96, "UID: -- -- -- --          ", Font_7x10,
					ILI9488_WHITE, COLOR_CARD_HERO, 2);
			ILI9488_WriteString(25, 126, "Tipe    : - (Menunggu kartu...)               ",
					Font_7x10, ILI9488_LIGHTGREY, COLOR_CARD_HERO);
		}
	}
}

/**
 * @brief  Membaca dan memproses input sentuh dari chip XPT2046
 */
void App_TouchProcess(void) {
	uint16_t x = 0, y = 0;
	static uint32_t last_touch_time = 0;
	static bool is_touched = false;
	static uint32_t touch_count = 0;
	static uint16_t prev_x = 0, prev_y = 0;

	if (XPT2046_GetTouch(&x, &y)) {
		HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_SET);
		last_touch_time = HAL_GetTick();

		if (!is_touched) {
			is_touched = true;
			touch_count++;

			// Animasi tombol ketika ditekan (ubah ke warna Hijau Gelap)
			ILI9488_FillRectangle(325, 195, 125, 88, ILI9488_DARKGREEN);
			ILI9488_DrawRectangle(325, 195, 125, 88, ILI9488_GREENYELLOW);
			ILI9488_WriteStringScaled(335, 216, "TERTEKAN", Font_7x10,
					ILI9488_WHITE, ILI9488_DARKGREEN, 2);
			ILI9488_WriteStringScaled(350, 242, "OK!", Font_7x10,
					ILI9488_YELLOW, ILI9488_DARKGREEN, 2);

			// Perbarui status sentuh
			ILI9488_WriteString(25, 216, "Status  : Layar Disentuh!     ",
					Font_7x10, ILI9488_GREENYELLOW, COLOR_CARD_TOUCH);
		}

		// Perbarui koordinat hanya jika ada perpindahan posisi
		if (x != prev_x || y != prev_y) {
			char coord_buf[32];
			snprintf(coord_buf, sizeof(coord_buf), "X : %03u   |   Y : %03u", x, y);
			ILI9488_WriteStringScaled(25, 236, coord_buf, Font_7x10,
					ILI9488_GREEN, COLOR_CARD_TOUCH, 2);

			char count_buf[48];
			snprintf(count_buf, sizeof(count_buf), "Total Sentuhan: %lu kali  [X:%u, Y:%u]    ",
					(unsigned long)touch_count, x, y);
			ILI9488_WriteString(25, 276, count_buf, Font_7x10, ILI9488_CYAN,
					COLOR_CARD_TOUCH);

			// Gambar titik kecil penanda lokasi sentuh jika di luar teks
			if (y < 180 || y > 300 || x < 15 || x > 465) {
				ILI9488_FillRectangle(x > 1 ? x - 1 : 0, y > 1 ? y - 1 : 0, 3, 3,
						ILI9488_RED);
			}

			prev_x = x;
			prev_y = y;
		}
	} else {
		// Efek debounce lepas sentuhan setelah 150 ms
		if (is_touched && (HAL_GetTick() - last_touch_time > 150)) {
			is_touched = false;
			if (!g_card_active) {
				HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);
			}

			// Kembalikan tombol ke warna Biru normal
			ILI9488_FillRectangle(325, 195, 125, 88, ILI9488_BLUE);
			ILI9488_DrawRectangle(325, 195, 125, 88, ILI9488_WHITE);
			ILI9488_WriteStringScaled(340, 216, "SENTUH", Font_7x10,
					ILI9488_WHITE, ILI9488_BLUE, 2);
			ILI9488_WriteStringScaled(340, 242, "DISINI", Font_7x10,
					ILI9488_WHITE, ILI9488_BLUE, 2);

			// Kembalikan teks status
			ILI9488_WriteString(25, 216, "Status  : Menunggu Sentuhan...",
					Font_7x10, ILI9488_WHITE, COLOR_CARD_TOUCH);
		}
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_CAN1_Init();
  MX_SDIO_SD_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
  MX_UART4_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USB_HOST_Init();
  MX_TIM3_Init();
  MX_FATFS_Init();
  MX_RTC_Init();
  MX_ADC1_Init();
  MX_TIM14_Init();
  /* USER CODE BEGIN 2 */
	// 1. Nyalakan Backlight Layar via PWM TIM3 Channel 3 (PB0) ~75% brightness
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 49152);

	// 2. Inisialisasi Layar TFT ILI9488 (SPI1) dan Touch Controller XPT2046 (SPI2)
	ILI9488_Init();
	XPT2046_Init();

	// 3. Inisialisasi RFID Reader NXP CLRC66303 (SPI3)
	if (CLRC663_Init(&hspi3) == CLRC663_OK) {
		g_clrc663_ready = true;
		g_clrc663_version = CLRC663_ReadVersion();
	} else {
		g_clrc663_ready = false;
		g_clrc663_version = CLRC663_ReadVersion();
	}

	// 4. Tampilkan Antarmuka Utama RFID & Touch pada Layar LCD
	// App_DrawMainUI(); // Commented out for LVGL

	// --- LVGL Setup ---
	lv_init();

	// 1. Display Setup
	lv_display_t *disp = lv_display_create(ILI9488_WIDTH, ILI9488_HEIGHT);
	lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);
	lv_display_set_flush_cb(disp, my_disp_flush);

	// Create a draw buffer for LVGL
	#define DRAW_BUF_SIZE (ILI9488_WIDTH * ILI9488_HEIGHT / 10 * 1)
	static uint8_t buf1[DRAW_BUF_SIZE] __attribute__((aligned(64)));
	lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

	// 2. Input Device Setup (Touch)
	lv_indev_t *indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, my_touchpad_read);

	// Start TIM14 for LVGL tick
	HAL_TIM_Base_Start_IT(&htim14);

	// Initialize EEZ Studio UI
	ui_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	uint32_t last_tick = HAL_GetTick();
	uint32_t last_heartbeat = 0;
	while (1) {
		uint32_t current_tick = HAL_GetTick();
		if (current_tick > last_tick) {
			lv_tick_inc(current_tick - last_tick);
		}
		last_tick = current_tick;
		lv_timer_handler();

		MX_USB_HOST_Process();

		// 1. Proses pembacaan kartu RFID CLRC66303 (SPI3)
		// App_RFIDProcess();

		// 2. Proses pembacaan sentuhan layar XPT2046 (SPI2)
		// App_TouchProcess();

		// 3. Heartbeat LED (berkedip setiap 1 detik saat idle dan tidak ada kartu/sentuhan aktif)
		if (!g_card_active && !XPT2046_IsTouched()) {
			if (HAL_GetTick() - last_heartbeat >= 1000) {
				last_heartbeat = HAL_GetTick();
				HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
			}
		}
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */
		ui_tick();
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 16;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_1TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SDIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 32;
  /* USER CODE BEGIN SDIO_Init 2 */

  /* USER CODE END SDIO_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM14 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM14_Init(void)
{

  /* USER CODE BEGIN TIM14_Init 0 */

  /* USER CODE END TIM14_Init 0 */

  /* USER CODE BEGIN TIM14_Init 1 */

  /* USER CODE END TIM14_Init 1 */
  htim14.Instance = TIM14;
  htim14.Init.Prescaler = 0;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim14.Init.Period = 65535;
  htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM14_Init 2 */

  /* USER CODE END TIM14_Init 2 */

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
  /* DMA2_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
  /* DMA2_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, SPI1_RST_Pin|SPI1_DC_Pin|USB_POWER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, USER_LED_Pin|SPI2_CS_Pin|SPI3_CS_Pin|SPI3_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI1_RST_Pin SPI1_DC_Pin USB_POWER_Pin */
  GPIO_InitStruct.Pin = SPI1_RST_Pin|SPI1_DC_Pin|USB_POWER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : USER_LED_Pin SPI2_CS_Pin SPI3_CS_Pin SPI3_RST_Pin */
  GPIO_InitStruct.Pin = USER_LED_Pin|SPI2_CS_Pin|SPI3_CS_Pin|SPI3_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_IRQ_Pin */
  GPIO_InitStruct.Pin = SPI2_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SPI2_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SDIO_DET_Pin */
  GPIO_InitStruct.Pin = SDIO_DET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SDIO_DET_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI3_IRQ_Pin */
  GPIO_InitStruct.Pin = SPI3_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SPI3_IRQ_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
