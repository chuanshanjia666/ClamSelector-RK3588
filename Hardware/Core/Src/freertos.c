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
#include "stepmotor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
  GPIO_TypeDef *GPIO_Port;
  uint16_t GPIO_Pin;
  uint16_t debounce_time; // 消抖时间(ms)
  uint8_t stable_state;   // 稳定状态
  uint8_t last_state;     // 上一次状态
  uint8_t button_id;      // 按钮标识
  uint8_t is_debouncing;  // 是否正在消抖
} Button_TypeDef;
Button_TypeDef buttons[] = {
    {PXX1_GPIO_Port, PXX1_Pin, 50, 1, 1, 1, 0},
    {PXX2_GPIO_Port, PXX2_Pin, 50, 1, 1, 2, 0},
    {PXX3_GPIO_Port, PXX3_Pin, 50, 1, 1, 3, 0},
    {PXX4_GPIO_Port, PXX4_Pin, 50, 1, 1, 4, 0},
    {PXX5_GPIO_Port, PXX5_Pin, 50, 1, 1, 5, 0},
    {PXX6_GPIO_Port, PXX6_Pin, 50, 1, 1, 6, 0}};
const char *cmd[] =
    {
        "start \n",
        "class1\n",
        "class2\n",
        "class3\n",
        "class4\n",
};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
uint8_t received;
uint8_t buff[30];
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
uint8_t received_flag = 0; // Flag to indicate if data has been received
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 512 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for STEP_MOTOR1 */
osThreadId_t STEP_MOTOR1Handle;
const osThreadAttr_t STEP_MOTOR1_attributes = {
    .name = "STEP_MOTOR1",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for STEP_MOTOR2 */
osThreadId_t STEP_MOTOR2Handle;
const osThreadAttr_t STEP_MOTOR2_attributes = {
    .name = "STEP_MOTOR2",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for STEP_MOTOR3 */
osThreadId_t STEP_MOTOR3Handle;
const osThreadAttr_t STEP_MOTOR3_attributes = {
    .name = "STEP_MOTOR3",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for rs485mutex */
osMutexId_t rs485mutexHandle;
const osMutexAttr_t rs485mutex_attributes = {
    .name = "rs485mutex"};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Pos)
{
  if (huart->Instance == USART1)
    if (HAL_UARTEx_GetRxEventType(huart) == HAL_UART_RXEVENT_IDLE)
    {
      // memset(buff, 0, Pos);

      buff[Pos] = '\0'; // 添加终止符

      for (int i = 0; i < 4; i++)
      {
        if (strncmp((const char *)buff, cmd[i + 1], 6) == 0)
        {
          printf("Received %s command\r\n", cmd[i + 1]);
          // 根据i的值处理不同命令
          break;
        }
      }

      HAL_UARTEx_ReceiveToIdle_IT(huart, buff, sizeof(buff));
    }
}
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void STEP_MOTOR1fun(void *argument);
void STEP_MOTOR2fun(void *argument);
void STEP_MOTOR3fun(void *argument);

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

void MYHAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM13)
  {
    printf("\r\n");
    for (int i = 0; i < sizeof(buttons) / sizeof(Button_TypeDef); i++)
    {
      if (buttons[i].is_debouncing)
      {
        uint8_t current_state = HAL_GPIO_ReadPin(buttons[i].GPIO_Port, buttons[i].GPIO_Pin);

        // 状态稳定
        if (current_state == buttons[i].last_state)
        {
          if (current_state != buttons[i].stable_state)
          {
            buttons[i].stable_state = current_state;

            // 按钮状态变化处理
            if (current_state == 0)
            {
              printf("1\r\n", buttons[i].button_id);
              // 这里添加按钮按下处理逻辑
            }
            else
            {
              printf("2\r\n", buttons[i].button_id);
              // 这里添加按钮释放处理逻辑
            }
          }
        }
        buttons[i].is_debouncing = 0;
        HAL_TIM_Base_Stop_IT(&htim13); // 停止定时器
        break;
      }
    }
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

  for (int i = 0; i < sizeof(buttons) / sizeof(Button_TypeDef); i++)
  {
    if (GPIO_Pin == buttons[i].GPIO_Pin)
    {
      uint8_t current_state = HAL_GPIO_ReadPin(buttons[i].GPIO_Port, buttons[i].GPIO_Pin);

      // 只有状态变化时才启动消抖
      if (current_state != buttons[i].last_state)
      {
        buttons[i].last_state = current_state;
        buttons[i].is_debouncing = 1;

        // 配置定时器为单次模式
        __HAL_TIM_SET_AUTORELOAD(&htim13, (buttons[i].debounce_time * 1000) - 1); // 转换为us
        __HAL_TIM_SET_COUNTER(&htim13, 0);
        HAL_TIM_Base_Start_IT(&htim13); // 启动定时器
      }
      break;
    }
  }

  // 光传感器处理保持不变
  if (GPIO_Pin == LIGHT_SENSOR_Pin)
  {
    HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin);
    HAL_UART_Transmit(&huart1, cmd[0], 5, HAL_MAX_DELAY);
  }
}

/* USER CODE END 4 */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
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

  /* creation of STEP_MOTOR1 */
  STEP_MOTOR1Handle = osThreadNew(STEP_MOTOR1fun, NULL, &STEP_MOTOR1_attributes);

  /* creation of STEP_MOTOR2 */
  STEP_MOTOR2Handle = osThreadNew(STEP_MOTOR2fun, NULL, &STEP_MOTOR2_attributes);

  /* creation of STEP_MOTOR3 */
  STEP_MOTOR3Handle = osThreadNew(STEP_MOTOR3fun, NULL, &STEP_MOTOR3_attributes);

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
  DC_Motor_Init(&h_dc_motor, &htim1, &htim3, CHANNEL_1, 1000); // Initialize DC motor with TIM3 and TIM4
  for (int i = 0; i < sizeof(buttons) / sizeof(Button_TypeDef); i++)
  {
    buttons[i].stable_state = HAL_GPIO_ReadPin(buttons[i].GPIO_Port, buttons[i].GPIO_Pin);
    buttons[i].last_state = buttons[i].stable_state;
    buttons[i].is_debouncing = 0;
  }
  DC_Motor_SetSpeed(&h_dc_motor, 50); // Set initial speed to 0
  DC_Motor_Start(&h_dc_motor); // Start the DC motor
  // 初始化TIM13为单次模式
  //  HAL_TIM_Base_Start(&htim13); // 先启动定时器但不开启中断

  // Initialize UART for communication
  RS485_Init(&huart1, &rs485mutexHandle, WRITE_EN_GPIO_Port, READ_EN_GPIO_Port, WRITE_EN_Pin, READ_EN_Pin); // Initialize RS485 communication
  RS485_WriteOn();
  osDelay(10);
  RS485_ReadOn(buff, sizeof(buff)); // Enable RS485 write and read



  osDelay(10);
  HAL_UARTEx_ReceiveToIdle_IT(&huart1, buff, sizeof(buff));

  // for (;;)
  // {
  //   for (int k = 0;k<100;k++)
  //   {
  //     DC_Motor_SetSpeed(&h_dc_motor, k);
  //     osDelay(10); // Delay for 10 ms to allow speed change
  //   }
  //   for (int k = 100; k > 0;k--)
  //   {
  //     DC_Motor_SetSpeed(&h_dc_motor, k);
  //     osDelay(10);
  //   }
  // }

    if (HAL_CAN_Start(&hcan2) != HAL_OK)
    {
      /* Start Error */
      printf("CAN2 Start Error\r\n");
    }
    else
    {
      /* Start Success */
      printf("CAN2 Start Success\r\n");
    }
  osDelay(100);
  StepMotor_Init();
  // DC_Motor_SetSpeed(&h_dc_motor, 300); // Set DC motor speed to 500
  // DC_Motor_Start(&h_dc_motor); // Start the DC motor
  // DC_Motor_SetSpeed(&h_dc_motor, 300);
  /* Infinite loop */
  int i = 0;
  for (;;)
  {
    // printf("%d\r\n", __HAL_TIM_GET_COUNTER(&htim13));
    // printf("%d\r\n", HAL_GPIO_ReadPin(PXX2_GPIO_Port, PXX2_Pin));
    // Print the current speed of the DC motor
    // HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin); // Toggle LED state

    osDelay(100); // Delay for 100 ms
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_STEP_MOTOR1fun */
/**
 * @brief Function implementing the STEP_MOTOR1 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_STEP_MOTOR1fun */
void STEP_MOTOR1fun(void *argument)
{
  /* USER CODE BEGIN STEP_MOTOR1fun */
  /* Infinite loop */
  for (;;)
  {
    uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
    StepMotor_id_1.dir = STEPMOTOR_DIRECTION_CW;
    can_ctrl_stepmotor(&StepMotor_id_1);
    
    flags = osThreadFlagsWait(0x02, osFlagsWaitAny, osWaitForever);
    StepMotor_id_1.dir = STEPMOTOR_DIRECTION_CCW;
    can_ctrl_stepmotor(&StepMotor_id_1);

  }
  /* USER CODE END STEP_MOTOR1fun */
}

/* USER CODE BEGIN Header_STEP_MOTOR2fun */
/**
 * @brief Function implementing the STEP_MOTOR2 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_STEP_MOTOR2fun */
void STEP_MOTOR2fun(void *argument)
{
  /* USER CODE BEGIN STEP_MOTOR2fun */
  /* Infinite loop */
  for (;;)
  {
    uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);

    flags = osThreadFlagsWait(0x02, osFlagsWaitAny, osWaitForever);
  }
  /* USER CODE END STEP_MOTOR2fun */
}

/* USER CODE BEGIN Header_STEP_MOTOR3fun */
/**
 * @brief Function implementing the STEP_MOTOR3 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_STEP_MOTOR3fun */
void STEP_MOTOR3fun(void *argument)
{
  /* USER CODE BEGIN STEP_MOTOR3fun */
  /* Infinite loop */
  for (;;)
  {
    uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);

    flags = osThreadFlagsWait(0x02, osFlagsWaitAny, osWaitForever);
  }
  /* USER CODE END STEP_MOTOR3fun */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
