#pragma once
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

typedef struct RS485_HandleTypeDef
{
    UART_HandleTypeDef *rs485_uart;
    osMutexId_t *rs485_mutex;
    GPIO_TypeDef *rs485_write;
    GPIO_TypeDef *rs485_read;
    uint16_t rs485_write_pin;
    uint16_t rs485_read_pin;
    uint8_t *buff;
} RS485_HandleTypeDef;

void RS485_Init(UART_HandleTypeDef *rs485_uart, osMutexId_t *rs485_mutex, 
                GPIO_TypeDef *rs485_write, GPIO_TypeDef *rs485_read,
                uint16_t rs485_write_pin, uint16_t rs485_read_pin);
void RS485_WriteOn(void);
void RS485_ReadOn(uint8_t *buff, uint16_t size);
void RS485_Send(const uint8_t *pData, uint16_t size);

