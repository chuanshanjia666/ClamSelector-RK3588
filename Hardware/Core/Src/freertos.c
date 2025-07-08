/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dc_motor.h"
#include "tim.h"
#include "usart.h"
#include "rs485.h"
#include <stdio.h>
#include <string.h>
#include "can.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
uint8_t buff[30]; // Buffer for printf output
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin)
	{
	case LIGHT_SENSOR_Pin:
		HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin);
		printf("Light sensor triggered\r\n"); // Print message when light sensor is triggered
		/* code */
		break;
	// case BUTTON_Pin:
	// 	/* code */
	// 	break;
	default:
		break;
	}
}

uint8_t flag = 0; // Flag to indicate if data has been received

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for rs485mutex */
osMutexId_t rs485mutexHandle;
const osMutexAttr_t rs485mutex_attributes = {
  .name = "rs485mutex"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Pos)
{
	if (huart->Instance == USART1)
		if (HAL_UARTEx_GetRxEventType(huart) == HAL_UART_RXEVENT_IDLE)
		{
			flag++;
			memset(buff, 0, sizeof(buff));
			HAL_UARTEx_ReceiveToIdle_IT(huart, buff, sizeof(buff));
		}
}
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */

   printf("Stack overflow in task %s\r\n", pcTaskName);

   while (1)
   {
	/* code */
   }
   
}
/* USER CODE END 4 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of rs485mutex */
  rs485mutexHandle = osMutexNew(&rs485mutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
	/* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	DC_Monitor_TypeDef h_dc_motor;
	DC_Motor_Init(&h_dc_motor, &htim1, &htim3, CHANNEL_1, 1000);											  // Initialize DC motor with TIM3 and TIM4

	
	// Initialize UART for communication
	RS485_Init(&huart1, &rs485mutexHandle, WRITE_EN_GPIO_Port, READ_EN_GPIO_Port, WRITE_EN_Pin, READ_EN_Pin); // Initialize RS485 communication
	RS485_WriteOn();
	osDelay(10);
	RS485_ReadOn(buff, sizeof(buff)); // Enable RS485 write and read

	osDelay(10);
	HAL_UARTEx_ReceiveToIdle_IT(&huart1, buff, sizeof(buff));
	CAN_TxHeaderTypeDef cantxheader;

	CAN_TxHeaderTypeDef txHeader;
	uint8_t txData[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	uint32_t txMailbox;

	txHeader.StdId = 0x123;		 // 标准帧ID
	txHeader.ExtId = 0x00;		 // 扩展帧ID（不用时可不管）
	txHeader.RTR = CAN_RTR_DATA; // 数据帧
	txHeader.IDE = CAN_ID_STD;	 // 标准帧
	txHeader.DLC = 8;			 // 数据长度
	txHeader.TransmitGlobalTime = DISABLE;
	HAL_CAN_Start(&hcan2);
	HAL_CAN_ActivateNotification(&hcan2, CAN_IT_TX_MAILBOX_EMPTY);

	// DC_Motor_SetSpeed(&h_dc_motor, 300); // Set DC motor speed to 500
	// DC_Motor_Start(&h_dc_motor); // Start the DC motor
	// DC_Motor_SetSpeed(&h_dc_motor, 300);
	/* Infinite loop */
	int i = 0;
	for (;;)
	{
		printf("%d\r\n", flag); // Print the current speed of the DC motor
		// printf("%d\r\n", h_dc_motor.encode_num); // Print the encoder number
		// HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin); // Toggle LED state
		HAL_CAN_AddTxMessage(&hcan2, &txHeader, txData, &txMailbox);
		osDelay(1000); // Delay for 100 ms
	}
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
	printf("Mailbox 0 transmission complete\r\n");
}
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
	printf("Mailbox 1 transmission complete\r\n");
	// 邮箱1发送完成
	// 可以在这里处理发送完成后的逻辑
	// 例如，设置一个标志位或发送下一个数据包
	// flag = 1; // 设置标志位，表示数据已发送
}
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
	printf("Mailbox 2 transmission complete\r\n");
	// 邮箱2发送完成
	// 可以在这里处理发送完成后的逻辑
	// 例如，设置一个标志位或发送下一个数据包
	// flag = 1; // 设置标志位，表示数据已发送
}

HAL_StatusTypeDef can2_send_msg(uint32_t id, uint8_t type, uint8_t len, uint8_t *msg)
{
  CAN_TxHeaderTypeDef g_can2_txheader; /* 发送参数句柄 */
  uint16_t t = 0;
  uint32_t TxMailbox = CAN_TX_MAILBOX0;

  g_can2_txheader.StdId = id;         /* 标准标识符 */
  g_can2_txheader.IDE = CAN_ID_STD;   /* 使用标准帧 */
  g_can2_txheader.RTR = type; /* 数据帧 */
  g_can2_txheader.DLC = len;

  while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) < 1); /* 等待发送邮箱有空闲 */

  if (HAL_CAN_AddTxMessage(&hcan2, &g_can2_txheader, msg, &TxMailbox) != HAL_OK) /* 发送消息 */
  {
    return HAL_ERROR;
  }
  while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) != 3) /* 等待发送完成,所有邮箱为空 */
  {
    t++;

    if (t > 0xFFF)
    {
      HAL_CAN_AbortTxRequest(&hcan2, TxMailbox); /* 超时，直接中止邮箱的发送请求 */
      return HAL_TIMEOUT; /* 超时 */
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef can2_receive_msg(uint32_t id, uint8_t FIFO_id, uint8_t type, uint8_t *buf)
{
  CAN_RxHeaderTypeDef g_can2_rxheader; /* 接收参数句柄 */
  if (HAL_CAN_GetRxFifoFillLevel(&hcan2, FIFO_id) == 0) /* 没有接收到数据 */
  {
    return HAL_ERROR;
  }
  if (HAL_CAN_GetRxMessage(&hcan2, FIFO_id, &g_can2_rxheader, buf) != HAL_OK) /* 读取数据 */
  {
    return HAL_ERROR;
  }
  if (g_can2_rxheader.StdId != id || g_can2_rxheader.IDE != CAN_ID_STD 
    || g_can2_rxheader.RTR != type) /* 接收到的ID不对 / 不是标准帧 / 不是type类型 （软件筛选）*/
  {
    return HAL_ERROR;
  }
  return g_can2_rxheader.DLC;
}

HAL_StatusTypeDef setCANxFilter(CAN_HandleTypeDef hcan,CAN_FilterTypeDef* userFilter){
  CAN_FilterTypeDef sFilterConfig;
  sFilterConfig.FilterBank = userFilter->FilterBank; /* 过滤器ID */
  sFilterConfig.FilterMode = userFilter->FilterMode;
  sFilterConfig.FilterScale = userFilter->FilterScale;
  sFilterConfig.FilterIdHigh = userFilter->FilterIdHigh; /* 32位ID */
  sFilterConfig.FilterIdLow = userFilter->FilterIdLow;
  sFilterConfig.FilterMaskIdHigh = userFilter->FilterMaskIdHigh; /* 32位MASK */
  sFilterConfig.FilterMaskIdLow = userFilter->FilterMaskIdLow;
  sFilterConfig.FilterFIFOAssignment = userFilter->FilterFIFOAssignment; /* 过滤器关联到FIFO0 */
  sFilterConfig.FilterActivation = userFilter->FilterActivation;    /* 激活滤波器 */
  #ifndef IS_SET_SALVE_START_FILTER_BANK
  #define IS_SET_SALVE_START_FILTER_BANK
  sFilterConfig.SlaveStartFilterBank = 0;/* 从0开始的从属滤波器 */
  #endif                           
  if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
  {
    return HAL_ERROR;
  }
  return HAL_OK;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{

}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{

}

/* USER CODE END Application */

