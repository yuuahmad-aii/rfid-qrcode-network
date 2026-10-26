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
#include "cmsis_os.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "bsp_driver_sd.h"
#include "clrc663.h"
#include "fonts.h"
#include "gui_attendance.h"
#include "ili9488.h"
#include "lvgl.h"
#include "task.h"
#include "xpt2046.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct {
  uint32_t timestamp; /* Unix or custom timestamp (4 bytes) */
  uint8_t year;       /* Year - 2000 (1 byte) */
  uint8_t month;      /* Month 1-12 (1 byte) */
  uint8_t date;       /* Day of month 1-31 (1 byte) */
  uint8_t hours;      /* Hours 0-23 (1 byte) */
  uint8_t minutes;    /* Minutes 0-59 (1 byte) */
  uint8_t seconds;    /* Seconds 0-59 (1 byte) */
  uint8_t uid_len;    /* Length of UID (1 byte) */
  uint8_t uid[10];    /* Card UID (10 bytes) */
  char log_msg[44];   /* Text log details or remark (44 bytes) */
} sd_log_item_t;      /* Exactly 64 bytes (matches sdLogQueue item size) */

#ifndef ESP_TX_ITEM_DEFINED
#define ESP_TX_ITEM_DEFINED
typedef struct {
  char payload[128]; /* Formatted string packet to ESP32 (128 bytes) */
} esp_tx_item_t;     /* Exactly 128 bytes (matches espTxQueue item size) */
#endif
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Color Palette for Modern Glassmorphism UI */
#define COLOR_BG 0x0821            /* Dark Charcoal / Deep Navy Background */
#define COLOR_HEADER_BG 0x10A2     /* Slate Header Background */
#define COLOR_PANEL_BG 0x1082      /* Dark Slate Card Container */
#define COLOR_BORDER_CYAN 0x05BF   /* Glowing Neon Cyan Border */
#define COLOR_POPUP_BG 0x0862      /* Deep Indigo Pop-up Modal */
#define COLOR_ACCENT_GREEN 0x07E0  /* Emerald Green Status */
#define COLOR_ACCENT_RED 0xF800    /* Vivid Coral Red Error */
#define COLOR_ACCENT_YELLOW 0xFFE0 /* Warm Amber Clock / Highlights */
#define COLOR_TEXT_WHITE 0xFFFF    /* Crisp White */
#define COLOR_TEXT_MUTED 0x8410    /* Medium Muted Grey */
#define COLOR_BTN_BG 0x1904        /* Slate Button Fill */
#define COLOR_BTN_BORDER 0x07FF    /* Cyan Button Outline */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

IWDG_HandleTypeDef hiwdg;

RTC_HandleTypeDef hrtc;

SD_HandleTypeDef hsd;
DMA_HandleTypeDef hdma_sdio_rx;
DMA_HandleTypeDef hdma_sdio_tx;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* Definitions for Task_RFID */
osThreadId_t Task_RFIDHandle;
const osThreadAttr_t Task_RFID_attributes = {
    .name = "Task_RFID",
    .stack_size = 512 * 4,
    .priority = (osPriority_t)osPriorityHigh,
};
/* Definitions for Task_GUI */
osThreadId_t Task_GUIHandle;
const osThreadAttr_t Task_GUI_attributes = {
    .name = "Task_GUI",
    .stack_size = 2560 * 4,
    .priority = (osPriority_t)osPriorityAboveNormal,
};
/* Definitions for Task_ESP32 */
osThreadId_t Task_ESP32Handle;
const osThreadAttr_t Task_ESP32_attributes = {
    .name = "Task_ESP32",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for Task_Storage */
osThreadId_t Task_StorageHandle;
const osThreadAttr_t Task_Storage_attributes = {
    .name = "Task_Storage",
    .stack_size = 1536 * 4,
    .priority = (osPriority_t)osPriorityBelowNormal,
};
/* Definitions for uiEventQueue */
osMessageQueueId_t uiEventQueueHandle;
const osMessageQueueAttr_t uiEventQueue_attributes = {.name = "uiEventQueue"};
/* Definitions for sdLogQueue */
osMessageQueueId_t sdLogQueueHandle;
const osMessageQueueAttr_t sdLogQueue_attributes = {.name = "sdLogQueue"};
/* Definitions for espTxQueue */
osMessageQueueId_t espTxQueueHandle;
const osMessageQueueAttr_t espTxQueue_attributes = {.name = "espTxQueue"};
/* Definitions for sdioFatFsMutex */
osMutexId_t sdioFatFsMutexHandle;
const osMutexAttr_t sdioFatFsMutex_attributes = {.name = "sdioFatFsMutex"};
/* Definitions for spi1TxDmaSem */
osSemaphoreId_t spi1TxDmaSemHandle;
const osSemaphoreAttr_t spi1TxDmaSem_attributes = {.name = "spi1TxDmaSem"};
/* USER CODE BEGIN PV */
/* Mutex for protecting SPI2 bus between Touch & other SPI tasks */
osMutexId_t spi2MutexHandle;
const osMutexAttr_t spi2Mutex_attributes = {.name = "spi2Mutex"};

/* Hardware & System Status Flags */
volatile bool g_clrc663_ready = false;
volatile uint8_t g_clrc663_version = 0;
volatile bool g_sd_mounted = false;
volatile bool g_wifi_connected = false;
volatile uint32_t g_attendance_total = 0;
volatile bool g_bulk_sync_requested = false;

wifi_info_t g_wifi_info = {
    .status = "DISCONNECTED",
    .ssid = "-",
    .ip = "0.0.0.0",
    .rssi = "- dBm",
    .protocol = "Wi-Fi 6 (802.11ax)",
    .mac = "-",
    .is_wifi6 = false
};

/* RFID duplicate filter */
static uint8_t g_prev_uid[10] = {0};
static uint8_t g_prev_uid_len = 0;
static uint32_t g_prev_uid_tick = 0;

/* ESP32 USART1 Rx Buffer */
uint8_t uart1_rx_data;
char uart1_rx_buf[128];
volatile uint16_t uart1_rx_idx = 0;
volatile uint8_t uart1_rx_ready = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_RTC_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_SPI3_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_IWDG_Init(void);
void StartRfidTask(void *argument);
void StartGuiTask(void *argument);
void StartEsp32Task(void *argument);
void StartStorageTask(void *argument);

/* USER CODE BEGIN PFP */
static void my_disp_flush(lv_display_t *disp, const lv_area_t *area,
                          uint8_t *px_map);
static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* ========================================================================== */
/*                COOPERATIVE MULTI-TASK WATCHDOG (IWDG) SUPERVISOR           */
/* ========================================================================== */
#define WDT_TASK_RFID (1U << 0)
#define WDT_TASK_GUI (1U << 1)
#define WDT_TASK_ESP32 (1U << 2)
#define WDT_TASK_STORAGE (1U << 3)
#define WDT_ALL_TASKS                                                          \
  (WDT_TASK_RFID | WDT_TASK_GUI | WDT_TASK_ESP32 | WDT_TASK_STORAGE)

static volatile uint32_t g_wdt_task_checkin = 0;

/**
 * @brief  Pencatatan check-in berkala dari masing-masing task FreeRTOS.
 *         Watchdog IWDG HANYA akan disegarkan jika SELURUH task aktif
 *         (RFID, GUI, ESP32, Storage) beroperasi normal.
 *         Jika salah satu task hang/deadlock, IWDG akan reset hardware dalam 2
 * detik.
 */
void Watchdog_CheckIn(uint32_t task_mask) {
  taskENTER_CRITICAL();
  g_wdt_task_checkin |= task_mask;
  if ((g_wdt_task_checkin & WDT_ALL_TASKS) == WDT_ALL_TASKS) {
    HAL_IWDG_Refresh(&hiwdg);
    g_wdt_task_checkin = 0;
  }
  taskEXIT_CRITICAL();
}

void Watchdog_KickDirect(void) {
  taskENTER_CRITICAL();
  HAL_IWDG_Refresh(&hiwdg);
  g_wdt_task_checkin = 0;
  taskEXIT_CRITICAL();
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* Zero out CCMRAM section (0x10000000) agar semua variabel dan pool LVGL terinisialisasi bersih */
  extern uint8_t _sccmram, _eccmram;
  memset(&_sccmram, 0, (size_t)(&_eccmram - &_sccmram));
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_RTC_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  HAL_IWDG_Refresh(&hiwdg);
  ILI9488_Init();
  HAL_IWDG_Refresh(&hiwdg);
  XPT2046_Init();
  HAL_IWDG_Refresh(&hiwdg);

  /* Cek apakah perangkat baru saja pulih mandiri dari Watchdog Reset */
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
    __HAL_RCC_CLEAR_RESET_FLAGS();
  }
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of sdioFatFsMutex */
  sdioFatFsMutexHandle = osMutexNew(&sdioFatFsMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* creation of spi2Mutex */
  spi2MutexHandle = osMutexNew(&spi2Mutex_attributes);
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of spi1TxDmaSem */
  spi1TxDmaSemHandle = osSemaphoreNew(1, 1, &spi1TxDmaSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of uiEventQueue */
  uiEventQueueHandle =
      osMessageQueueNew(8, sizeof(ui_event_t), &uiEventQueue_attributes);

  /* creation of sdLogQueue */
  sdLogQueueHandle = osMessageQueueNew(16, 64, &sdLogQueue_attributes);

  /* creation of espTxQueue */
  espTxQueueHandle =
      osMessageQueueNew(32, sizeof(esp_tx_item_t), &espTxQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Task_RFID */
  Task_RFIDHandle = osThreadNew(StartRfidTask, NULL, &Task_RFID_attributes);

  /* creation of Task_GUI */
  Task_GUIHandle = osThreadNew(StartGuiTask, NULL, &Task_GUI_attributes);

  /* creation of Task_ESP32 */
  Task_ESP32Handle = osThreadNew(StartEsp32Task, NULL, &Task_ESP32_attributes);

  /* creation of Task_Storage */
  Task_StorageHandle =
      osThreadNew(StartStorageTask, NULL, &Task_Storage_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  if (Task_RFIDHandle == NULL || Task_GUIHandle == NULL ||
      Task_ESP32Handle == NULL || Task_StorageHandle == NULL) {
    /* Alokasi task FreeRTOS gagal akibat memori heap habis */
    while (1) {
      HAL_GPIO_TogglePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin);
      HAL_Delay(80);
      HAL_IWDG_Refresh(&hiwdg);
    }
  }
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Segarkan IWDG tepat sebelum RTOS scheduler dimulai */
  HAL_IWDG_Refresh(&hiwdg);

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data
   * Alignment and number of conversion)
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
  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in
   * the sequencer and its sample time.
   */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */
}

/**
 * @brief IWDG Initialization Function
 * @param None
 * @retval None
 */
static void MX_IWDG_Init(void) {

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
  hiwdg.Init.Reload =
      3000; /* 32 kHz / 64 = 500 Hz => 3000 reload = 6.0 detik timeout */
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */
}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void) {

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
  if (HAL_RTC_Init(&hrtc) != HAL_OK) {
    Error_Handler();
  }

  /* Konfigurasi RTC Wakeup Timer (LSE 32.768 kHz / 16 = 2048 Hz).
     Reload 2047 = periode 1.0 detik untuk membangunkan MCU secara aman dari
     STOP mode */
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
  HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
  HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2047, RTC_WAKEUPCLOCK_RTCCLK_DIV16);

  /* USER CODE BEGIN Check_RTC_BKUP */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == 0x32F2) {
    /* RTC sudah pernah disinkronkan dan terus berjalan via baterai backup */
    return;
  }
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
   */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK) {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
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
static void MX_SDIO_SD_Init(void) {

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE;
  hsd.Init.ClockDiv = 4;
  /* USER CODE BEGIN SDIO_Init 2 */

  /* USER CODE END SDIO_Init 2 */
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void) {

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
  if (HAL_SPI_Init(&hspi1) != HAL_OK) {
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
static void MX_SPI2_Init(void) {

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
  if (HAL_SPI_Init(&hspi2) != HAL_OK) {
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
static void MX_SPI3_Init(void) {

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
  if (HAL_SPI_Init(&hspi3) != HAL_OK) {
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
static void MX_TIM3_Init(void) {

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
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

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
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
  /* DMA2_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
  /* DMA2_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
  /* DMA2_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
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
  HAL_GPIO_WritePin(GPIOA, OUTPUT_BUZZER_Pin | SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, SPI1_RST_Pin | SPI1_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB,
                    USER_LED_Pin | SPI2_CS_Pin | SPI3_CS_Pin | SPI3_RST_Pin,
                    GPIO_PIN_RESET);

  /*Configure GPIO pins : OUTPUT_BUZZER_Pin SPI1_CS_Pin */
  GPIO_InitStruct.Pin = OUTPUT_BUZZER_Pin | SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI1_RST_Pin SPI1_DC_Pin */
  GPIO_InitStruct.Pin = SPI1_RST_Pin | SPI1_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : USER_LED_Pin SPI2_CS_Pin SPI3_CS_Pin SPI3_RST_Pin */
  GPIO_InitStruct.Pin = USER_LED_Pin | SPI2_CS_Pin | SPI3_CS_Pin | SPI3_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_IRQ_Pin */
  GPIO_InitStruct.Pin = SPI2_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SPI2_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SDIO_DET_Pin */
  GPIO_InitStruct.Pin = SDIO_DET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SDIO_DET_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI3_IRQ_Pin */
  GPIO_InitStruct.Pin = SPI3_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(SPI3_IRQ_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 8, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 8, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief  EXTI Callback for RFID and Touch interrupts
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  /* RFID IRQ: PA15 (SPI3_IRQ) */
  if (GPIO_Pin == SPI3_IRQ_Pin) {
    if (Task_RFIDHandle != NULL) {
      vTaskNotifyGiveFromISR((TaskHandle_t)Task_RFIDHandle,
                             &xHigherPriorityTaskWoken);
    }
    if (Task_GUIHandle != NULL) {
      vTaskNotifyGiveFromISR((TaskHandle_t)Task_GUIHandle,
                             &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
  /* Touch IRQ: PC6 (SPI2_IRQ) atau PB1 */
  else if (GPIO_Pin == SPI2_IRQ_Pin || GPIO_Pin == GPIO_PIN_1) {
    if (Task_GUIHandle != NULL) {
      vTaskNotifyGiveFromISR((TaskHandle_t)Task_GUIHandle,
                             &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  }
}

/**
 * @brief  UART Rx Complete Callback for USART1 ESP32 communication
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    if (uart1_rx_data == '\n' || uart1_rx_data == '\r') {
      uart1_rx_buf[uart1_rx_idx] = '\0';
      if (uart1_rx_idx > 0) {
        uart1_rx_ready = 1;
      }
      uart1_rx_idx = 0;
    } else {
      if (uart1_rx_idx < sizeof(uart1_rx_buf) - 1) {
        uart1_rx_buf[uart1_rx_idx++] = uart1_rx_data;
      }
    }
    HAL_UART_Receive_IT(&huart1, &uart1_rx_data, 1);
  }
}

/* ========================================================================== */
/*                       LVGL DISPLAY & TOUCH DRIVER CALLBACKS                */
/* ========================================================================== */

/**
 * @brief  LVGL Display flush callback sending pixels to ILI9488
 */
static void my_disp_flush(lv_display_t *disp, const lv_area_t *area,
                          uint8_t *px_map) {
  ILI9488_DrawBitmapLVGL(area->x1, area->y1, area->x2, area->y2, px_map);
  lv_display_flush_ready(disp);
}

/**
 * @brief  Membaca riwayat absensi terakhir dari MicroSD (ATTEND.CSV)
 */
uint16_t Storage_ReadRecentLogs(char logs[][64], uint16_t max_logs) {
  uint16_t count = 0;
  if (!g_sd_mounted)
    return 0;
  if (osMutexAcquire(sdioFatFsMutexHandle, 500) != osOK)
    return 0;

  if (f_open(&SDFile, "ATTEND.CSV", FA_READ) == FR_OK) {
    FSIZE_t size = f_size(&SDFile);
    /* Baca dari dekat akhir file jika ukuran file lebih dari 2KB */
    FSIZE_t start_pos = (size > 2048) ? (size - 2048) : 0;
    f_lseek(&SDFile, start_pos);

    char line_buf[128];
    while (f_gets(line_buf, sizeof(line_buf), &SDFile) != NULL) {
      size_t len = strlen(line_buf);
      while (len > 0 &&
             (line_buf[len - 1] == '\r' || line_buf[len - 1] == '\n')) {
        line_buf[--len] = '\0';
      }
      if (len > 5) {
        if (count < max_logs) {
          strncpy(logs[count], line_buf, 63);
          logs[count][63] = '\0';
          count++;
        } else {
          for (int i = 0; i < max_logs - 1; i++) {
            strncpy(logs[i], logs[i + 1], 64);
          }
          strncpy(logs[max_logs - 1], line_buf, 63);
          logs[max_logs - 1][63] = '\0';
        }
      }
    }
    f_close(&SDFile);
  }

  osMutexRelease(sdioFatFsMutexHandle);
  return count;
}

/**
 * @brief  Pengujian diagnostik MicroSD (Mount, Buat file, Tulis, Baca, Hapus)
 */
bool Storage_TestDiagnostic(char *out_msg, size_t max_len) {
  if (!g_sd_mounted) {
    if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 1) == FR_OK) {
      g_sd_mounted = true;
    } else {
      snprintf(out_msg, max_len,
               "GAGAL: MicroSD tidak terdeteksi (f_mount gagal). Cek kartu & "
               "slot SDIO.");
      return false;
    }
  }

  if (osMutexAcquire(sdioFatFsMutexHandle, 1000) != osOK) {
    snprintf(out_msg, max_len, "GAGAL: Antrean akses FatFS sibuk.");
    return false;
  }

  FIL testFile;
  FRESULT fres = f_open(&testFile, "TEST_RW.TMP", FA_CREATE_ALWAYS | FA_WRITE);
  if (fres != FR_OK) {
    osMutexRelease(sdioFatFsMutexHandle);
    snprintf(
        out_msg, max_len,
        "GAGAL: Tidak dapat membuat file tulis (FR=%d). Pastikan format FAT32.",
        (int)fres);
    return false;
  }

  const char test_data[] = "ABSENSI_DIAG_OK_12345";
  UINT bw = 0;
  f_write(&testFile, test_data, sizeof(test_data), &bw);
  f_close(&testFile);

  char read_buf[32] = {0};
  UINT br = 0;
  fres = f_open(&testFile, "TEST_RW.TMP", FA_READ);
  if (fres == FR_OK) {
    f_read(&testFile, read_buf, sizeof(read_buf) - 1, &br);
    f_close(&testFile);
    f_unlink("TEST_RW.TMP");
  }

  osMutexRelease(sdioFatFsMutexHandle);

  if (strcmp(read_buf, test_data) == 0) {
    snprintf(out_msg, max_len,
             "OK: MicroSD FAT32 normal, uji tulis & baca data berhasil.");
    return true;
  } else {
    snprintf(out_msg, max_len,
             "GAGAL: Data uji baca tidak cocok dengan data tulis.");
    return false;
  }
}

/* Struktur RAM Cache Cepat untuk lookup kartu lokal (sub-milidetik) */
typedef struct {
  char uid[24];
  char name[48];
  char pos[32];
  bool valid;
} emp_cache_t;

#define EMP_CACHE_SIZE 32
static emp_cache_t s_emp_cache[EMP_CACHE_SIZE] = {0};
static uint8_t s_emp_cache_idx = 0;

/**
 * @brief  Memuat data karyawan dari USERS.CSV ke RAM Cache saat booting
 * (warm-up)
 */
void Storage_WarmupCache(void) {
  if (osMutexAcquire(sdioFatFsMutexHandle, 1000) != osOK)
    return;

  if (!g_sd_mounted) {
    if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 0) == FR_OK) {
      g_sd_mounted = true;
    } else {
      osMutexRelease(sdioFatFsMutexHandle);
      return;
    }
  }

  FIL file;
  if (f_open(&file, "USERS.CSV", FA_READ) == FR_OK) {
    char line[128];
    while (f_gets(line, sizeof(line), &file) != NULL) {
      size_t l = strlen(line);
      while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == '\n' ||
                       line[l - 1] == ' ')) {
        line[--l] = '\0';
      }
      if (l < 3)
        continue;
      if (strncasecmp(line, "UID,", 4) == 0)
        continue;

      char *token_uid = strtok(line, ",");
      char *token_name = strtok(NULL, ",");
      char *token_pos = strtok(NULL, ",");

      if (token_uid && token_name) {
        while (*token_uid == ' ')
          token_uid++;
        while (*token_name == ' ')
          token_name++;
        if (token_pos) {
          while (*token_pos == ' ')
            token_pos++;
        }

        /* Filter entri dummy / placeholder Akses Diterima agar tidak masuk cache */
        if (strcasecmp(token_name, "Akses Diterima") == 0 ||
            strcasecmp(token_name, "Akses Diterima!") == 0 ||
            strcasecmp(token_name, "UNKNOWN") == 0 ||
            strcasecmp(token_name, "-") == 0 ||
            strlen(token_name) < 2) {
          continue;
        }

        strncpy(s_emp_cache[s_emp_cache_idx].uid, token_uid,
                sizeof(s_emp_cache[s_emp_cache_idx].uid) - 1);
        s_emp_cache[s_emp_cache_idx]
            .uid[sizeof(s_emp_cache[s_emp_cache_idx].uid) - 1] = '\0';
        strncpy(s_emp_cache[s_emp_cache_idx].name, token_name,
                sizeof(s_emp_cache[s_emp_cache_idx].name) - 1);
        s_emp_cache[s_emp_cache_idx]
            .name[sizeof(s_emp_cache[s_emp_cache_idx].name) - 1] = '\0';
        if (token_pos) {
          strncpy(s_emp_cache[s_emp_cache_idx].pos, token_pos,
                  sizeof(s_emp_cache[s_emp_cache_idx].pos) - 1);
          s_emp_cache[s_emp_cache_idx]
              .pos[sizeof(s_emp_cache[s_emp_cache_idx].pos) - 1] = '\0';
        } else {
          s_emp_cache[s_emp_cache_idx].pos[0] = '\0';
        }
        s_emp_cache[s_emp_cache_idx].valid = true;
        s_emp_cache_idx = (s_emp_cache_idx + 1) % EMP_CACHE_SIZE;
      }
    }
    f_close(&file);
  }

  osMutexRelease(sdioFatFsMutexHandle);
}

/**
 * @brief  Mencari nama dan jabatan karyawan di RAM Cache atau MicroSD
 * (USERS.CSV) Format CSV: UID,NAMA,JABATAN
 */
bool Storage_LookupEmployee(const char *uid_str, char *out_name,
                            size_t name_len, char *out_pos, size_t pos_len) {
  if (uid_str == NULL || strlen(uid_str) == 0)
    return false;
  if (out_name == NULL || name_len == 0)
    return false;
  out_name[0] = '\0';
  if (out_pos && pos_len > 0)
    out_pos[0] = '\0';

  /* 1. Fast Path: Periksa RAM Cache lokal terlebih dahulu */
  for (int i = 0; i < EMP_CACHE_SIZE; i++) {
    if (s_emp_cache[i].valid && strcasecmp(s_emp_cache[i].uid, uid_str) == 0) {
      if (strcasecmp(s_emp_cache[i].name, "Akses Diterima") == 0 ||
          strcasecmp(s_emp_cache[i].name, "Akses Diterima!") == 0 ||
          strcasecmp(s_emp_cache[i].name, "UNKNOWN") == 0 ||
          strcasecmp(s_emp_cache[i].name, "-") == 0) {
        continue;
      }
      strncpy(out_name, s_emp_cache[i].name, name_len - 1);
      out_name[name_len - 1] = '\0';
      if (out_pos && pos_len > 0) {
        strncpy(out_pos, s_emp_cache[i].pos, pos_len - 1);
        out_pos[pos_len - 1] = '\0';
      }
      return true;
    }
  }

  /* 2. Slow Path: Cari di file fisik MicroSD (USERS.CSV) */
  if (osMutexAcquire(sdioFatFsMutexHandle, 1000) != osOK)
    return false;

  if (!g_sd_mounted) {
    if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 0) == FR_OK) {
      g_sd_mounted = true;
    } else {
      osMutexRelease(sdioFatFsMutexHandle);
      return false;
    }
  }

  bool found = false;
  FIL file;
  FRESULT fr = f_open(&file, "USERS.CSV", FA_READ);
  if (fr == FR_NO_FILE) {
    /* File belum ada, buat template header kosong tanpa mock dummy */
    fr = f_open(&file, "USERS.CSV", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr == FR_OK) {
      const char hdr[] = "UID,NAMA,JABATAN\r\n";
      UINT bw;
      f_write(&file, hdr, strlen(hdr), &bw);
      f_sync(&file);
      f_close(&file);
      fr = f_open(&file, "USERS.CSV", FA_READ);
    }
  }

  if (fr == FR_OK) {
    char line[128];
    while (f_gets(line, sizeof(line), &file) != NULL) {
      size_t l = strlen(line);
      while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == '\n' ||
                       line[l - 1] == ' ')) {
        line[--l] = '\0';
      }
      if (l < 3)
        continue;
      if (strncasecmp(line, "UID,", 4) == 0)
        continue;

      char *token_uid = strtok(line, ",");
      char *token_name = strtok(NULL, ",");
      char *token_pos = strtok(NULL, ",");

      if (token_uid && token_name) {
        while (*token_uid == ' ')
          token_uid++;
        while (*token_name == ' ')
          token_name++;
        if (token_pos) {
          while (*token_pos == ' ')
            token_pos++;
        }

        /* Filter jika token_name adalah placeholder */
        if (strcasecmp(token_name, "Akses Diterima") == 0 ||
            strcasecmp(token_name, "Akses Diterima!") == 0 ||
            strcasecmp(token_name, "UNKNOWN") == 0 ||
            strcasecmp(token_name, "-") == 0 ||
            strlen(token_name) < 2) {
          continue;
        }

        if (strcasecmp(token_uid, uid_str) == 0) {
          strncpy(out_name, token_name, name_len - 1);
          out_name[name_len - 1] = '\0';
          if (out_pos && token_pos) {
            strncpy(out_pos, token_pos, pos_len - 1);
            out_pos[pos_len - 1] = '\0';
          }
          found = true;

          /* Masukkan ke RAM cache agar tap berikutnya instan */
          strncpy(s_emp_cache[s_emp_cache_idx].uid, uid_str,
                  sizeof(s_emp_cache[s_emp_cache_idx].uid) - 1);
          s_emp_cache[s_emp_cache_idx]
              .uid[sizeof(s_emp_cache[s_emp_cache_idx].uid) - 1] = '\0';
          strncpy(s_emp_cache[s_emp_cache_idx].name, out_name,
                  sizeof(s_emp_cache[s_emp_cache_idx].name) - 1);
          s_emp_cache[s_emp_cache_idx]
              .name[sizeof(s_emp_cache[s_emp_cache_idx].name) - 1] = '\0';
          if (out_pos) {
            strncpy(s_emp_cache[s_emp_cache_idx].pos, out_pos,
                    sizeof(s_emp_cache[s_emp_cache_idx].pos) - 1);
            s_emp_cache[s_emp_cache_idx]
                .pos[sizeof(s_emp_cache[s_emp_cache_idx].pos) - 1] = '\0';
          } else {
            s_emp_cache[s_emp_cache_idx].pos[0] = '\0';
          }
          s_emp_cache[s_emp_cache_idx].valid = true;
          s_emp_cache_idx = (s_emp_cache_idx + 1) % EMP_CACHE_SIZE;
          break;
        }
      }
    }
    f_close(&file);
  }

  osMutexRelease(sdioFatFsMutexHandle);
  return found;
}

/**
 * @brief  Membaca seluruh pengguna yang terdaftar di MicroSD (USERS.CSV)
 *         untuk ditampilkan di menu Admin "USERS"
 */
uint16_t Storage_GetRegisteredUsers(user_entry_t *out_users, uint16_t max_users) {
  if (out_users == NULL || max_users == 0)
    return 0;

  if (osMutexAcquire(sdioFatFsMutexHandle, 1000) != osOK)
    return 0;

  if (!g_sd_mounted) {
    if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 0) == FR_OK) {
      g_sd_mounted = true;
    } else {
      osMutexRelease(sdioFatFsMutexHandle);
      return 0;
    }
  }

  uint16_t count = 0;
  FIL file;
  if (f_open(&file, "USERS.CSV", FA_READ) == FR_OK) {
    char line[128];
    while (count < max_users && f_gets(line, sizeof(line), &file) != NULL) {
      size_t l = strlen(line);
      while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == '\n' || line[l - 1] == ' ')) {
        line[--l] = '\0';
      }
      if (l < 3 || strncasecmp(line, "UID,", 4) == 0)
        continue;

      char *token_uid = strtok(line, ",");
      char *token_name = strtok(NULL, ",");
      char *token_pos = strtok(NULL, ",");

      if (token_uid && token_name) {
        while (*token_uid == ' ') token_uid++;
        while (*token_name == ' ') token_name++;
        if (token_pos) { while (*token_pos == ' ') token_pos++; }

        /* Filter entri dummy / placeholder Akses Diterima */
        if (strcasecmp(token_name, "Akses Diterima") == 0 ||
            strcasecmp(token_name, "Akses Diterima!") == 0 ||
            strcasecmp(token_name, "UNKNOWN") == 0 ||
            strcasecmp(token_name, "-") == 0 ||
            strlen(token_name) < 2) {
          continue;
        }

        strncpy(out_users[count].uid, token_uid, sizeof(out_users[count].uid) - 1);
        out_users[count].uid[sizeof(out_users[count].uid) - 1] = '\0';

        strncpy(out_users[count].name, token_name, sizeof(out_users[count].name) - 1);
        out_users[count].name[sizeof(out_users[count].name) - 1] = '\0';

        if (token_pos && strlen(token_pos) > 0) {
          strncpy(out_users[count].pos, token_pos, sizeof(out_users[count].pos) - 1);
          out_users[count].pos[sizeof(out_users[count].pos) - 1] = '\0';
        } else {
          strcpy(out_users[count].pos, "-");
        }
        count++;
      }
    }
    f_close(&file);
  }

  osMutexRelease(sdioFatFsMutexHandle);
  return count;
}

/**
 * @brief  Membersihkan file USERS.CSV dari baris "Akses Diterima" atau entri korup
 */
void Storage_SanitizeUsersFile(void) {
  if (osMutexAcquire(sdioFatFsMutexHandle, 1000) != osOK)
    return;

  if (!g_sd_mounted) {
    if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 0) == FR_OK) {
      g_sd_mounted = true;
    } else {
      osMutexRelease(sdioFatFsMutexHandle);
      return;
    }
  }

  FIL f_in, f_out;
  if (f_open(&f_in, "USERS.CSV", FA_READ) != FR_OK) {
    osMutexRelease(sdioFatFsMutexHandle);
    return;
  }

  bool need_rewrite = false;
  char line[128];
  while (f_gets(line, sizeof(line), &f_in) != NULL) {
    if (strstr(line, "Akses Diterima") != NULL) {
      need_rewrite = true;
      break;
    }
  }

  if (need_rewrite) {
    f_lseek(&f_in, 0);
    if (f_open(&f_out, "USERS.TMP", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
      UINT bw;
      while (f_gets(line, sizeof(line), &f_in) != NULL) {
        if (strstr(line, "Akses Diterima") == NULL) {
          f_write(&f_out, line, strlen(line), &bw);
        }
      }
      f_sync(&f_out);
      f_close(&f_out);
      f_close(&f_in);
      f_unlink("USERS.CSV");
      f_rename("USERS.TMP", "USERS.CSV");
      osMutexRelease(sdioFatFsMutexHandle);
      return;
    }
  }

  f_close(&f_in);
  osMutexRelease(sdioFatFsMutexHandle);
}

/**
 * @brief  Menyimpan atau memperbarui pasangan UID kartu dan Nama Karyawan ke
 * MicroSD (USERS.CSV) Digunakan untuk Lazy Caching saat pertama kali tap kartu
 * berhasil divalidasi oleh server.
 */
bool Storage_SaveEmployee(const char *uid_str, const char *emp_name,
                          const char *emp_pos) {
  if (uid_str == NULL || strlen(uid_str) == 0)
    return false;
  if (emp_name == NULL || strlen(emp_name) < 2)
    return false;

  /* JANGAN PERNAH simpan Akses Diterima atau placeholder sebagai nama */
  if (strcasecmp(emp_name, "Akses Diterima") == 0 ||
      strcasecmp(emp_name, "Akses Diterima!") == 0 ||
      strcasecmp(emp_name, "UNKNOWN") == 0 ||
      strcasecmp(emp_name, "-") == 0) {
    return false;
  }

  /* 1. Update/Simpan ke Fast RAM Cache */
  bool in_ram = false;
  for (int i = 0; i < EMP_CACHE_SIZE; i++) {
    if (s_emp_cache[i].valid && strcasecmp(s_emp_cache[i].uid, uid_str) == 0) {
      strncpy(s_emp_cache[i].name, emp_name, sizeof(s_emp_cache[i].name) - 1);
      s_emp_cache[i].name[sizeof(s_emp_cache[i].name) - 1] = '\0';
      if (emp_pos) {
        strncpy(s_emp_cache[i].pos, emp_pos, sizeof(s_emp_cache[i].pos) - 1);
        s_emp_cache[i].pos[sizeof(s_emp_cache[i].pos) - 1] = '\0';
      }
      in_ram = true;
      break;
    }
  }
  if (!in_ram) {
    strncpy(s_emp_cache[s_emp_cache_idx].uid, uid_str,
            sizeof(s_emp_cache[s_emp_cache_idx].uid) - 1);
    s_emp_cache[s_emp_cache_idx]
        .uid[sizeof(s_emp_cache[s_emp_cache_idx].uid) - 1] = '\0';
    strncpy(s_emp_cache[s_emp_cache_idx].name, emp_name,
            sizeof(s_emp_cache[s_emp_cache_idx].name) - 1);
    s_emp_cache[s_emp_cache_idx]
        .name[sizeof(s_emp_cache[s_emp_cache_idx].name) - 1] = '\0';
    if (emp_pos) {
      strncpy(s_emp_cache[s_emp_cache_idx].pos, emp_pos,
              sizeof(s_emp_cache[s_emp_cache_idx].pos) - 1);
      s_emp_cache[s_emp_cache_idx]
          .pos[sizeof(s_emp_cache[s_emp_cache_idx].pos) - 1] = '\0';
    } else {
      s_emp_cache[s_emp_cache_idx].pos[0] = '\0';
    }
    s_emp_cache[s_emp_cache_idx].valid = true;
    s_emp_cache_idx = (s_emp_cache_idx + 1) % EMP_CACHE_SIZE;
  }

  /* 2. Pastikan MicroSD terpasang dan mount aktif */
  if (osMutexAcquire(sdioFatFsMutexHandle, 1000) != osOK)
    return false;

  if (!g_sd_mounted) {
    if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 0) == FR_OK) {
      g_sd_mounted = true;
    } else {
      osMutexRelease(sdioFatFsMutexHandle);
      return false;
    }
  }

  /* 3. Cek apakah kartu SUDAH TERTULIS di file fisik USERS.CSV */
  FIL file;
  FRESULT fr = f_open(&file, "USERS.CSV", FA_READ);
  bool already_in_file = false;

  if (fr == FR_OK) {
    char line[128];
    while (f_gets(line, sizeof(line), &file) != NULL) {
      size_t l = strlen(line);
      while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == '\n' ||
                       line[l - 1] == ' ')) {
        line[--l] = '\0';
      }
      if (l < 3)
        continue;
      if (strncasecmp(line, "UID,", 4) == 0)
        continue;

      char *token_uid = strtok(line, ",");
      char *token_name = strtok(NULL, ",");

      if (token_uid && token_name) {
        while (*token_uid == ' ')
          token_uid++;
        while (*token_name == ' ')
          token_name++;
        if (strcasecmp(token_uid, uid_str) == 0 &&
            strcmp(token_name, emp_name) == 0) {
          already_in_file = true;
          break;
        }
      }
    }
    f_close(&file);
  }

  if (already_in_file) {
    osMutexRelease(sdioFatFsMutexHandle);
    return true; /* Sudah ada di MicroSD secara fisik, tidak perlu menulis ulang
                  */
  }

  /* 4. Tulis langsung ke USERS.CSV di MicroSD dan FLUSH fisik (f_sync) */
  fr = f_open(&file, "USERS.CSV", FA_OPEN_ALWAYS | FA_WRITE);
  if (fr == FR_OK) {
    if (f_size(&file) == 0) {
      const char *hdr = "UID,NAMA,JABATAN\r\n";
      UINT bw;
      f_write(&file, hdr, strlen(hdr), &bw);
    }
    f_lseek(&file, f_size(&file));

    char new_line[128];
    snprintf(new_line, sizeof(new_line), "%s,%s,%s\r\n", uid_str, emp_name,
             (emp_pos && strlen(emp_pos) > 0) ? emp_pos : "-");
    UINT bw;
    f_write(&file, new_line, strlen(new_line), &bw);
    f_sync(&file); /* Penting: Paksa tulis ke flash fisik SD card sebelum daya
                      dicabut */
    f_close(&file);
    osMutexRelease(sdioFatFsMutexHandle);
    return true;
  }

  osMutexRelease(sdioFatFsMutexHandle);
  return false;
}

/**
 * @brief  Menghitung jumlah transaksi di antrean PENDING.CSV yang belum
 * tersinkron
 */
uint32_t Storage_GetPendingCount(void) {
  if (!g_sd_mounted)
    return 0;
  if (osMutexAcquire(sdioFatFsMutexHandle, 100) != osOK)
    return 0;

  uint32_t count = 0;
  FIL file;
  if (f_open(&file, "PENDING.CSV", FA_READ) == FR_OK) {
    char line[128];
    while (f_gets(line, sizeof(line), &file) != NULL) {
      size_t l = strlen(line);
      while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == '\n'))
        line[--l] = '\0';
      if (l > 5)
        count++;
    }
    f_close(&file);
  }
  osMutexRelease(sdioFatFsMutexHandle);
  return count;
}

/**
 * @brief  Memicu proses bulk sync data antrean ke ESP32
 */
void Storage_TriggerBulkSync(void) {
  if (!g_sd_mounted) {
    GUI_Attendance_SetBulkStatus("Gagal: MicroSD tidak terpasang!", false);
    return;
  }
  if (!g_wifi_connected) {
    GUI_Attendance_SetBulkStatus("Gagal: Wi-Fi belum terhubung!", false);
    return;
  }
  if (osMutexAcquire(sdioFatFsMutexHandle, 500) != osOK) {
    GUI_Attendance_SetBulkStatus("Gagal: MicroSD sibuk!", false);
    return;
  }

  FIL file;
  if (f_open(&file, "PENDING.CSV", FA_READ) != FR_OK) {
    osMutexRelease(sdioFatFsMutexHandle);
    GUI_Attendance_SetBulkStatus("Antrean kosong (0 data).", true);
    return;
  }

  /* 1. Hitung berapa item valid yang akan dikirim (maksimum 50 item per batch)
   */
  uint32_t count = 0;
  char line[128];
  while (count < 50 && f_gets(line, sizeof(line), &file) != NULL) {
    size_t l = strlen(line);
    while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == '\n'))
      line[--l] = '\0';
    if (l > 5) {
      count++;
    }
  }

  if (count == 0) {
    f_close(&file);
    osMutexRelease(sdioFatFsMutexHandle);
    GUI_Attendance_SetBulkStatus("Antrean kosong (0 data).", true);
    return;
  }

  /* Rewind ke awal file untuk streaming pengiriman data */
  f_lseek(&file, 0);

  /* Kirim sinyal BULK_START */
  esp_tx_item_t tx_start = {0};
  snprintf(tx_start.payload, sizeof(tx_start.payload), "BULK_START:%lu\n",
           (unsigned long)count);
  osMessageQueuePut(espTxQueueHandle, &tx_start, 0, 100);

  /* 2. Streaming baris per baris tanpa alokasi array besar pada stack */
  uint32_t sent = 0;
  while (sent < count && f_gets(line, sizeof(line), &file) != NULL) {
    size_t l = strlen(line);
    while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == '\n'))
      line[--l] = '\0';
    if (l > 5) {
      esp_tx_item_t tx_item = {0};
      snprintf(tx_item.payload, sizeof(tx_item.payload), "BULK_ITEM:%.116s\n",
               line);
      osMessageQueuePut(espTxQueueHandle, &tx_item, 0, 100);
      sent++;
      Watchdog_CheckIn(WDT_TASK_STORAGE);
      osDelay(10);
    }
  }

  f_close(&file);
  osMutexRelease(sdioFatFsMutexHandle);

  /* Kirim sinyal BULK_END */
  esp_tx_item_t tx_end = {0};
  snprintf(tx_end.payload, sizeof(tx_end.payload), "BULK_END\n");
  osMessageQueuePut(espTxQueueHandle, &tx_end, 0, 100);

  GUI_Attendance_SetBulkStatus("Mengirim data bulk ke server...", true);
}

/**
 * @brief  Menghapus batch data transaksi yang sudah sukses diterima server
 */
void Storage_RemovePendingBatch(uint32_t count) {
  if (count == 0 || !g_sd_mounted)
    return;
  if (osMutexAcquire(sdioFatFsMutexHandle, 500) != osOK)
    return;

  FIL fin, fout;
  if (f_open(&fin, "PENDING.CSV", FA_READ) == FR_OK) {
    if (f_open(&fout, "PEND_TMP.CSV", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
      char line[128];
      uint32_t skipped = 0;
      UINT bw;
      while (f_gets(line, sizeof(line), &fin) != NULL) {
        size_t l = strlen(line);
        while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == '\n'))
          line[--l] = '\0';
        if (l > 5) {
          if (skipped < count) {
            skipped++;
          } else {
            f_write(&fout, line, strlen(line), &bw);
            f_write(&fout, "\r\n", 2, &bw);
          }
        }
      }
      f_close(&fout);
      f_close(&fin);
      f_unlink("PENDING.CSV");
      f_rename("PEND_TMP.CSV", "PENDING.CSV");
    } else {
      f_close(&fin);
    }
  }
  osMutexRelease(sdioFatFsMutexHandle);
}

/**
 * @brief  Menghapus satu record transaksi spesifik berdasarkan transaction UID
 */
void Storage_RemovePendingUid(const char *txn_uid) {
  if (txn_uid == NULL || strlen(txn_uid) == 0 || !g_sd_mounted)
    return;
  if (osMutexAcquire(sdioFatFsMutexHandle, 500) != osOK)
    return;

  FIL fin, fout;
  if (f_open(&fin, "PENDING.CSV", FA_READ) == FR_OK) {
    if (f_open(&fout, "PEND_TMP.CSV", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
      char line[128];
      UINT bw;
      while (f_gets(line, sizeof(line), &fin) != NULL) {
        size_t l = strlen(line);
        while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == '\n'))
          line[--l] = '\0';
        if (l > 5) {
          if (strncmp(line, txn_uid, strlen(txn_uid)) == 0 &&
              line[strlen(txn_uid)] == '|') {
            continue; /* Lewati baris ini (dihapus) */
          }
          f_write(&fout, line, strlen(line), &bw);
          f_write(&fout, "\r\n", 2, &bw);
        }
      }
      f_close(&fout);
      f_close(&fin);
      f_unlink("PENDING.CSV");
      f_rename("PEND_TMP.CSV", "PENDING.CSV");
    } else {
      f_close(&fin);
    }
  }
  osMutexRelease(sdioFatFsMutexHandle);
}

/**
 * @brief  Pengujian diagnostik hardware RFID Reader CLRC663 via SPI3
 */
bool RFID_TestDiagnostic(char *out_msg, size_t max_len) {
  uint8_t ver = 0;
  if (osMutexAcquire(spi2MutexHandle, 500) == osOK) {
    ver = CLRC663_ReadVersion();
    if (ver == 0x00 || ver == 0xFF) {
      if (CLRC663_Init(&hspi3) == CLRC663_OK) {
        g_clrc663_ready = true;
        CLRC663_AntennaOn();
        CLRC663_SetupISO14443A();
        ver = CLRC663_ReadVersion();
      }
    }
    osMutexRelease(spi2MutexHandle);
  }

  if (ver != 0x00 && ver != 0xFF) {
    snprintf(out_msg, max_len,
             "OK: Chip CLRC663 terdeteksi (Versi 0x%02X). Antena RF aktif.",
             ver);
    return true;
  } else {
    snprintf(out_msg, max_len,
             "GAGAL: Chip CLRC663 tidak merespons SPI3 (Ver=0x%02X). Cek pin "
             "SCK/MISO/MOSI/CS & 3.3V.",
             ver);
    return false;
  }
}

/**
 * @brief  Pengujian diagnostik status koneksi Wi-Fi ESP32-C6
 */
bool WiFi_GetDiagnostic(char *out_msg, size_t max_len) {
  /* Kirim ping query status ke ESP32 */
  esp_tx_item_t item = {0};
  snprintf(item.payload, sizeof(item.payload), "CMD:GET_WIFI_STATUS\n");
  osMessageQueuePut(espTxQueueHandle, &item, 0, 50);

  if (g_wifi_connected) {
    snprintf(out_msg, max_len,
             "OK: Wi-Fi terhubung via ESP32-C6. Koneksi AP aktif.");
    return true;
  } else {
    snprintf(out_msg, max_len,
             "PERINGATAN: Wi-Fi belum terhubung. Cek konfigurasi SSID & "
             "Password di tab WI-FI.");
    return false;
  }
}

/**
 * @brief  Pengujian diagnostik RTC internal
 */
bool RTC_GetDiagnostic(char *out_msg, size_t max_len) {
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
  snprintf(out_msg, max_len,
           "OK: RTC berdetak normal: %02d:%02d:%02d WIB (20%02d-%02d-%02d).",
           sTime.Hours, sTime.Minutes, sTime.Seconds, sDate.Year, sDate.Month,
           sDate.Date);
  return true;
}

/**
 * @brief  Pengujian diagnostik Buzzer
 */
void Buzzer_TestDiagnostic(void) {
  HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
  osDelay(60);
  HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);
  osDelay(50);
  HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
  osDelay(80);
  HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  LVGL Touch input callback reading from XPT2046 with mutex protection
 */
static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  uint16_t touch_x = 0, touch_y = 0;
  if (osMutexAcquire(spi2MutexHandle, 5) == osOK) {
    if (XPT2046_GetTouch(&touch_x, &touch_y)) {
      if (GUI_Attendance_IsSleeping()) {
        /* Bangunkan layar dari mode sleep dan abaikan sentuhan pertama */
        GUI_Attendance_Wakeup();
        data->state = LV_INDEV_STATE_RELEASED;
        osMutexRelease(spi2MutexHandle);
        return;
      }

      GUI_Attendance_NotifyActivity();
      data->state = LV_INDEV_STATE_PRESSED;
      data->point.x = touch_x;
      data->point.y = touch_y;
      GUI_Attendance_CheckCornerTouch(touch_x, touch_y);
    } else {
      data->state = LV_INDEV_STATE_RELEASED;
      GUI_Attendance_CheckCornerTouch(0, 0);
    }
    osMutexRelease(spi2MutexHandle);
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
    GUI_Attendance_CheckCornerTouch(0, 0);
  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartRfidTask */
/**
 * @brief  Function implementing the Task_RFID thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartRfidTask */
/* ========================================================================== */
/*                STM32 LOW POWER STOP MODE WITH REGULATOR LP                 */
/* ========================================================================== */
void SystemClock_RestoreFromStop(void) {
  if (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_PLLCLK) {
    SystemClock_Config();
  }
}

void Power_EnterStopMode(void) {
  /* 1. Feed IWDG sebelum tidur */
  HAL_IWDG_Refresh(&hiwdg);

  /* 2. Suspend SysTick agar interrupt SysTick 1ms tidak membangunkan MCU */
  HAL_SuspendTick();

  /* 3. Masuk ke STOP Mode dengan Regulator Low Power.
     Konsumsi arus STM32F405 turun dari ~50mA menjadi ~300uA.
     MCU akan bangun jika ada sinyal EXTI (PA15 RFID LPCD, PC6 Touch, atau RTC
     1s). */
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

  /* 4. MCU Bangun: Clock secara otomatis beralih ke HSI 16MHz.
     Segera kembalikan clock ke 168MHz (HSE + PLL). */
  SystemClock_RestoreFromStop();

  /* 5. Resume SysTick */
  HAL_ResumeTick();

  /* 6. Segarkan IWDG segera setelah bangun */
  HAL_IWDG_Refresh(&hiwdg);
}

void StartRfidTask(void *argument) {
  /* USER CODE BEGIN 5 */
  /* 1. Inisialisasi Hardware RFID Reader NXP CLRC66303 */
  osMutexAcquire(spi2MutexHandle, osWaitForever);
  if (CLRC663_Init(&hspi3) == CLRC663_OK) {
    g_clrc663_ready = true;
    g_clrc663_version = CLRC663_ReadVersion();
    CLRC663_AntennaOn();
    CLRC663_SetupISO14443A();
    /* Lakukan kalibrasi baseline demodulator I/Q untuk LPCD */
    CLRC663_LPCD_Calibrate(NULL, NULL);
  } else {
    g_clrc663_ready = false;
    g_clrc663_version = CLRC663_ReadVersion();
  }
  osMutexRelease(spi2MutexHandle);

  /* Infinite loop */
  for (;;) {
    Watchdog_CheckIn(WDT_TASK_RFID);

    /* Cek apakah layar sedang dalam kondisi sleep / idle */
    bool is_asleep = GUI_Attendance_IsSleeping();

    if (is_asleep && g_clrc663_ready) {
      /* Saat layar sleep: masukkan CLRC663 ke mode LPCD (Low-Power Card
         Detection). Antena pemancar RF 13.56 MHz dimatikan (arus turun dari
         ~120mA ke ~30uA). Chip bangun setiap 300 ms secara periodik. Jika ada
         kartu mendekat, pin IRQ (PA15) akan menarik interrupt EXTI untuk
         membangunkan task ini. */
      if (osMutexAcquire(spi2MutexHandle, 50) == osOK) {
        if (!CLRC663_LPCD_IsActive()) {
          uint8_t delta = (g_app_settings.lpcd_delta >= 1 && g_app_settings.lpcd_delta <= 10) ? g_app_settings.lpcd_delta : 3;
          uint16_t interval = (g_app_settings.lpcd_interval_ms >= 100 && g_app_settings.lpcd_interval_ms <= 2000) ? g_app_settings.lpcd_interval_ms : 300;
          CLRC663_LPCD_Start(delta, interval);
        }
        osMutexRelease(spi2MutexHandle);
      }

      /* Tidur hemat daya menunggu Task Notification dari PA15 EXTI (kartu
         menempel) atau timeout 1000 ms agar Watchdog tetap ter-feed */
      uint32_t woken = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
      if (woken > 0) {
        /* Terdeteksi interupsi dari pin PA15 (kartu mendekat saat LPCD) */
        if (osMutexAcquire(spi2MutexHandle, 50) == osOK) {
          CLRC663_LPCD_Stop();
          osMutexRelease(spi2MutexHandle);
        }
        /* Bangunkan layar segera */
        GUI_Attendance_Wakeup();
        /* Beri waktu 15ms agar transponder pasif kartu RFID ter-charge oleh
         * medan RF */
        osDelay(15);
      } else {
        /* Timeout 1s: loop kembali untuk check-in watchdog */
        continue;
      }
    } else {
      /* Saat layar aktif / baru bangun: pastikan antena menyala dan mode LPCD
       * dimatikan */
      if (osMutexAcquire(spi2MutexHandle, 50) == osOK) {
        if (CLRC663_LPCD_IsActive()) {
          CLRC663_LPCD_Stop();
        }
        osMutexRelease(spi2MutexHandle);
      }
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));
    }

    /* Kunci spi2Mutex sebelum mengakses reader kartu */
    if (osMutexAcquire(spi2MutexHandle, 50) == osOK) {
      if (!g_clrc663_ready) {
        if (CLRC663_Init(&hspi3) == CLRC663_OK) {
          g_clrc663_ready = true;
          CLRC663_AntennaOn();
          CLRC663_SetupISO14443A();
          CLRC663_LPCD_Calibrate(NULL, NULL);
        }
      }

      if (g_clrc663_ready && !CLRC663_LPCD_IsActive()) {
        CLRC663_Card_t card;
        if (CLRC663_ReadCard(&card)) {
          /* Debounce kartu yang sama dalam durasi 1500 ms */
          uint32_t now = HAL_GetTick();
          bool is_duplicate = false;
          if (card.uid_len == g_prev_uid_len &&
              (now - g_prev_uid_tick < 1500)) {
            if (memcmp(card.uid, g_prev_uid, card.uid_len) == 0) {
              is_duplicate = true;
            }
          }

          if (!is_duplicate) {
            /* Bangunkan layar seketika saat kartu terdeteksi */
            GUI_Attendance_Wakeup();

            /* Simpan UID untuk debounce */
            memcpy(g_prev_uid, card.uid, card.uid_len);
            g_prev_uid_len = card.uid_len;
            g_prev_uid_tick = now;

            /* Ambil Waktu & Tanggal dari RTC internal */
            RTC_TimeTypeDef sTime = {0};
            RTC_DateTypeDef sDate = {0};
            HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

            char uid_str[24] = {0};
            int offset = 0;
            for (uint8_t i = 0; i < card.uid_len && i < 10 &&
                                offset < (int)sizeof(uid_str) - 3;
                 i++) {
              offset += snprintf(uid_str + offset, sizeof(uid_str) - offset,
                                 "%02X", card.uid[i]);
            }
            if (offset == 0) {
              snprintf(uid_str, sizeof(uid_str), "UNKNOWN");
            }

            /* 1. Offline-First: Periksa apakah kartu sudah tersimpan di memori
             * lokal */
            char emp_name[48] = {0};
            char emp_pos[32] = {0};
            bool emp_found = Storage_LookupEmployee(
                uid_str, emp_name, sizeof(emp_name), emp_pos, sizeof(emp_pos));

            if (emp_found && strlen(emp_name) > 0) {
              /* =============================================================
               */
              /* KONDISI A: DATA DITEMUKAN DI LOKAL (OFFLINE-FIRST) */
              /* =============================================================
               */
              /* Bunyikan 1x beep sukses & nyalakan USER LED singkat (75 ms) */
              HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                                GPIO_PIN_SET);
              HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_SET);
              osDelay(75);
              HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                                GPIO_PIN_RESET);
              HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin,
                                GPIO_PIN_RESET);

              /* Tampilkan deteksi kartu dan nama karyawan ke GUI */
              ui_event_t ui_evt = {0};
              ui_evt.type = UI_EVENT_RFID_CARD_DETECTED;
              ui_evt.status = 1;
              ui_evt.uid_len = card.uid_len;
              memcpy(ui_evt.uid, card.uid, card.uid_len);
              snprintf(ui_evt.extra, sizeof(ui_evt.extra), "%02d:%02d:%02d WIB",
                       sTime.Hours, sTime.Minutes, sTime.Seconds);
              osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);

              ui_event_t ui_ok = {0};
              ui_ok.type = UI_EVENT_ATTENDANCE_SUCCESS;
              ui_ok.status = 1;
              strncpy(ui_ok.extra, emp_name, sizeof(ui_ok.extra) - 1);
              osMessageQueuePut(uiEventQueueHandle, &ui_ok, 0, 10);

              /* Kirim data absensi ke sdLogQueue untuk disimpan ke ATTEND.CSV &
               * PENDING.CSV */
              sd_log_item_t sd_item = {0};
              sd_item.year = sDate.Year;
              sd_item.month = sDate.Month;
              sd_item.date = sDate.Date;
              sd_item.hours = sTime.Hours;
              sd_item.minutes = sTime.Minutes;
              sd_item.seconds = sTime.Seconds;
              sd_item.uid_len = card.uid_len;
              memcpy(sd_item.uid, card.uid, card.uid_len);
              strncpy(sd_item.log_msg, emp_name, sizeof(sd_item.log_msg) - 1);
              osMessageQueuePut(sdLogQueueHandle, &sd_item, 0, 10);

            } else {
              /* =============================================================
               */
              /* KONDISI B: KARTU BELUM ADA DI LOKAL (TAP PERTAMA / BARU) */
              /* =============================================================
               */
              if (!g_wifi_connected) {
                /* Wi-Fi mati: Kartu baru tidak bisa diverifikasi ke server */
                HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                                  GPIO_PIN_SET);
                osDelay(250);
                HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                                  GPIO_PIN_RESET);

                ui_event_t ui_fail = {0};
                ui_fail.type = UI_EVENT_ATTENDANCE_REJECTED;
                ui_fail.status = 0;
                strncpy(ui_fail.extra, "Kartu Baru (Offline)",
                        sizeof(ui_fail.extra) - 1);
                osMessageQueuePut(uiEventQueueHandle, &ui_fail, 0, 10);

              } else {
                /* Wi-Fi aktif: Tanya server via ESP32 "Kartu ini milik siapa?"
                 */
                HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                                  GPIO_PIN_SET);
                osDelay(30);
                HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                                  GPIO_PIN_RESET);

                ui_event_t ui_evt = {0};
                ui_evt.type = UI_EVENT_RFID_CARD_DETECTED;
                ui_evt.status = 1;
                ui_evt.uid_len = card.uid_len;
                memcpy(ui_evt.uid, card.uid, card.uid_len);
                strncpy(ui_evt.extra, "Cek Server...",
                        sizeof(ui_evt.extra) - 1);
                osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);

                char time_iso[32];
                snprintf(time_iso, sizeof(time_iso),
                         "20%02d-%02d-%02d %02d:%02d:%02d", sDate.Year,
                         sDate.Month, sDate.Date, sTime.Hours, sTime.Minutes,
                         sTime.Seconds);

                char dev_code_buf[32];
                if (g_app_settings.device_code[0] != '\0') {
                  strncpy(dev_code_buf, g_app_settings.device_code,
                          sizeof(dev_code_buf) - 1);
                  dev_code_buf[sizeof(dev_code_buf) - 1] = '\0';
                } else {
                  uint16_t dev_id =
                      (uint16_t)((HAL_GetUIDw0() ^ HAL_GetUIDw1() ^
                                  HAL_GetUIDw2()) &
                                 0xFFFF);
                  snprintf(dev_code_buf, sizeof(dev_code_buf), "GATE-%04X",
                           (unsigned int)dev_id);
                }

                char txn_uid[96];
                snprintf(txn_uid, sizeof(txn_uid),
                         "%.24s_%02d%02d%02d%02d%02d%02d_%.24s", dev_code_buf,
                         sDate.Year, sDate.Month, sDate.Date, sTime.Hours,
                         sTime.Minutes, sTime.Seconds, uid_str);

                esp_tx_item_t esp_item = {0};
                snprintf(esp_item.payload, sizeof(esp_item.payload),
                         "TXN:%.64s|%.20s|%.24s|rfid|tap\n", txn_uid, time_iso,
                         uid_str);
                osMessageQueuePut(espTxQueueHandle, &esp_item, 0, 50);
              }
            }
          }
        }
      }
      osMutexRelease(spi2MutexHandle);
    }
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartGuiTask */
/**
 * @brief Function implementing the Task_GUI thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartGuiTask */
void StartGuiTask(void *argument) {
  /* USER CODE BEGIN StartGuiTask */
  /* Muat konfigurasi tersimpan dari Flash STM32 (Sector 11) */
  App_Settings_Init();

  /* 1. Inisialisasi Core LVGL */
  lv_init();
  lv_tick_set_cb(HAL_GetTick);

  /* Tambahkan pool memori sekunder 16 KB dari SRAM ke allocator LVGL (total pool: 62 KB CCMRAM + 16 KB SRAM = 78 KB) */
  static uint8_t s_lvgl_extra_pool[16384] __attribute__((aligned(4)));
  lv_mem_add_pool(s_lvgl_extra_pool, sizeof(s_lvgl_extra_pool));

  /* 2. Setup Display LVGL untuk ILI9488 (RGB888) */
  lv_display_t *disp = lv_display_create(ILI9488_WIDTH, ILI9488_HEIGHT);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);
  lv_display_set_flush_cb(disp, my_disp_flush);

/* Buffer render parsial LVGL (16 baris @ 480 px = 23040 byte) */
#define DRAW_BUF_SIZE (ILI9488_WIDTH * 16 * 3)
  static uint8_t lv_draw_buf[DRAW_BUF_SIZE] __attribute__((aligned(64)));
  lv_display_set_buffers(disp, lv_draw_buf, NULL, sizeof(lv_draw_buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  /* 3. Setup Input Device Touchscreen (XPT2046) */
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  Watchdog_KickDirect();

  /* 4. Inisialisasi Antarmuka GUI Absensi */
  GUI_Attendance_Init();

  /* 5. Render frame pertama ke ILI9488 SEBELUM menyalakan backlight agar tidak ada blank putih */
  lv_timer_handler();

  /* 6. Nyalakan Backlight Layar LCD via PWM TIM3 CH3 setelah gambar pertama selesai */
  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  uint32_t compare = ((uint32_t)g_app_settings.brightness * 65535) / 100;
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, compare);

  Watchdog_KickDirect();

  uint32_t last_clock_tick = 0;

  /* Infinite loop GUI Task */
  for (;;) {
    Watchdog_CheckIn(WDT_TASK_GUI);
    /* 1. Periksa event masuk dari uiEventQueue (misal dari Task_RFID,
     * Task_ESP32) */
    ui_event_t evt;
    while (osMessageQueueGet(uiEventQueueHandle, &evt, NULL, 0) == osOK) {
      GUI_Attendance_ProcessEvent(&evt);
    }

    /* 2. Periksa screen sleep timer */
    GUI_Attendance_CheckSleep();

    /* 3. Manajemen daya & refresh GUI */
    if (GUI_Attendance_IsSleeping()) {
      /* Jika layar sedang sleep, semua antrean kosong, dan CLRC663 sudah dalam
         mode LPCD: Tidurkan STM32F405 ke STOP Mode (Low-Power Regulator).
         Konsumsi daya STM32 anjlok dari ~50mA ke ~300 uA.
         MCU akan bangun instan jika ada:
         - Kartu menempel (PA15 EXTI dari CLRC663 LPCD)
         - Sentuhan layar (PC6 EXTI dari XPT2046)
         - Tombol fisik (PA0 EXTI)
         - Interupsi RTC Wakeup 1s berkala untuk feeding Watchdog */
      if (osMessageQueueGetCount(uiEventQueueHandle) == 0 &&
          osMessageQueueGetCount(espTxQueueHandle) == 0 &&
          osMessageQueueGetCount(sdLogQueueHandle) == 0 &&
          CLRC663_LPCD_IsActive()) {
        Power_EnterStopMode();
      }

      /* Saat layar dalam mode sleep:
       * - Tunggu interupsi sentuh dari XPT2046 (/PENIRQ jatuh ke LOW via EXTI)
       * atau timeout 50ms
       * - Bangunkan layar segera bila disentuh */
      if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50)) > 0 ||
          XPT2046_IsTouched()) {
        GUI_Attendance_Wakeup();
        lv_timer_handler();
      } else {
        /* Deteksi jika MicroSD tiba-tiba dicabut saat layar dalam mode sleep */
        if (g_sd_mounted &&
            (HAL_GPIO_ReadPin(SDIO_DET_GPIO_Port, SDIO_DET_Pin) !=
             GPIO_PIN_RESET)) {
          g_sd_mounted = false;
          if (osMutexAcquire(sdioFatFsMutexHandle, 100) == osOK) {
            f_mount(NULL, (TCHAR const *)SDPath, 0);
            BSP_SD_DeInit();
            osMutexRelease(sdioFatFsMutexHandle);
          }
          GUI_Attendance_Wakeup();
          GUI_Attendance_ShowSDWarning(true);
          HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                            GPIO_PIN_SET);
          osDelay(80);
          HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                            GPIO_PIN_RESET);
        }
      }
    } else {
      /* Saat layar aktif: perbarui jam digital, status MicroSD & badge
       * peripheral setiap 500 ms */
      uint32_t now = HAL_GetTick();
      if (now - last_clock_tick >= 500) {
        last_clock_tick = now;

        /* Deteksi status fisik kartu MicroSD secara real-time */
        bool sd_phys_present =
            (HAL_GPIO_ReadPin(SDIO_DET_GPIO_Port, SDIO_DET_Pin) ==
             GPIO_PIN_RESET);

        if (g_sd_mounted) {
          /* Kartu sebelumnya tercatat terpasang: verifikasi apakah dicabut */
          bool card_removed = !sd_phys_present;
          if (!card_removed) {
            if (osMutexAcquire(sdioFatFsMutexHandle, 20) == osOK) {
              HAL_SD_CardStateTypeDef cstate = HAL_SD_GetCardState(&hsd);
              osMutexRelease(sdioFatFsMutexHandle);
              if (cstate == HAL_SD_CARD_ERROR) {
                card_removed = true;
              }
            }
          }

          if (card_removed) {
            /* MicroSD DICABUT saat sedang beroperasi! */
            g_sd_mounted = false;
            if (osMutexAcquire(sdioFatFsMutexHandle, 100) == osOK) {
              f_mount(NULL, (TCHAR const *)SDPath, 0);
              BSP_SD_DeInit();
              osMutexRelease(sdioFatFsMutexHandle);
            }
            GUI_Attendance_ShowSDWarning(true);

            /* Alarm buzzer 2x beep tanda peringatan MicroSD dicabut */
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                              GPIO_PIN_SET);
            osDelay(60);
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                              GPIO_PIN_RESET);
            osDelay(40);
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                              GPIO_PIN_SET);
            osDelay(60);
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                              GPIO_PIN_RESET);
          }
        } else {
          /* Kartu sebelumnya belum terpasang: cek apakah baru dimasukkan
           * kembali */
          if (sd_phys_present) {
            osDelay(100); /* Debounce stabilisasi pin deteksi mekanik */
            if (osMutexAcquire(sdioFatFsMutexHandle, 200) == osOK) {
              BSP_SD_DeInit();
              if (BSP_SD_Init() == MSD_OK) {
                if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 0) == FR_OK) {
                  g_sd_mounted = true;
                  GUI_Attendance_ShowSDWarning(false);

                  /* Nada konfirmasi 1x beep tanda MicroSD terpasang kembali */
                  HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                                    GPIO_PIN_SET);
                  osDelay(60);
                  HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                                    GPIO_PIN_RESET);
                }
              }
              osMutexRelease(sdioFatFsMutexHandle);
            }
          }
        }

        RTC_TimeTypeDef sTime = {0};
        RTC_DateTypeDef sDate = {0};
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        GUI_Attendance_UpdateClock(&sTime, &sDate);
        GUI_Attendance_UpdateBadges(g_clrc663_ready, g_wifi_connected,
                                    g_sd_mounted);
      }

      /* Jalankan LVGL timer handler */
      lv_timer_handler();

      /* Yield task ke scheduler FreeRTOS */
      osDelay(5);
    }
  }
  /* USER CODE END StartGuiTask */
}

/* USER CODE BEGIN Header_StartEsp32Task */
/**
 * @brief Function implementing the Task_ESP32 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartEsp32Task */
void StartEsp32Task(void *argument) {
  /* USER CODE BEGIN StartEsp32Task */
  /* Aktifkan penerimaan UART1 interrupt dari ESP32-C6 */
  HAL_UART_Receive_IT(&huart1, &uart1_rx_data, 1);

  /* Beri jeda sejenak lalu kirim identitas device & konfigurasi ke ESP32 */
  osDelay(400);

  esp_tx_item_t init_dev = {0};
  snprintf(init_dev.payload, sizeof(init_dev.payload), "CMD:SET_DEVICE:%s|%s\n",
           g_app_settings.device_code, g_app_settings.device_name);
  HAL_UART_Transmit(&huart1, (uint8_t *)init_dev.payload,
                    strlen(init_dev.payload), 100);
  osDelay(50);

  esp_tx_item_t init_sync = {0};
  snprintf(init_sync.payload, sizeof(init_sync.payload),
           "CMD:SET_SYNC_MODE:%d\n", (int)g_app_settings.sync_mode);
  HAL_UART_Transmit(&huart1, (uint8_t *)init_sync.payload,
                    strlen(init_sync.payload), 100);
  osDelay(50);

  const char *cmd_wifi_status = "CMD:GET_WIFI_STATUS\n";
  HAL_UART_Transmit(&huart1, (uint8_t *)cmd_wifi_status,
                    strlen(cmd_wifi_status), 100);

  for (;;) {
    Watchdog_CheckIn(WDT_TASK_ESP32);
    /* 1. Ambil data dari espTxQueue jika ada */
    esp_tx_item_t tx_item;
    if (osMessageQueueGet(espTxQueueHandle, &tx_item, NULL, 30) == osOK) {
      /* Kirim ke ESP32-C6 via USART1 */
      HAL_UART_Transmit(&huart1, (uint8_t *)tx_item.payload,
                        strlen(tx_item.payload), 100);
    }

    /* 2. Proses pesan balasan serial dari ESP32-C6 (ACK / NTP / Verifikasi
     * Nama) */
    if (uart1_rx_ready) {
      uart1_rx_ready = 0;

      if (strncmp(uart1_rx_buf, "TXN_OK:", 7) == 0) {
        /* Realtime transaksi sukses dikirim ke server: hapus dari queue lokal
         */
        char *txn_uid = uart1_rx_buf + 7;
        size_t l = strlen(txn_uid);
        while (l > 0 && (txn_uid[l - 1] == '\r' || txn_uid[l - 1] == '\n'))
          txn_uid[--l] = '\0';
        Storage_RemovePendingUid(txn_uid);
        GUI_Attendance_UpdatePendingCount(Storage_GetPendingCount());
      } else if (strncmp(uart1_rx_buf, "BULK_ACK:", 9) == 0) {
        /* Bulk transfer response: BULK_ACK:<accepted>|<rejected> */
        int acc = 0, rej = 0;
        if (sscanf(uart1_rx_buf + 9, "%d|%d", &acc, &rej) >= 1) {
          if (acc > 0) {
            Storage_RemovePendingBatch((uint32_t)acc);
          }
          uint32_t rem_pending = Storage_GetPendingCount();
          GUI_Attendance_UpdatePendingCount(rem_pending);

          /* Jika masih ada antrean yang tersisa dan batch ini berhasil,
           * lanjutkan kirim batch berikutnya sampai habis! */
          if (rem_pending > 0 && acc > 0 && rej == 0 && g_wifi_connected) {
            g_bulk_sync_requested = true;
          } else {
            ui_event_t ui_evt = {0};
            ui_evt.type = UI_EVENT_BULK_SYNC_DONE;
            ui_evt.status = (rej == 0 && (acc > 0 || rem_pending == 0))
                                ? 1
                                : ((acc > 0) ? 2 : 0);
            if (rem_pending == 0 && acc > 0) {
              snprintf(ui_evt.extra, sizeof(ui_evt.extra),
                       "Semua antrean (%d data) sukses tersinkron!", acc);
            } else if (acc > 0 && rej == 0) {
              snprintf(ui_evt.extra, sizeof(ui_evt.extra),
                       "Sukses: %d data terekam di server!", acc);
            } else if (acc > 0 && rej > 0) {
              snprintf(ui_evt.extra, sizeof(ui_evt.extra),
                       "Sebagian: %d sukses, %d ditolak", acc, rej);
            } else if (rej > 0) {
              snprintf(ui_evt.extra, sizeof(ui_evt.extra),
                       "Gagal: %d data ditolak server!", rej);
            } else {
              snprintf(ui_evt.extra, sizeof(ui_evt.extra),
                       "Gagal terhubung ke backend!");
            }
            osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
          }
        }
      } else if (strncmp(uart1_rx_buf, "BULK_ERR:", 9) == 0) {
        char *msg = uart1_rx_buf + 9;
        size_t l = strlen(msg);
        while (l > 0 && (msg[l - 1] == '\r' || msg[l - 1] == '\n'))
          msg[--l] = '\0';
        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_BULK_SYNC_DONE;
        ui_evt.status = 0;
        snprintf(ui_evt.extra, sizeof(ui_evt.extra), "Gagal: %.40s", msg);
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
      } else if (strncmp(uart1_rx_buf, "REG_OK:", 7) == 0) {
        /* Balasan pendaftaran device berhasil dari server via ESP32 */
        char *msg = uart1_rx_buf + 7;
        size_t l = strlen(msg);
        while (l > 0 && (msg[l - 1] == '\r' || msg[l - 1] == '\n'))
          msg[--l] = '\0';
        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_DEVICE_REG_RESULT;
        ui_evt.status = 1;
        strncpy(ui_evt.extra, msg, sizeof(ui_evt.extra) - 1);
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
      } else if (strncmp(uart1_rx_buf, "REG_ERR:", 8) == 0) {
        /* Balasan pendaftaran device gagal dari server via ESP32 */
        char *msg = uart1_rx_buf + 8;
        size_t l = strlen(msg);
        while (l > 0 && (msg[l - 1] == '\r' || msg[l - 1] == '\n'))
          msg[--l] = '\0';
        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_DEVICE_REG_RESULT;
        ui_evt.status = 0;
        strncpy(ui_evt.extra, msg, sizeof(ui_evt.extra) - 1);
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
      } else if (strncmp(uart1_rx_buf, "GRANTED:", 8) == 0) {
        /* Format: GRANTED:<UID>|<NAMA>|<DEPARTEMEN> */
        char *data = uart1_rx_buf + 8;
        char *nl = strpbrk(data, "\r\n");
        if (nl)
          *nl = '\0';

        char *uid_card = strtok(data, "|");
        char *name_str = strtok(NULL, "|");
        char *pos_str = strtok(NULL, "|");

        /* 1. Bunyikan 2x beep pendek tanda verifikasi server sukses */
        HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                          GPIO_PIN_SET);
        HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_SET);
        osDelay(50);
        HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                          GPIO_PIN_RESET);
        HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);
        osDelay(50);
        HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                          GPIO_PIN_SET);
        HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_SET);
        osDelay(50);
        HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                          GPIO_PIN_RESET);
        HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

        /* 2. Cek apakah respon server mengembalikan nama karyawan yang sah */
        bool is_valid_name = (name_str != NULL && strlen(name_str) > 0 &&
                              strcmp(name_str, "-") != 0 &&
                              strcasecmp(name_str, "Akses Diterima") != 0 &&
                              strcasecmp(name_str, "Akses Diterima!") != 0 &&
                              strcasecmp(name_str, "UNKNOWN") != 0);

        if (is_valid_name) {
          /* Akses Sah: Tampilkan nama karyawan di layar */
          ui_event_t ui_evt = {0};
          ui_evt.type = UI_EVENT_ATTENDANCE_SUCCESS;
          ui_evt.status = 1;
          strncpy(ui_evt.extra, name_str, sizeof(ui_evt.extra) - 1);
          osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);

          /* 3. LAZY CACHING: Simpan pasangan UID + Nama ke memori lokal USERS.CSV */
          if (uid_card && strlen(uid_card) > 0) {
            while (*uid_card == ' ')
              uid_card++;
            while (*name_str == ' ')
              name_str++;
            if (pos_str) {
              while (*pos_str == ' ')
                pos_str++;
            }
            Storage_SaveEmployee(uid_card, name_str, pos_str);
          }
        } else {
          /* Kartu belum terdaftar / belum di-mapping di server */
          HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
          osDelay(250);
          HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);

          ui_event_t ui_evt = {0};
          ui_evt.type = UI_EVENT_ATTENDANCE_REJECTED;
          ui_evt.status = 0;
          strncpy(ui_evt.extra, "Belum Terdaftar", sizeof(ui_evt.extra) - 1);
          osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
        }

        /* 4. Tulis riwayat audit ke ATTEND.CSV */
        RTC_TimeTypeDef sTime = {0};
        RTC_DateTypeDef sDate = {0};
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        char time_iso[32];
        snprintf(time_iso, sizeof(time_iso), "20%02d-%02d-%02d %02d:%02d:%02d",
                 sDate.Year, sDate.Month, sDate.Date, sTime.Hours,
                 sTime.Minutes, sTime.Seconds);

        if (osMutexAcquire(sdioFatFsMutexHandle, 500) == osOK) {
          if (g_sd_mounted) {
            FIL attend_file;
            if (f_open(&attend_file, "ATTEND.CSV", FA_OPEN_ALWAYS | FA_WRITE) ==
                FR_OK) {
              f_lseek(&attend_file, f_size(&attend_file));
              UINT bw;
              char log_line[128];
              snprintf(log_line, sizeof(log_line), "%s,UID:%s,NAME=%.40s\r\n",
                       time_iso, uid_card ? uid_card : "-",
                       name_str ? name_str : "UNKNOWN");
              f_write(&attend_file, log_line, strlen(log_line), &bw);
              f_sync(&attend_file);
              f_close(&attend_file);
            }
          }
          osMutexRelease(sdioFatFsMutexHandle);
        }

      } else if (strncmp(uart1_rx_buf, "DENIED:", 7) == 0 ||
                 strncmp(uart1_rx_buf, "TXN_ERR:", 8) == 0) {
        /* Format: DENIED:<REASON> atau TXN_ERR:<TXN_UID>|<REASON> */
        char *data = NULL;
        if (strncmp(uart1_rx_buf, "DENIED:", 7) == 0) {
          data = uart1_rx_buf + 7;
        } else {
          char *p = uart1_rx_buf + 8;
          char *bar = strchr(p, '|');
          data = bar ? bar + 1 : p;
        }
        char *nl = strpbrk(data, "\r\n");
        if (nl)
          *nl = '\0';

        /* 1. Bunyikan 1x beep panjang tanda kartu ditolak */
        HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                          GPIO_PIN_SET);
        osDelay(250);
        HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin,
                          GPIO_PIN_RESET);

        /* 2. Tampilkan pesan penolakan dari API di layar */
        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_ATTENDANCE_REJECTED;
        ui_evt.status = 0;
        if (data != NULL && strlen(data) > 0) {
          strncpy(ui_evt.extra, data, sizeof(ui_evt.extra) - 1);
        } else {
          strncpy(ui_evt.extra, "Kartu Tidak Terdaftar",
                  sizeof(ui_evt.extra) - 1);
        }
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
      } else if (strncmp(uart1_rx_buf, "TIME:", 5) == 0) {
        /* Sinkronisasi Waktu NTP dari ESP32 */
        int h = 0, m = 0, s = 0, d = 0, mo = 0, y = 0;
        bool parsed = false;

        if (sscanf(uart1_rx_buf + 5, "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &m,
                   &s) == 6) {
          parsed = true;
        } else if (sscanf(uart1_rx_buf + 5, "%d-%d-%dT%d:%d:%d", &y, &mo, &d,
                          &h, &m, &s) == 6) {
          parsed = true;
        } else if (sscanf(uart1_rx_buf + 5, "%d:%d:%d-%d/%d/%d", &h, &m, &s, &d,
                          &mo, &y) == 6) {
          parsed = true;
        }

        if (parsed) {
          RTC_TimeTypeDef sTime = {0};
          RTC_DateTypeDef sDate = {0};
          sTime.Hours = (uint8_t)h;
          sTime.Minutes = (uint8_t)m;
          sTime.Seconds = (uint8_t)s;
          HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

          sDate.Date = (uint8_t)d;
          sDate.Month = (uint8_t)mo;
          sDate.Year = (uint8_t)(y >= 2000 ? y - 2000 : y);
          HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

          __HAL_RCC_PWR_CLK_ENABLE();
          HAL_PWR_EnableBkUpAccess();
          HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F2);

          ui_event_t ui_evt = {0};
          ui_evt.type = UI_EVENT_NTP_SYNCED;
          ui_evt.status = 1;
          snprintf(ui_evt.extra, sizeof(ui_evt.extra), "%02d:%02d:%02d", h, m,
                   s);
          osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
        }
      } else if (strncmp(uart1_rx_buf, "TIME_ERR:", 9) == 0) {
        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_NTP_FAILED;
        ui_evt.status = 0;
        char *err_str = uart1_rx_buf + 9;
        size_t l = strlen(err_str);
        while (l > 0 && (err_str[l - 1] == '\r' || err_str[l - 1] == '\n'))
          err_str[--l] = '\0';
        strncpy(ui_evt.extra, err_str, sizeof(ui_evt.extra) - 1);
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
      } else if (strncmp(uart1_rx_buf, "WIFI:OK", 7) == 0) {
        g_wifi_connected = true;
        strncpy(g_wifi_info.status, "CONNECTED", sizeof(g_wifi_info.status) - 1);
        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_WIFI_STATUS;
        ui_evt.status = 1;
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);

        esp_tx_item_t sync_cmd = {0};
        snprintf(sync_cmd.payload, sizeof(sync_cmd.payload), "CMD:SYNC_TIME\n");
        osMessageQueuePut(espTxQueueHandle, &sync_cmd, 0, 10);
      } else if (strncmp(uart1_rx_buf, "WIFI:FAIL", 9) == 0) {
        g_wifi_connected = false;
        strncpy(g_wifi_info.status, "DISCONNECTED", sizeof(g_wifi_info.status) - 1);
        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_WIFI_STATUS;
        ui_evt.status = 0;
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
      } else if (strncmp(uart1_rx_buf, "WIFI_SAVED", 10) == 0) {
        g_wifi_connected = false;
        strncpy(g_wifi_info.status, "CONNECTING", sizeof(g_wifi_info.status) - 1);
        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_WIFI_STATUS;
        ui_evt.status = 0;
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
      } else if (strncmp(uart1_rx_buf, "WIFI_INFO:", 10) == 0) {
        /* Format: WIFI_INFO:<STATUS>|<SSID>|<IP>|<RSSI>|<PROTO>|<MAC> */
        char *data = uart1_rx_buf + 10;
        char *nl = strpbrk(data, "\r\n");
        if (nl) *nl = '\0';

        char *st    = strtok(data, "|");
        char *ssid  = strtok(NULL, "|");
        char *ip    = strtok(NULL, "|");
        char *rssi  = strtok(NULL, "|");
        char *proto = strtok(NULL, "|");
        char *mac   = strtok(NULL, "|");

        if (st) strncpy(g_wifi_info.status, st, sizeof(g_wifi_info.status) - 1);
        if (ssid) strncpy(g_wifi_info.ssid, ssid, sizeof(g_wifi_info.ssid) - 1);
        if (ip) strncpy(g_wifi_info.ip, ip, sizeof(g_wifi_info.ip) - 1);
        if (rssi) strncpy(g_wifi_info.rssi, rssi, sizeof(g_wifi_info.rssi) - 1);
        if (proto) {
          strncpy(g_wifi_info.protocol, proto, sizeof(g_wifi_info.protocol) - 1);
          g_wifi_info.is_wifi6 = (strstr(proto, "Wi-Fi 6") != NULL || strstr(proto, "11ax") != NULL);
        }
        if (mac) strncpy(g_wifi_info.mac, mac, sizeof(g_wifi_info.mac) - 1);

        GUI_Attendance_UpdateWifiInfo(&g_wifi_info);
      }
    }

    osDelay(10);
  }
  /* USER CODE END StartEsp32Task */
}

/* USER CODE BEGIN Header_StartStorageTask */
/**
 * @brief Function implementing the Task_Storage thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartStorageTask */
void StartStorageTask(void *argument) {
  /* USER CODE BEGIN StartStorageTask */
  if (osMutexAcquire(sdioFatFsMutexHandle, 500) == osOK) {
    if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 0) == FR_OK) {
      g_sd_mounted = true;
    }
    osMutexRelease(sdioFatFsMutexHandle);
  }

  /* Sanitasi USERS.CSV untuk membersihkan entri invalid lama sebelum dimuat ke RAM */
  Storage_SanitizeUsersFile();

  /* Hangatkan RAM cache dari file USERS.CSV agar lookup kartu instan sejak
   * detik pertama boot */
  Storage_WarmupCache();

  uint32_t last_periodic_tick = HAL_GetTick();

  for (;;) {
    Watchdog_CheckIn(WDT_TASK_STORAGE);
    sd_log_item_t sd_item;
    if (osMessageQueueGet(sdLogQueueHandle, &sd_item, NULL, 500) == osOK) {
      char uid_hex[24] = {0};
      int off = 0;
      for (uint8_t i = 0;
           i < sd_item.uid_len && i < 10 && off < (int)sizeof(uid_hex) - 3;
           i++) {
        off += snprintf(uid_hex + off, sizeof(uid_hex) - off, "%02X",
                        sd_item.uid[i]);
      }
      if (off == 0)
        snprintf(uid_hex, sizeof(uid_hex), "UNKNOWN");

      char time_iso[32];
      snprintf(time_iso, sizeof(time_iso), "20%02d-%02d-%02d %02d:%02d:%02d",
               sd_item.year, sd_item.month, sd_item.date, sd_item.hours,
               sd_item.minutes, sd_item.seconds);

      char txn_uid[96];
      char dev_code_buf[32];
      if (g_app_settings.device_code[0] != '\0') {
        strncpy(dev_code_buf, g_app_settings.device_code,
                sizeof(dev_code_buf) - 1);
        dev_code_buf[sizeof(dev_code_buf) - 1] = '\0';
      } else {
        uint16_t dev_id =
            (uint16_t)((HAL_GetUIDw0() ^ HAL_GetUIDw1() ^ HAL_GetUIDw2()) &
                       0xFFFF);
        snprintf(dev_code_buf, sizeof(dev_code_buf), "GATE-%04X",
                 (unsigned int)dev_id);
      }
      snprintf(txn_uid, sizeof(txn_uid), "%.24s_%02d%02d%02d%02d%02d%02d_%.24s",
               dev_code_buf, sd_item.year, sd_item.month, sd_item.date,
               sd_item.hours, sd_item.minutes, sd_item.seconds, uid_hex);

      /* 1. Tulis ke ATTEND.CSV (Riwayat Audit Permanen) */
      if (osMutexAcquire(sdioFatFsMutexHandle, 500) == osOK) {
        if (g_sd_mounted) {
          FIL file;
          if (f_open(&file, "ATTEND.CSV", FA_OPEN_ALWAYS | FA_WRITE) == FR_OK) {
            f_lseek(&file, f_size(&file));
            UINT bw;
            char log_line[128];
            snprintf(log_line, sizeof(log_line), "%s,UID:%s,NAME=%.40s\r\n",
                     time_iso, uid_hex, sd_item.log_msg);
            f_write(&file, log_line, strlen(log_line), &bw);
            f_sync(&file);
            f_close(&file);
          }

          /* 2. Tulis ke PENDING.CSV (Antrean untuk sinkronisasi ke server) */
          if (f_open(&file, "PENDING.CSV", FA_OPEN_ALWAYS | FA_WRITE) ==
              FR_OK) {
            f_lseek(&file, f_size(&file));
            UINT bw;
            char pend_line[160];
            snprintf(pend_line, sizeof(pend_line),
                     "%.64s|%.20s|%.24s|rfid|tap\r\n", txn_uid, time_iso,
                     uid_hex);
            f_write(&file, pend_line, strlen(pend_line), &bw);
            f_sync(&file);
            f_close(&file);
          }
        }
        osMutexRelease(sdioFatFsMutexHandle);
      }

      /* Perbarui jumlah antrean di tab sinkronisasi GUI */
      GUI_Attendance_UpdatePendingCount(Storage_GetPendingCount());

      /* 3. Jika mode Realtime dan Wi-Fi terhubung, kirim segera ke ESP32 */
      if (g_app_settings.sync_mode == SYNC_MODE_REALTIME && g_wifi_connected) {
        esp_tx_item_t esp_item = {0};
        snprintf(esp_item.payload, sizeof(esp_item.payload),
                 "TXN:%.64s|%.20s|%.24s|rfid|tap\n", txn_uid, time_iso,
                 uid_hex);
        osMessageQueuePut(espTxQueueHandle, &esp_item, 0, 50);
      }
    }

    /* 4. Pemeriksaan permintaan sinkronisasi bulk (dari GUI atau timer
     * periodik) */
    uint32_t now = HAL_GetTick();
    uint32_t interval_ms = 0;
    switch (g_app_settings.sync_mode) {
    case SYNC_MODE_10_MIN:
      interval_ms = 10 * 60 * 1000;
      break;
    case SYNC_MODE_30_MIN:
      interval_ms = 30 * 60 * 1000;
      break;
    case SYNC_MODE_1_HOUR:
      interval_ms = 60 * 60 * 1000;
      break;
    case SYNC_MODE_1_DAY:
      interval_ms = 24 * 60 * 60 * 1000;
      break;
    default:
      interval_ms = 0;
      break; /* Realtime */
    }

    bool periodic_due =
        (interval_ms > 0 && (now - last_periodic_tick >= interval_ms));
    if (g_bulk_sync_requested || periodic_due) {
      if (periodic_due)
        last_periodic_tick = now;
      g_bulk_sync_requested = false;
      if (g_wifi_connected) {
        Storage_TriggerBulkSync();
      }
    }
  }
  /* USER CODE END StartStorageTask */
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM6 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
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
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
