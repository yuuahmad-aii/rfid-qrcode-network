/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h" // Wajib untuk komunikasi USB
#include <stdio.h>		 // Untuk sprintf
#include <tm1638.h>
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
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

/* USER CODE BEGIN PV */
// Variabel Timer
uint32_t IC_Value1 = 0;
uint32_t IC_Value2 = 0;
uint32_t Difference = 0;
uint8_t Is_First_Captured = 0;

// Variabel Data
volatile uint32_t Total_Pulse = 0;		 // Total pulsa terbaca
float Frequency = 0.0;					 // Frekuensi sinyal (Hz)
float RPM = 0.0;						 // RPM
volatile uint32_t Last_Capture_Time = 0; // Waktu terakhir capture
float Max_RPM = 0.0f;
float Min_RPM = 0.0f;
uint8_t Display_Mode = 0; // 0:RPM, 1:Freq, 2:Count, 3:Max, 4:Min

// Buffer untuk pengiriman USB
uint8_t txBuffer[64];

// tampilan di tm1638
uint8_t buttons;
uint8_t last_buttons = 0;
uint32_t input_state = 0;
char display_str[9] = "        ";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

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
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
	// inisialisasi awal tm1638
	TM1638_Init();
	TM1638_DisplayString("123");
	HAL_Delay(3000);
	TM1638_Clear();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {

		// ====================================================================
		// 1. BACA INPUT GPIO -> TAMPILKAN KE LED TM1638
		// ====================================================================
		input_state = 0; // Reset nilai setiap perulangan

		// Baca masing-masing pin dan set bit yang bersesuaian pada input_state
		if (HAL_GPIO_ReadPin(INPUT_1_GPIO_Port, INPUT_1_Pin) == GPIO_PIN_SET)
			input_state |= (1 << 0);
		if (HAL_GPIO_ReadPin(INPUT_2_GPIO_Port, INPUT_2_Pin) == GPIO_PIN_SET)
			input_state |= (1 << 1);
		if (HAL_GPIO_ReadPin(INPUT_3_GPIO_Port, INPUT_3_Pin) == GPIO_PIN_SET)
			input_state |= (1 << 2);
		if (HAL_GPIO_ReadPin(INPUT_4_GPIO_Port, INPUT_4_Pin) == GPIO_PIN_SET)
			input_state |= (1 << 3);
		if (HAL_GPIO_ReadPin(INPUT_5_GPIO_Port, INPUT_5_Pin) == GPIO_PIN_SET)
			input_state |= (1 << 4);
		if (HAL_GPIO_ReadPin(INPUT_6_GPIO_Port, INPUT_6_Pin) == GPIO_PIN_SET)
			input_state |= (1 << 5);
		if (HAL_GPIO_ReadPin(INPUT_7_GPIO_Port, INPUT_7_Pin) == GPIO_PIN_SET)
			input_state |= (1 << 6);
		if (HAL_GPIO_ReadPin(INPUT_8_GPIO_Port, INPUT_8_Pin) == GPIO_PIN_SET)
			input_state |= (1 << 7);

		// 1. Baca tombol (Blocking sebentar, tapi sangat cepat)
		buttons = TM1638_ReadButtons();

		// ====================================================================
		// 2. BACA TOMBOL TM1638 -> KONTROL OUTPUT GPIO (MODE TOGGLE)
		// ====================================================================
		buttons = TM1638_ReadButtons();

		if (buttons != last_buttons) {
			// Deteksi "Rising Edge": Cari bit yang bernilai 1 di 'buttons' tapi 0 di 'last_buttons'
			// Ini mencegah output berkedip cepat saat tombol ditahan
			uint8_t pressed = buttons & ~last_buttons;

			// Jika tombol ditekan, Toggle (balikkan state) pin output yang bersesuaian
			if (pressed & (1 << 0))
				HAL_GPIO_TogglePin(OUTPUT_1_GPIO_Port, OUTPUT_1_Pin);
			if (pressed & (1 << 1))
				HAL_GPIO_TogglePin(OUTPUT_2_GPIO_Port, OUTPUT_2_Pin);
			if (pressed & (1 << 2))
				HAL_GPIO_TogglePin(OUTPUT_3_GPIO_Port, OUTPUT_3_Pin);
			if (pressed & (1 << 3))
				HAL_GPIO_TogglePin(OUTPUT_4_GPIO_Port, OUTPUT_4_Pin);
			if (pressed & (1 << 4))
				HAL_GPIO_TogglePin(OUTPUT_5_GPIO_Port, OUTPUT_5_Pin);
			if (pressed & (1 << 5))
				HAL_GPIO_TogglePin(OUTPUT_6_GPIO_Port, OUTPUT_6_Pin);
			if (pressed & (1 << 6))
				HAL_GPIO_TogglePin(OUTPUT_7_GPIO_Port, OUTPUT_7_Pin);
			if (pressed & (1 << 7))
				HAL_GPIO_TogglePin(OUTPUT_8_GPIO_Port, OUTPUT_8_Pin);

			last_buttons = buttons;
		}

		// 3. Kirim ke Display pakai DMA
		// Fungsi ini hanya memakan waktu ~1-2 us untuk setup DMA,
		// sisanya dikerjakan hardware di background.
		TM1638_SendDMA(display_str, input_state);

		// 1. Format data menjadi String
		int len = sprintf((char*) txBuffer, "button_state: %ld\r\n", input_state);

		// 2. Kirim via USB CDC
		CDC_Transmit_FS(txBuffer, len);

		// 3. Delay agar tidak membanjiri buffer USB PC
		HAL_Delay(100);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
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
  hspi1.Init.Direction = SPI_DIRECTION_1LINE;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_LSB;
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OUTPUT_8_GPIO_Port, OUTPUT_8_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, USER_LED_Pin|OUTPUT_1_Pin|OUTPUT_2_Pin|OUTPUT_3_Pin
                          |OUTPUT_4_Pin|OUTPUT_5_Pin|OUTPUT_6_Pin|OUTPUT_7_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : OUTPUT_8_Pin */
  GPIO_InitStruct.Pin = OUTPUT_8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OUTPUT_8_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USER_BTN_Pin INPUT_5_Pin INPUT_6_Pin INPUT_7_Pin
                           INPUT_8_Pin */
  GPIO_InitStruct.Pin = USER_BTN_Pin|INPUT_5_Pin|INPUT_6_Pin|INPUT_7_Pin
                          |INPUT_8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USER_LED_Pin OUTPUT_1_Pin OUTPUT_2_Pin OUTPUT_3_Pin
                           OUTPUT_4_Pin OUTPUT_5_Pin OUTPUT_6_Pin OUTPUT_7_Pin */
  GPIO_InitStruct.Pin = USER_LED_Pin|OUTPUT_1_Pin|OUTPUT_2_Pin|OUTPUT_3_Pin
                          |OUTPUT_4_Pin|OUTPUT_5_Pin|OUTPUT_6_Pin|OUTPUT_7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : INPUT_1_Pin INPUT_2_Pin INPUT_3_Pin INPUT_4_Pin */
  GPIO_InitStruct.Pin = INPUT_1_Pin|INPUT_2_Pin|INPUT_3_Pin|INPUT_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
