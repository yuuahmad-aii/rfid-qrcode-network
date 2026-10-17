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
#include "task.h"
#include "ili9488.h"
#include "xpt2046.h"
#include "clrc663.h"
#include "fonts.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "lvgl.h"
#include "gui_attendance.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct {
    uint32_t timestamp;       /* Unix or custom timestamp (4 bytes) */
    uint8_t  year;            /* Year - 2000 (1 byte) */
    uint8_t  month;           /* Month 1-12 (1 byte) */
    uint8_t  date;            /* Day of month 1-31 (1 byte) */
    uint8_t  hours;           /* Hours 0-23 (1 byte) */
    uint8_t  minutes;         /* Minutes 0-59 (1 byte) */
    uint8_t  seconds;         /* Seconds 0-59 (1 byte) */
    uint8_t  uid_len;         /* Length of UID (1 byte) */
    uint8_t  uid[10];         /* Card UID (10 bytes) */
    char     log_msg[44];     /* Text log details or remark (44 bytes) */
} sd_log_item_t;              /* Exactly 64 bytes (matches sdLogQueue item size) */

#ifndef ESP_TX_ITEM_DEFINED
#define ESP_TX_ITEM_DEFINED
typedef struct {
    char payload[64];         /* Formatted string packet to ESP32 (64 bytes) */
} esp_tx_item_t;              /* Exactly 64 bytes (matches espTxQueue item size) */
#endif
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Color Palette for Modern Glassmorphism UI */
#define COLOR_BG            0x0821   /* Dark Charcoal / Deep Navy Background */
#define COLOR_HEADER_BG     0x10A2   /* Slate Header Background */
#define COLOR_PANEL_BG      0x1082   /* Dark Slate Card Container */
#define COLOR_BORDER_CYAN   0x05BF   /* Glowing Neon Cyan Border */
#define COLOR_POPUP_BG      0x0862   /* Deep Indigo Pop-up Modal */
#define COLOR_ACCENT_GREEN  0x07E0   /* Emerald Green Status */
#define COLOR_ACCENT_RED    0xF800   /* Vivid Coral Red Error */
#define COLOR_ACCENT_YELLOW 0xFFE0   /* Warm Amber Clock / Highlights */
#define COLOR_TEXT_WHITE    0xFFFF   /* Crisp White */
#define COLOR_TEXT_MUTED    0x8410   /* Medium Muted Grey */
#define COLOR_BTN_BG        0x1904   /* Slate Button Fill */
#define COLOR_BTN_BORDER    0x07FF   /* Cyan Button Outline */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

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
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for Task_GUI */
osThreadId_t Task_GUIHandle;
const osThreadAttr_t Task_GUI_attributes = {
  .name = "Task_GUI",
  .stack_size = 1536 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for Task_ESP32 */
osThreadId_t Task_ESP32Handle;
const osThreadAttr_t Task_ESP32_attributes = {
  .name = "Task_ESP32",
  .stack_size = 768 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Storage */
osThreadId_t Task_StorageHandle;
const osThreadAttr_t Task_Storage_attributes = {
  .name = "Task_Storage",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for uiEventQueue */
osMessageQueueId_t uiEventQueueHandle;
const osMessageQueueAttr_t uiEventQueue_attributes = {
  .name = "uiEventQueue"
};
/* Definitions for sdLogQueue */
osMessageQueueId_t sdLogQueueHandle;
const osMessageQueueAttr_t sdLogQueue_attributes = {
  .name = "sdLogQueue"
};
/* Definitions for espTxQueue */
osMessageQueueId_t espTxQueueHandle;
const osMessageQueueAttr_t espTxQueue_attributes = {
  .name = "espTxQueue"
};
/* Definitions for sdioFatFsMutex */
osMutexId_t sdioFatFsMutexHandle;
const osMutexAttr_t sdioFatFsMutex_attributes = {
  .name = "sdioFatFsMutex"
};
/* Definitions for spi1TxDmaSem */
osSemaphoreId_t spi1TxDmaSemHandle;
const osSemaphoreAttr_t spi1TxDmaSem_attributes = {
  .name = "spi1TxDmaSem"
};
/* USER CODE BEGIN PV */
/* Mutex for protecting SPI2 bus between Touch & other SPI tasks */
osMutexId_t spi2MutexHandle;
const osMutexAttr_t spi2Mutex_attributes = {
  .name = "spi2Mutex"
};

/* Hardware & System Status Flags */
volatile bool g_clrc663_ready = false;
volatile uint8_t g_clrc663_version = 0;
volatile bool g_sd_mounted = false;
volatile bool g_wifi_connected = false;
volatile uint32_t g_attendance_total = 0;

/* RFID duplicate filter */
static uint8_t  g_prev_uid[10] = {0};
static uint8_t  g_prev_uid_len = 0;
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
void StartRfidTask(void *argument);
void StartGuiTask(void *argument);
void StartEsp32Task(void *argument);
void StartStorageTask(void *argument);

/* USER CODE BEGIN PFP */
static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_RTC_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  ILI9488_Init();
  XPT2046_Init();
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
  spi1TxDmaSemHandle = osSemaphoreNew(1, 0, &spi1TxDmaSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of uiEventQueue */
  uiEventQueueHandle = osMessageQueueNew (8, 32, &uiEventQueue_attributes);

  /* creation of sdLogQueue */
  sdLogQueueHandle = osMessageQueueNew (16, 64, &sdLogQueue_attributes);

  /* creation of espTxQueue */
  espTxQueueHandle = osMessageQueueNew (16, 64, &espTxQueue_attributes);

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
  Task_StorageHandle = osThreadNew(StartStorageTask, NULL, &Task_Storage_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  HAL_GPIO_WritePin(GPIOA, OUTPUT_BUZZER_Pin|SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, SPI1_RST_Pin|SPI1_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, USER_LED_Pin|SPI2_CS_Pin|SPI3_CS_Pin|SPI3_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : OUTPUT_BUZZER_Pin SPI1_CS_Pin */
  GPIO_InitStruct.Pin = OUTPUT_BUZZER_Pin|SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI1_RST_Pin SPI1_DC_Pin */
  GPIO_InitStruct.Pin = SPI1_RST_Pin|SPI1_DC_Pin;
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
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  /* RFID IRQ: PA15 (SPI3_IRQ) atau PA8 */
  if (GPIO_Pin == SPI3_IRQ_Pin || GPIO_Pin == GPIO_PIN_8)
  {
    if (Task_RFIDHandle != NULL)
    {
      vTaskNotifyGiveFromISR((TaskHandle_t)Task_RFIDHandle, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  }
  /* Touch IRQ: PC6 (SPI2_IRQ) atau PB1 */
  else if (GPIO_Pin == SPI2_IRQ_Pin || GPIO_Pin == GPIO_PIN_1)
  {
    if (Task_GUIHandle != NULL)
    {
      vTaskNotifyGiveFromISR((TaskHandle_t)Task_GUIHandle, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  }
}

/**
  * @brief  UART Rx Complete Callback for USART1 ESP32 communication
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    if (uart1_rx_data == '\n' || uart1_rx_data == '\r')
    {
      uart1_rx_buf[uart1_rx_idx] = '\0';
      if (uart1_rx_idx > 0)
      {
        uart1_rx_ready = 1;
      }
      uart1_rx_idx = 0;
    }
    else
    {
      if (uart1_rx_idx < sizeof(uart1_rx_buf) - 1)
      {
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
static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  ILI9488_DrawBitmapLVGL(area->x1, area->y1, area->x2, area->y2, px_map);
  lv_display_flush_ready(disp);
}

/**
  * @brief  LVGL Touch input callback reading from XPT2046 with mutex protection
  */
static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  uint16_t touch_x = 0, touch_y = 0;
  if (osMutexAcquire(spi2MutexHandle, 5) == osOK)
  {
    if (XPT2046_GetTouch(&touch_x, &touch_y))
    {
      data->state = LV_INDEV_STATE_PRESSED;
      data->point.x = touch_x;
      data->point.y = touch_y;
      GUI_Attendance_CheckCornerTouch(touch_x, touch_y);
    }
    else
    {
      data->state = LV_INDEV_STATE_RELEASED;
      GUI_Attendance_CheckCornerTouch(0, 0);
    }
    osMutexRelease(spi2MutexHandle);
  }
  else
  {
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
void StartRfidTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* 1. Inisialisasi Hardware RFID Reader NXP CLRC66303 */
  osMutexAcquire(spi2MutexHandle, osWaitForever);
  if (CLRC663_Init(&hspi3) == CLRC663_OK)
  {
    g_clrc663_ready = true;
    g_clrc663_version = CLRC663_ReadVersion();
    CLRC663_AntennaOn();
    CLRC663_SetupISO14443A();
  }
  else
  {
    g_clrc663_ready = false;
    g_clrc663_version = CLRC663_ReadVersion();
  }
  osMutexRelease(spi2MutexHandle);

  /* Infinite loop */
  for(;;)
  {
    /* Tidur menunggu Task Notification dari RFID_IRQ (PA15/EXTI15_10 atau PA8).
       Diberi timeout 50 ms sebagai fallback polling agar pembacaan kartu tetap responsif. */
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));

    /* Kunci spi2Mutex sebelum mengakses reader kartu */
    if (osMutexAcquire(spi2MutexHandle, 20) == osOK)
    {
      if (!g_clrc663_ready)
      {
        if (CLRC663_Init(&hspi3) == CLRC663_OK)
        {
          g_clrc663_ready = true;
          CLRC663_AntennaOn();
          CLRC663_SetupISO14443A();
        }
      }

      if (g_clrc663_ready)
      {
        CLRC663_Card_t card;
        if (CLRC663_ReadCard(&card))
        {
          /* Debounce kartu yang sama dalam durasi 1500 ms */
          uint32_t now = HAL_GetTick();
          bool is_duplicate = false;
          if (card.uid_len == g_prev_uid_len && (now - g_prev_uid_tick < 1500))
          {
            if (memcmp(card.uid, g_prev_uid, card.uid_len) == 0)
            {
              is_duplicate = true;
            }
          }

          if (!is_duplicate)
          {
            /* Simpan UID untuk debounce */
            memcpy(g_prev_uid, card.uid, card.uid_len);
            g_prev_uid_len = card.uid_len;
            g_prev_uid_tick = now;

            /* Ambil Waktu & Tanggal dari RTC internal */
            RTC_TimeTypeDef sTime = {0};
            RTC_DateTypeDef sDate = {0};
            HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

            /* Bunyikan BUZZER dan nyalakan USER LED singkat (75 ms) */
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_SET);
            osDelay(75);
            HAL_GPIO_WritePin(OUTPUT_BUZZER_GPIO_Port, OUTPUT_BUZZER_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

            /* Kirim event deteksi kartu ke uiEventQueue untuk ditampilkan Task_GUI */
            ui_event_t ui_evt = {0};
            ui_evt.type = UI_EVENT_RFID_CARD_DETECTED;
            ui_evt.status = 1;
            ui_evt.uid_len = card.uid_len;
            memcpy(ui_evt.uid, card.uid, card.uid_len);
            snprintf(ui_evt.extra, sizeof(ui_evt.extra), "%02d:%02d:%02d WIB",
                     sTime.Hours, sTime.Minutes, sTime.Seconds);
            osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);

            /* Kirim data absensi ke sdLogQueue untuk ditulis ke MicroSD oleh Task_Storage */
            sd_log_item_t sd_item = {0};
            sd_item.year = sDate.Year;
            sd_item.month = sDate.Month;
            sd_item.date = sDate.Date;
            sd_item.hours = sTime.Hours;
            sd_item.minutes = sTime.Minutes;
            sd_item.seconds = sTime.Seconds;
            sd_item.uid_len = card.uid_len;
            memcpy(sd_item.uid, card.uid, card.uid_len);
            snprintf(sd_item.log_msg, sizeof(sd_item.log_msg), "CARD_TAP_OK");
            osMessageQueuePut(sdLogQueueHandle, &sd_item, 0, 10);

            /* Kirim permintaan verifikasi ke ESP32 via espTxQueue */
            esp_tx_item_t esp_item = {0};
            if (card.uid_len >= 4) {
              snprintf(esp_item.payload, sizeof(esp_item.payload), "RFID:%02X%02X%02X%02X\n",
                       card.uid[0], card.uid[1], card.uid[2], card.uid[3]);
            } else {
              snprintf(esp_item.payload, sizeof(esp_item.payload), "RFID:UNKNOWN\n");
            }
            osMessageQueuePut(espTxQueueHandle, &esp_item, 0, 10);
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
void StartGuiTask(void *argument)
{
  /* USER CODE BEGIN StartGuiTask */
  /* 1. Nyalakan Backlight Layar LCD via PWM TIM3 CH3 */
  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 50000); /* Kecerahan ~75% */

  /* 2. Inisialisasi Core LVGL */
  lv_init();
  lv_tick_set_cb(HAL_GetTick);

  /* 3. Setup Display LVGL untuk ILI9488 (RGB888) */
  lv_display_t *disp = lv_display_create(ILI9488_WIDTH, ILI9488_HEIGHT);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);
  lv_display_set_flush_cb(disp, my_disp_flush);

  /* Buffer render parsial LVGL (16 baris @ 480 px = 23040 byte) */
  #define DRAW_BUF_SIZE (ILI9488_WIDTH * 16 * 3)
  static uint8_t lv_draw_buf[DRAW_BUF_SIZE] __attribute__((aligned(64)));
  lv_display_set_buffers(disp, lv_draw_buf, NULL, sizeof(lv_draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

  /* 4. Setup Input Device Touchscreen (XPT2046) */
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  /* 5. Inisialisasi Antarmuka GUI Absensi */
  GUI_Attendance_Init();

  uint32_t last_clock_tick = 0;

  /* Infinite loop GUI Task */
  for(;;)
  {
    /* 1. Periksa event masuk dari uiEventQueue (misal dari Task_RFID, Task_ESP32) */
    ui_event_t evt;
    while (osMessageQueueGet(uiEventQueueHandle, &evt, NULL, 0) == osOK)
    {
      GUI_Attendance_ProcessEvent(&evt);
    }

    /* 2. Perbarui tampilan jam RTC dan status peripheral setiap 500 ms */
    uint32_t now = HAL_GetTick();
    if (now - last_clock_tick >= 500)
    {
      last_clock_tick = now;
      RTC_TimeTypeDef sTime = {0};
      RTC_DateTypeDef sDate = {0};
      HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
      HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
      GUI_Attendance_UpdateClock(&sTime, &sDate);
      GUI_Attendance_UpdateBadges(g_clrc663_ready, g_wifi_connected, g_sd_mounted);
    }

    /* 3. Jalankan LVGL timer handler */
    lv_timer_handler();

    /* 4. Yield task ke scheduler FreeRTOS */
    osDelay(5);
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
void StartEsp32Task(void *argument)
{
  /* USER CODE BEGIN StartEsp32Task */
  /* Aktifkan penerimaan UART1 interrupt dari ESP32-C6 */
  HAL_UART_Receive_IT(&huart1, &uart1_rx_data, 1);

  /* Beri jeda sejenak lalu minta status koneksi Wi-Fi awal ke ESP32 */
  osDelay(300);
  const char *cmd_wifi_status = "CMD:GET_WIFI_STATUS\n";
  HAL_UART_Transmit(&huart1, (uint8_t*)cmd_wifi_status, strlen(cmd_wifi_status), 100);

  for(;;)
  {
    /* 1. Ambil data dari espTxQueue jika ada */
    esp_tx_item_t tx_item;
    if (osMessageQueueGet(espTxQueueHandle, &tx_item, NULL, 30) == osOK)
    {
      /* Kirim ke ESP32-C6 via USART1 */
      HAL_UART_Transmit(&huart1, (uint8_t*)tx_item.payload, strlen(tx_item.payload), 100);
    }

    /* 2. Proses pesan balasan serial dari ESP32-C6 (ACK / NTP / Verifikasi Nama) */
    if (uart1_rx_ready)
    {
      uart1_rx_ready = 0;

      if (strncmp(uart1_rx_buf, "GRANTED:", 8) == 0)
      {
        /* Format: GRANTED:<UID>|<NAMA>|<DEPARTEMEN> */
        char *data = uart1_rx_buf + 8;
        char *uid_str = strtok(data, "|");
        (void)uid_str; /* Suppress unused variable warning */
        char *name_str = strtok(NULL, "|");

        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_ATTENDANCE_SUCCESS;
        ui_evt.status = 1;
        if (name_str != NULL) {
          strncpy(ui_evt.extra, name_str, sizeof(ui_evt.extra) - 1);
        } else {
          strncpy(ui_evt.extra, "Akses Diterima", sizeof(ui_evt.extra) - 1);
        }
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
      }
      else if (strncmp(uart1_rx_buf, "DENIED:", 7) == 0)
      {
        /* Format: DENIED:<ALASAN> */
        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_ATTENDANCE_REJECTED;
        ui_evt.status = 0;
        strncpy(ui_evt.extra, uart1_rx_buf + 7, sizeof(ui_evt.extra) - 1);
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
      }
      else if (strncmp(uart1_rx_buf, "TIME:", 5) == 0)
      {
        /* Sinkronisasi Waktu NTP dari ESP32: TIME:HH:MM:SS-DD/MM/YYYY */
        int h = 0, m = 0, s = 0, d = 0, mo = 0, y = 0;
        if (sscanf(uart1_rx_buf + 5, "%d:%d:%d-%d/%d/%d", &h, &m, &s, &d, &mo, &y) == 6)
        {
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

          ui_event_t ui_evt = {0};
          ui_evt.type = UI_EVENT_NTP_SYNCED;
          osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
        }
      }
      else if (strncmp(uart1_rx_buf, "WIFI:OK", 7) == 0)
      {
        g_wifi_connected = true;
        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_WIFI_STATUS;
        ui_evt.status = 1;
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
      }
      else if (strncmp(uart1_rx_buf, "WIFI:FAIL", 9) == 0)
      {
        g_wifi_connected = false;
        ui_event_t ui_evt = {0};
        ui_evt.type = UI_EVENT_WIFI_STATUS;
        ui_evt.status = 0;
        osMessageQueuePut(uiEventQueueHandle, &ui_evt, 0, 10);
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
void StartStorageTask(void *argument)
{
  /* USER CODE BEGIN StartStorageTask */
  /* Coba mount MicroSD saat startup agar status badge langsung OK jika kartu terpasang */
  if (osMutexAcquire(sdioFatFsMutexHandle, 500) == osOK)
  {
    if (f_mount(&SDFatFS, (TCHAR const*)SDPath, 1) == FR_OK)
    {
      g_sd_mounted = true;
    }
    osMutexRelease(sdioFatFsMutexHandle);
  }

  /* Tunggu data masuk dari sdLogQueue */
  for(;;)
  {
    sd_log_item_t sd_item;
    if (osMessageQueueGet(sdLogQueueHandle, &sd_item, NULL, osWaitForever) == osOK)
    {
      /* Kunci sdioFatFsMutex untuk melindungi akses file system MicroSD */
      if (osMutexAcquire(sdioFatFsMutexHandle, 200) == osOK)
      {
        if (!g_sd_mounted)
        {
          if (f_mount(&SDFatFS, (TCHAR const*)SDPath, 1) == FR_OK)
          {
            g_sd_mounted = true;
          }
        }

        if (g_sd_mounted)
        {
          /* Buka file log di MicroSD via FatFS (f_open, f_write, f_sync) */
          if (f_open(&SDFile, "ATTEND.CSV", FA_OPEN_ALWAYS | FA_WRITE) == FR_OK)
          {
            f_lseek(&SDFile, f_size(&SDFile));
            UINT bw;
            char log_line[96];
            snprintf(log_line, sizeof(log_line),
                     "20%02d-%02d-%02d %02d:%02d:%02d,UID:%02X%02X%02X%02X,LOG=%s\r\n",
                     sd_item.year, sd_item.month, sd_item.date,
                     sd_item.hours, sd_item.minutes, sd_item.seconds,
                     sd_item.uid[0], sd_item.uid[1], sd_item.uid[2], sd_item.uid[3],
                     sd_item.log_msg);
            f_write(&SDFile, log_line, strlen(log_line), &bw);
            f_sync(&SDFile);
            f_close(&SDFile);
          }
          else
          {
            /* Jika gagal membuka file, asumsikan SD dicabut atau error */
            g_sd_mounted = false;
          }
        }
        osMutexRelease(sdioFatFsMutexHandle);
      }

      /* Meneruskan payload absensi menuju espTxQueue untuk dikirim via Wi-Fi oleh Task_ESP32 */
      esp_tx_item_t esp_item = {0};
      snprintf(esp_item.payload, sizeof(esp_item.payload),
               "ATTEND:20%02d-%02d-%02d %02d:%02d:%02d|UID=%02X%02X%02X%02X\n",
               sd_item.year, sd_item.month, sd_item.date,
               sd_item.hours, sd_item.minutes, sd_item.seconds,
               sd_item.uid[0], sd_item.uid[1], sd_item.uid[2], sd_item.uid[3]);
      osMessageQueuePut(espTxQueueHandle, &esp_item, 0, 50);
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
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
