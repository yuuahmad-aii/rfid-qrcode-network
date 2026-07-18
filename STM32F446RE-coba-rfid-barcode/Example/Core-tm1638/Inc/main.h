/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define OUTPUT_8_Pin GPIO_PIN_13
#define OUTPUT_8_GPIO_Port GPIOC
#define USER_BTN_Pin GPIO_PIN_0
#define USER_BTN_GPIO_Port GPIOA
#define SPI1_CS_Pin GPIO_PIN_4
#define SPI1_CS_GPIO_Port GPIOA
#define USER_LED_Pin GPIO_PIN_2
#define USER_LED_GPIO_Port GPIOB
#define INPUT_1_Pin GPIO_PIN_12
#define INPUT_1_GPIO_Port GPIOB
#define INPUT_2_Pin GPIO_PIN_13
#define INPUT_2_GPIO_Port GPIOB
#define INPUT_3_Pin GPIO_PIN_14
#define INPUT_3_GPIO_Port GPIOB
#define INPUT_4_Pin GPIO_PIN_15
#define INPUT_4_GPIO_Port GPIOB
#define INPUT_5_Pin GPIO_PIN_8
#define INPUT_5_GPIO_Port GPIOA
#define INPUT_6_Pin GPIO_PIN_9
#define INPUT_6_GPIO_Port GPIOA
#define INPUT_7_Pin GPIO_PIN_10
#define INPUT_7_GPIO_Port GPIOA
#define INPUT_8_Pin GPIO_PIN_15
#define INPUT_8_GPIO_Port GPIOA
#define OUTPUT_1_Pin GPIO_PIN_3
#define OUTPUT_1_GPIO_Port GPIOB
#define OUTPUT_2_Pin GPIO_PIN_4
#define OUTPUT_2_GPIO_Port GPIOB
#define OUTPUT_3_Pin GPIO_PIN_5
#define OUTPUT_3_GPIO_Port GPIOB
#define OUTPUT_4_Pin GPIO_PIN_6
#define OUTPUT_4_GPIO_Port GPIOB
#define OUTPUT_5_Pin GPIO_PIN_7
#define OUTPUT_5_GPIO_Port GPIOB
#define OUTPUT_6_Pin GPIO_PIN_8
#define OUTPUT_6_GPIO_Port GPIOB
#define OUTPUT_7_Pin GPIO_PIN_9
#define OUTPUT_7_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
