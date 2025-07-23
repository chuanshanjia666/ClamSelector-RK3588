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
#include <stdio.h>
#include "usart.h"
#include "dc_motor.h"
#include "tim.h"
#include "can.h"
#include "step_motor.h"
#include "myqueue.h"
#include "button_system.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
typedef enum System_Mode
{
  SYSTEM_MODE_OFF = 0,
  SYSTEM_MODE_AUTO = 1,
  SYSTEM_MODE_HAND = 2
} System_Mode_Typedef;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
uint8_t timer_set_flag = 0;
System_Mode_Typedef system_mode = SYSTEM_MODE_OFF;
const char *cmd[] =
    {
        "start \n",
        "class1\n",
        "class2\n",
        "class3\n"};
uint8_t buff[40];
DC_Monitor_TypeDef hdc;
DECLARE_QUEUE(step_motor1_queue, osTimerId_t, 8)
DECLARE_QUEUE(step_motor2_queue, osTimerId_t, 8)
DECLARE_QUEUE(step_motor3_queue, osTimerId_t, 8)

/* USER CODE END Variables */
/* Definitions for main_task */
osThreadId_t main_taskHandle;
const osThreadAttr_t main_task_attributes = {
    .name = "main_task",
    .stack_size = 512 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for step_motor1 */
osThreadId_t step_motor1Handle;
const osThreadAttr_t step_motor1_attributes = {
    .name = "step_motor1",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityHigh,
};
/* Definitions for step_motor2 */
osThreadId_t step_motor2Handle;
const osThreadAttr_t step_motor2_attributes = {
    .name = "step_motor2",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityHigh,
};
/* Definitions for step_motor3 */
osThreadId_t step_motor3Handle;
const osThreadAttr_t step_motor3_attributes = {
    .name = "step_motor3",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityHigh,
};
/* Definitions for step_motor1_queue_mutex */
osMutexId_t step_motor1_queue_mutexHandle;
const osMutexAttr_t step_motor1_queue_mutex_attributes = {
    .name = "step_motor1_queue_mutex"};
/* Definitions for step_motor2_queue_mutex */
osMutexId_t step_motor2_queue_mutexHandle;
const osMutexAttr_t step_motor2_queue_mutex_attributes = {
    .name = "step_motor2_queue_mutex"};
/* Definitions for step_motor3_queue_mutex */
osMutexId_t step_motor3_queue_mutexHandle;
const osMutexAttr_t step_motor3_queue_mutex_attributes = {
    .name = "step_motor3_queue_mutex"};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void step_motor1_timer_callback()
{
  osThreadFlagsSet(step_motor1Handle, 0x01);
}
void step_motor2_timer_callback()
{
  osThreadFlagsSet(step_motor2Handle, 0x01);
}
void step_motor3_timer_callback()
{
  osThreadFlagsSet(step_motor3Handle, 0x01);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == LIGHT_SENSOR_Pin)
  {
    if(system_mode==SYSTEM_MODE_AUTO)
    {
      timer_set_flag = 1;
    }

    // HAL_UART_Transmit_IT(&huart1, cmd[0], 7);
    // printf("Tim start\r\n");
  }
  else
  {
    Button_System_IRQ_Entry(GPIO_Pin);
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART1)
  {
    if (HAL_UARTEx_GetRxEventType(huart) == HAL_UART_RXEVENT_IDLE)
    {
      if (system_mode == SYSTEM_MODE_AUTO)
      {
        if (Size >= 6)
        {
          timer_set_flag = buff[5] - '0';
        }
        if (timer_set_flag == 4)
          timer_set_flag = 1;
      }
      HAL_UARTEx_ReceiveToIdle_IT(&huart1, buff, sizeof(buff));
    }
  }
}

void STEP_Motor1_buttonPushCallback(void)
{
  if (system_mode == SYSTEM_MODE_HAND)
  {
    osThreadFlagsSet(step_motor1Handle, 0x01);
  }
  else
  {
    printf("[down]Error: You can't control stepmotor1 unless hand mode\n");
  }
}

void STEP_Motor2_buttonPushCallback(void)
{
  if (system_mode == SYSTEM_MODE_HAND)
  {
    osThreadFlagsSet(step_motor2Handle, 0x01);
  }
  else
  {
    printf("[down]Error: You can't control stepmotor2 unless hand mode\n");
  }
}

void STEP_Motor3_buttonPushCallback(void)
{
  if (system_mode == SYSTEM_MODE_HAND)
  {
    osThreadFlagsSet(step_motor3Handle, 0x01);
  }
  else
  {
    printf("[down]Error: You can't control stepmotor3 unless hand mode\n");
  }
}

void SYSTEM_Hand_Mode_OnButtonCallBack(void)
{
  if (system_mode != SYSTEM_MODE_HAND)
  {
    system_mode = SYSTEM_MODE_HAND;
    printf("System Mode: Hand\r\n");

    DC_Motor_SetSpeed(&hdc, 10);
  }
}

void SYSTEM_Off_Mode_OnButtonCallBack(void)
{
  if (system_mode != SYSTEM_MODE_OFF)
  {
    system_mode = SYSTEM_MODE_OFF;
    printf("System Mode: Off\r\n");
    DC_Motor_SetSpeed(&hdc, 0);
    HAL_GPIO_WritePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin, GPIO_PIN_SET);
  }
}

void SYSTEM_Auto_Mode_OnButtonCallBack(void)
{
  if (system_mode != SYSTEM_MODE_AUTO)
  {
    system_mode = SYSTEM_MODE_AUTO;
    printf("System Mode: Auto\r\n");
    DC_Motor_SetSpeed(&hdc, 10);
    HAL_GPIO_WritePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin, GPIO_PIN_RESET);
  }
}



/* USER CODE END FunctionPrototypes */

void MainTask(void *argument);
void step_motor1_task(void *argument);
void step_motor2_task(void *argument);
void step_motor3_task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
  called if a stack overflow is detected. */
  printf("overflow %s\r\n", pcTaskName);
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
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of step_motor1_queue_mutex */
  step_motor1_queue_mutexHandle = osMutexNew(&step_motor1_queue_mutex_attributes);

  /* creation of step_motor2_queue_mutex */
  step_motor2_queue_mutexHandle = osMutexNew(&step_motor2_queue_mutex_attributes);

  /* creation of step_motor3_queue_mutex */
  step_motor3_queue_mutexHandle = osMutexNew(&step_motor3_queue_mutex_attributes);

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
  /* creation of main_task */
  main_taskHandle = osThreadNew(MainTask, NULL, &main_task_attributes);

  /* creation of step_motor1 */
  step_motor1Handle = osThreadNew(step_motor1_task, NULL, &step_motor1_attributes);

  /* creation of step_motor2 */
  step_motor2Handle = osThreadNew(step_motor2_task, NULL, &step_motor2_attributes);

  /* creation of step_motor3 */
  step_motor3Handle = osThreadNew(step_motor3_task, NULL, &step_motor3_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_MainTask */
/**
 * @brief  Function implementing the main_task thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_MainTask */
void MainTask(void *argument)
{
  /* USER CODE BEGIN MainTask */
  step_motor1_queue_init();
  step_motor2_queue_init();
  step_motor3_queue_init();


  DC_Motor_Init(&hdc, &htim1, &htim3, TIM_CHANNEL_1, 100);
  DC_Motor_SetSpeed(&hdc, 0);
  DC_Motor_Start(&hdc);
  HAL_CAN_Start(&hcan1);
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
  HAL_UARTEx_ReceiveToIdle_IT(&huart1, buff, sizeof(buff));
  osDelay(1);
  StepMotor_Init(&hcan1);

  Button_System_Init(&htim13);
  // system_mode = SYSTEM_MODE_HAND;
  Button_System_Add(PXX1_GPIO_Port, PXX1_Pin, GPIO_PIN_RESET, STEP_Motor1_buttonPushCallback, NULL);
  Button_System_Add(PXX2_GPIO_Port, PXX2_Pin, GPIO_PIN_RESET, STEP_Motor3_buttonPushCallback, NULL);
  Button_System_Add(PXX3_GPIO_Port, PXX3_Pin, GPIO_PIN_RESET, STEP_Motor2_buttonPushCallback, NULL);
  Button_System_Add(PXX4_GPIO_Port, PXX4_Pin, GPIO_PIN_SET, SYSTEM_Hand_Mode_OnButtonCallBack, SYSTEM_Off_Mode_OnButtonCallBack);
  Button_System_Add(PXX5_GPIO_Port, PXX5_Pin, GPIO_PIN_SET, SYSTEM_Auto_Mode_OnButtonCallBack, SYSTEM_Off_Mode_OnButtonCallBack);
  // Button_System_Add(BUTTON_TEST_GPIO_Port, BUTTON_TEST_Pin, GPIO_PIN_RESET, PushCallBack, ReleaseCallBack);
  printf("System Init success\n");
  osTimerId_t tim_id;
  /* Infinite loop */
  for (;;)
  {
    switch (timer_set_flag)
    {
    case 1:
      tim_id = osTimerNew(step_motor1_timer_callback, osTimerOnce, NULL, NULL);
      osTimerStart(tim_id, 10500);
      osMutexAcquire(step_motor1_queue_mutexHandle, osWaitForever);
      step_motor1_queue_enqueue(&tim_id);
      osMutexRelease(step_motor1_queue_mutexHandle);
      timer_set_flag = 0;
      break;
    case 2:
      tim_id = osTimerNew(step_motor2_timer_callback, osTimerOnce, NULL, NULL);
      osTimerStart(tim_id, 3000);
      osMutexAcquire(step_motor2_queue_mutexHandle, osWaitForever);
      step_motor2_queue_enqueue(&tim_id);
      osMutexRelease(step_motor2_queue_mutexHandle);
      timer_set_flag = 0;
      break;
    case 3:
      tim_id = osTimerNew(step_motor3_timer_callback, osTimerOnce, NULL, NULL);
      osTimerStart(tim_id, 6000);
      osMutexAcquire(step_motor3_queue_mutexHandle, osWaitForever);
      step_motor3_queue_enqueue(&tim_id);
      osMutexRelease(step_motor3_queue_mutexHandle);
      timer_set_flag = 0;
      break;
    default:
      break;
    }

    osDelay(1);
  }
  /* USER CODE END MainTask */
}

/* USER CODE BEGIN Header_step_motor1_task */
/**
 * @brief Function implementing the step_motor1 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_step_motor1_task */
void step_motor1_task(void *argument)
{
  /* USER CODE BEGIN step_motor1_task */
  /* Infinite loop */
  osTimerId_t tim_id;
  for (;;)
  {
    osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
    StepMotor_id_1.speed_l = 70;
    StepMotor_id_1.dir = STEPMOTOR_DIRECTION_CW;
    can_ctrl_stepmotor(&StepMotor_id_1);

    osDelay(600);
    StepMotor_id_1.speed_l = 100;
    StepMotor_id_1.dir = STEPMOTOR_DIRECTION_CCW;
    can_ctrl_stepmotor(&StepMotor_id_1);

    if (system_mode == SYSTEM_MODE_AUTO)
    {
      osMutexAcquire(step_motor1_queue_mutexHandle, osWaitForever);
      step_motor1_queue_dequeue(&tim_id);
      osMutexRelease(step_motor1_queue_mutexHandle);
      osTimerDelete(tim_id);
    }
  }
  /* USER CODE END step_motor1_task */
}

/* USER CODE BEGIN Header_step_motor2_task */
/**
 * @brief Function implementing the step_motor2 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_step_motor2_task */
void step_motor2_task(void *argument)
{
  /* USER CODE BEGIN step_motor2_task */
  /* Infinite loop */
  osTimerId_t tim_id;
  for (;;)
  {
    osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
    StepMotor_id_2.speed_l = 70;
    StepMotor_id_2.dir = STEPMOTOR_DIRECTION_CW;
    can_ctrl_stepmotor(&StepMotor_id_2);

    osDelay(600);
    StepMotor_id_2.speed_l = 100;
    StepMotor_id_2.dir = STEPMOTOR_DIRECTION_CCW;
    can_ctrl_stepmotor(&StepMotor_id_2);

    if (system_mode == SYSTEM_MODE_AUTO)
    {
      osMutexAcquire(step_motor2_queue_mutexHandle, osWaitForever);
      step_motor2_queue_dequeue(&tim_id);
      osMutexRelease(step_motor2_queue_mutexHandle);
      osTimerDelete(tim_id);
    }
  }
  /* USER CODE END step_motor2_task */
}

/* USER CODE BEGIN Header_step_motor3_task */
/**
 * @brief Function implementing the step_motor3 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_step_motor3_task */
void step_motor3_task(void *argument)
{
  /* USER CODE BEGIN step_motor3_task */
  /* Infinite loop */
  osTimerId_t tim_id;
  for (;;)
  {
    osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
    StepMotor_id_3.speed_l = 70;
    StepMotor_id_3.dir = STEPMOTOR_DIRECTION_CW;
    can_ctrl_stepmotor(&StepMotor_id_3);

    osDelay(600);
    StepMotor_id_3.speed_l = 100;
    StepMotor_id_3.dir = STEPMOTOR_DIRECTION_CCW;
    can_ctrl_stepmotor(&StepMotor_id_3);

    if (system_mode == SYSTEM_MODE_AUTO)
    {
      osMutexAcquire(step_motor3_queue_mutexHandle, osWaitForever);
      step_motor3_queue_dequeue(&tim_id);
      osMutexRelease(step_motor3_queue_mutexHandle);
      osTimerDelete(tim_id);
    }
  }
  /* USER CODE END step_motor3_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
