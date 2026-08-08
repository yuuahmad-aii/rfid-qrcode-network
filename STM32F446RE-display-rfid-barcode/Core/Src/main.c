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
#include "rc522.h"
#include "usbh_hid.h"
#include "usbh_hid_keybd.h"
#include "xpt2046.h"
#include "lvgl.h"
#include "eez/ui.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

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

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */
typedef enum {
  STATE_BOOTING,
  STATE_STANDBY,
  STATE_READING,
  STATE_GRANTED,
  STATE_DENIED,
  STATE_MENU_PASS,
  STATE_MENU_ADMIN,
  STATE_MENU_ADD,
  STATE_SYNC_TIME,
  STATE_SYNC_RESULT,
  STATE_BARCODE_READ
} AppState;

AppState currentState = STATE_BOOTING;
uint32_t stateEnterTime = 0;
uint8_t rfid_id[5];
char input_password[10] = "";
const char *correct_password = "1234";
bool card_tapped = false;

// Dummy credentials for testing
const uint8_t known_uid[4] = {0x12, 0x34, 0x56, 0x78};

extern USBH_HandleTypeDef hUsbHostFS;
char barcode_buffer[64];
char barcode_display_buffer[64];
uint8_t barcode_idx = 0;
volatile bool barcode_ready = false;
volatile bool time_sync_success = false;
volatile bool time_sync_error = false;
char time_error_msg[32] = {0};

volatile bool api_status_granted = false;
volatile bool api_status_denied = false;
char api_result_msg[64] = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM3_Init(void);
static void MX_SPI2_Init(void);
static void MX_SPI3_Init(void);
static void MX_ADC1_Init(void);
static void MX_RTC_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM14_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
void UI_DrawHeader(void);
void UI_DrawFooter(void);
void UI_DrawStandby(void);
void UI_DrawKeypad(void);
void UI_UpdateClock(void);
void UI_DrawMenuAdmin(void);

// LVGL Callbacks
static void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
static void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t rx_uart_byte;
char rx_uart_buf[128];
uint8_t rx_uart_idx = 0;
uint32_t last_time_update = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    if (rx_uart_byte == '\n' || rx_uart_byte == '\r') {
      rx_uart_buf[rx_uart_idx] = '\0';
      if (rx_uart_idx > 0) {
        if (strncmp(rx_uart_buf, "TIME:", 5) == 0) {
          int year, month, day, hour, min, sec;
          if (sscanf(rx_uart_buf + 5, "%d-%d-%d %d:%d:%d", &year, &month, &day,
                     &hour, &min, &sec) == 6) {
            RTC_TimeTypeDef sTime = {0};
            RTC_DateTypeDef sDate = {0};

            sTime.Hours = hour;
            sTime.Minutes = min;
            sTime.Seconds = sec;
            sTime.TimeFormat = RTC_HOURFORMAT_24;
            sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
            sTime.StoreOperation = RTC_STOREOPERATION_RESET;
            HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

            sDate.Year = year - 2000;
            sDate.Month = month;
            sDate.Date = day;
            sDate.WeekDay = RTC_WEEKDAY_MONDAY;
            HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            time_sync_success = true;
          }
        } else if (strncmp(rx_uart_buf, "TIME_ERR:", 9) == 0) {
          strncpy(time_error_msg, rx_uart_buf + 9, sizeof(time_error_msg) - 1);
          time_error_msg[sizeof(time_error_msg) - 1] = '\0';
          time_sync_error = true;
        } else if (strncmp(rx_uart_buf, "GRANTED:", 8) == 0) {
          strncpy(api_result_msg, rx_uart_buf + 8, sizeof(api_result_msg) - 1);
          api_result_msg[sizeof(api_result_msg) - 1] = '\0';
          api_status_granted = true;
        } else if (strncmp(rx_uart_buf, "DENIED:", 7) == 0) {
          strncpy(api_result_msg, rx_uart_buf + 7, sizeof(api_result_msg) - 1);
          api_result_msg[sizeof(api_result_msg) - 1] = '\0';
          api_status_denied = true;
        }
      }
      rx_uart_idx = 0;
    } else {
      if (rx_uart_idx < sizeof(rx_uart_buf) - 1) {
        rx_uart_buf[rx_uart_idx++] = rx_uart_byte;
      }
    }
    HAL_UART_Receive_IT(&huart1, &rx_uart_byte, 1);
  }
}

void UI_DrawHeader(void) {
  ILI9488_FillRectangle(0, 0, 480, 40, ILI9488_NAVY);
  ILI9488_WriteStringScaled(10, 10, "ACCESS SYSTEM", Font_7x10, ILI9488_WHITE,
                            ILI9488_NAVY, 2);
  ILI9488_WriteStringScaled(380, 10, "WIFI: ON", Font_7x10, ILI9488_GREEN,
                            ILI9488_NAVY, 2);
}

void UI_DrawFooter(void) {
  ILI9488_FillRectangle(0, 320 - 40, 480, 40, ILI9488_DARKGREY);

  // Call update clock to draw the dynamic text
  UI_UpdateClock();

  // Admin button
  ILI9488_FillRectangle(380, 320 - 35, 90, 30, ILI9488_BLUE);
  ILI9488_WriteStringScaled(395, 320 - 28, "ADMIN", Font_7x10, ILI9488_WHITE,
                            ILI9488_BLUE, 2);
}

void UI_UpdateClock(void) {
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

  char time_str[64];
  snprintf(time_str, sizeof(time_str), "DEV-001 | %02d:%02d:%02d", sTime.Hours,
           sTime.Minutes, sTime.Seconds);

  // Clear specifically the text area
  ILI9488_FillRectangle(10, 320 - 30, 250, 20, ILI9488_DARKGREY);
  ILI9488_WriteStringScaled(10, 320 - 30, time_str, Font_7x10, ILI9488_WHITE,
                            ILI9488_DARKGREY, 2);
}

void UI_DrawStandby(void) {
  ILI9488_FillScreen(ILI9488_BLACK);
  UI_DrawHeader();
  UI_DrawFooter();
  ILI9488_WriteStringScaled(40, 140, "SILAKAN SCAN KARTU", Font_7x10,
                            ILI9488_WHITE, ILI9488_BLACK, 3);
  ILI9488_WriteStringScaled(80, 180, "ATAU QR CODE", Font_7x10,
                            ILI9488_LIGHTGREY, ILI9488_BLACK, 3);
}

void UI_DrawKeypad(void) {
  ILI9488_FillScreen(ILI9488_BLACK);
  UI_DrawHeader();
  ILI9488_WriteStringScaled(135, 45, "MASUKKAN PIN ADMIN:", Font_7x10,
                            ILI9488_WHITE, ILI9488_BLACK, 2);
  ILI9488_FillRectangle(135, 65, 200, 30, ILI9488_DARKGREY);

  int start_x = 135;
  int start_y = 105;
  int btn_w = 60;
  int btn_h = 40;
  int spc = 10;

  char labels[12][4] = {"1", "2", "3", "4",   "5", "6",
                        "7", "8", "9", "DEL", "0", "OK"};
  for (int i = 0; i < 12; i++) {
    int row = i / 3;
    int col = i % 3;
    int x = start_x + col * (btn_w + spc);
    int y = start_y + row * (btn_h + spc);

    uint16_t color = ILI9488_BLUE;
    if (i == 9)
      color = ILI9488_RED;
    else if (i == 11)
      color = ILI9488_GREEN;

    ILI9488_FillRectangle(x, y, btn_w, btn_h, color);
    if (i == 9 || i == 11) {
      ILI9488_WriteStringScaled(x + 5, y + 10, labels[i], Font_7x10,
                                ILI9488_WHITE, color, 2);
    } else {
      ILI9488_WriteStringScaled(x + 20, y + 10, labels[i], Font_7x10,
                                ILI9488_WHITE, color, 2);
    }
  }
}

void UI_DrawMenuAdmin(void) {
  ILI9488_FillScreen(ILI9488_BLACK);
  UI_DrawHeader();
  ILI9488_WriteStringScaled(160, 60, "MENU ADMIN", Font_7x10, ILI9488_YELLOW,
                            ILI9488_BLACK, 3);

  // Btn Tambah Kartu
  ILI9488_FillRectangle(80, 120, 320, 50, ILI9488_BLUE);
  ILI9488_WriteStringScaled(120, 135, "TAMBAH KARTU", Font_7x10, ILI9488_WHITE,
                            ILI9488_BLUE, 3);

  // Btn Sync Waktu
  ILI9488_FillRectangle(80, 190, 320, 50, ILI9488_ORANGE);
  ILI9488_WriteStringScaled(140, 205, "SYNC WAKTU", Font_7x10, ILI9488_WHITE,
                            ILI9488_ORANGE, 3);

  // Btn Kembali
  ILI9488_FillRectangle(190, 260, 100, 40, ILI9488_RED);
  ILI9488_WriteStringScaled(210, 270, "KEMBALI", Font_7x10, ILI9488_WHITE,
                            ILI9488_RED, 2);
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
  MX_SPI1_Init();
  MX_TIM3_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
  MX_SDIO_SD_Init();
  MX_USART1_UART_Init();
  MX_FATFS_Init();
  MX_USB_HOST_Init();
  MX_TIM14_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, &rx_uart_byte, 1);
  // Start the backlight PWM on TIM3 CH3
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 32767); // 50% brightness

  // Initialize the Display and Touch
  ILI9488_Init();
  XPT2046_Init();
  RC522_Init(&hspi3);

  // Booting Screen (Manual UI commented out for LVGL)
  /*
  ILI9488_FillScreen(ILI9488_BLACK);
  ILI9488_WriteStringScaled(40, 140, "MEMULAI PERANGKAT...", Font_7x10,
                            ILI9488_WHITE, ILI9488_BLACK, 3);
  HAL_Delay(1000);
  ILI9488_WriteStringScaled(40, 180, "DISPLAY SIAP", Font_7x10, ILI9488_GREEN,
                            ILI9488_BLACK, 2);
  HAL_Delay(500);
  ILI9488_WriteStringScaled(40, 200, "RFID SIAP", Font_7x10, ILI9488_GREEN,
                            ILI9488_BLACK, 2);
  HAL_Delay(1000);
  */
  
  currentState = STATE_STANDBY;
  // UI_DrawStandby(); // Commented out for LVGL


  // --- LVGL Setup ---
  lv_init();

  // 1. Display Setup
  lv_display_t * disp = lv_display_create(ILI9488_WIDTH, ILI9488_HEIGHT);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);
  lv_display_set_flush_cb(disp, my_disp_flush);
  
  // Create a draw buffer for LVGL (e.g., 1/10 screen size)
  #define DRAW_BUF_SIZE (ILI9488_WIDTH * ILI9488_HEIGHT / 10 * 3)
  static uint8_t buf1[DRAW_BUF_SIZE] __attribute__((aligned(64)));
  lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

  // 2. Input Device Setup (Touch)
  lv_indev_t * indev = lv_indev_create();
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
  while (1) {
    uint32_t current_tick = HAL_GetTick();
    if (current_tick > last_tick) {
        lv_tick_inc(current_tick - last_tick);
    }
    last_tick = current_tick;
    lv_timer_handler();

    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */
    ui_tick();
    
    // --- Background Scanner Checks ---
    if (barcode_ready) {
      barcode_ready = false;
      static char barcode_uart_buf[90];
      snprintf(barcode_uart_buf, sizeof(barcode_uart_buf), "BARCODE:%s\n", barcode_display_buffer);
      HAL_UART_Transmit_DMA(&huart1, (uint8_t *)barcode_uart_buf, strlen(barcode_uart_buf));
    }

    static uint32_t last_rfid_scan = 0;
    if (HAL_GetTick() - last_rfid_scan > 1000) {
      if (RC522_Check(rfid_id) == MI_OK) {
        last_rfid_scan = HAL_GetTick();
        static char rfid_uart_buf[32];
        snprintf(rfid_uart_buf, sizeof(rfid_uart_buf),
                 "RFID:%02X%02X%02X%02X\n", rfid_id[0], rfid_id[1], rfid_id[2], rfid_id[3]);
        HAL_UART_Transmit_DMA(&huart1, (uint8_t *)rfid_uart_buf, strlen(rfid_uart_buf));
      }
    }

    // --- Process API Responses ---
    if (api_status_granted) {
      api_status_granted = false;
      HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_SET);
      HAL_Delay(200);
      HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);
    }
    
    if (api_status_denied) {
      api_status_denied = false;
      for (int i = 0; i < 2; i++) {
        HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);
        HAL_Delay(100);
      }
    }
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
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLRCLK;
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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
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
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
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
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
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
  /* DMA2_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
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

  /*Configure GPIO pin : USER_BTN_Pin */
  GPIO_InitStruct.Pin = USER_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(USER_BTN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI1_RST_Pin SPI1_DC_Pin */
  GPIO_InitStruct.Pin = SPI1_RST_Pin|SPI1_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : USER_LED_Pin */
  GPIO_InitStruct.Pin = USER_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USER_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI2_CS_Pin SPI3_CS_Pin SPI3_RST_Pin */
  GPIO_InitStruct.Pin = SPI2_CS_Pin|SPI3_CS_Pin|SPI3_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_IRQ_Pin */
  GPIO_InitStruct.Pin = SPI2_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SPI2_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_POWER_Pin */
  GPIO_InitStruct.Pin = USB_POWER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_POWER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SDIO_DET_Pin */
  GPIO_InitStruct.Pin = SDIO_DET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SDIO_DET_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void USBH_HID_EventCallback(USBH_HandleTypeDef *phost) {
  if (USBH_HID_GetDeviceType(phost) == HID_KEYBOARD) {
    HID_KEYBD_Info_TypeDef *k_pinfo = USBH_HID_GetKeybdInfo(phost);
    if (k_pinfo != NULL) {
      char c = USBH_HID_GetASCIICode(k_pinfo);
      if (c != 0) {
        if (c == '\n' || c == '\r') {
          if (barcode_idx > 0) {
            barcode_buffer[barcode_idx] = '\0';
            strncpy(barcode_display_buffer, barcode_buffer,
                    sizeof(barcode_display_buffer) - 1);
            barcode_display_buffer[sizeof(barcode_display_buffer) - 1] = '\0';
            barcode_ready = true;
            barcode_idx = 0;
          }
        } else {
          if (barcode_idx < sizeof(barcode_buffer) - 1) {
            barcode_buffer[barcode_idx++] = c;
          }
        }
      }
    }
  }
}

/* LVGL Display Flush Callback */
static void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    ILI9488_DrawBitmapLVGL(area->x1, area->y1, area->x2, area->y2, px_map);
    lv_display_flush_ready(disp);
}

/* LVGL Touchpad Read Callback */
static void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    uint16_t touch_x = 0, touch_y = 0;
    if(XPT2046_GetTouch(&touch_x, &touch_y)) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touch_x;
        data->point.y = touch_y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* Timer callback for LVGL */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14) {
        // Unused now, tick is handled in main loop
    }
}
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
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
