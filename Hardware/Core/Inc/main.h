/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

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
#define LIGHT_SENSOR_Pin GPIO_PIN_5
#define LIGHT_SENSOR_GPIO_Port GPIOE
#define LIGHT_SENSOR_EXTI_IRQn EXTI9_5_IRQn
#define PXX7_Pin GPIO_PIN_6
#define PXX7_GPIO_Port GPIOE
#define PXX7_EXTI_IRQn EXTI9_5_IRQn
#define DEBUG_LED_Pin GPIO_PIN_13
#define DEBUG_LED_GPIO_Port GPIOC
#define READ_EN_Pin GPIO_PIN_12
#define READ_EN_GPIO_Port GPIOD
#define WRITE_EN_Pin GPIO_PIN_9
#define WRITE_EN_GPIO_Port GPIOC
#define INTA_Pin GPIO_PIN_11
#define INTA_GPIO_Port GPIOA
#define INTB_Pin GPIO_PIN_12
#define INTB_GPIO_Port GPIOA
#define PXX6_Pin GPIO_PIN_3
#define PXX6_GPIO_Port GPIOD
#define PXX6_EXTI_IRQn EXTI3_IRQn
#define PXX2_Pin GPIO_PIN_4
#define PXX2_GPIO_Port GPIOB
#define PXX2_EXTI_IRQn EXTI4_IRQn
#define PXX3_Pin GPIO_PIN_8
#define PXX3_GPIO_Port GPIOB
#define PXX3_EXTI_IRQn EXTI9_5_IRQn
#define PXX4_Pin GPIO_PIN_9
#define PXX4_GPIO_Port GPIOB
#define PXX4_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
