#include "rs485.h"
static RS485_HandleTypeDef rs485_handler;
void RS485_Init(UART_HandleTypeDef *rs485_uart, osMutexId_t *rs485_mutex, 
                GPIO_TypeDef *rs485_write, GPIO_TypeDef *rs485_read,
                uint16_t rs485_write_pin, uint16_t rs485_read_pin)
{
    rs485_handler.rs485_uart = rs485_uart;
    rs485_handler.rs485_mutex = rs485_mutex;
    rs485_handler.rs485_write = rs485_write;
    rs485_handler.rs485_read = rs485_read;
    rs485_handler.rs485_write_pin = rs485_write_pin;
    rs485_handler.rs485_read_pin = rs485_read_pin;
    rs485_handler.buff = NULL; // Initialize the buffer to NULL
}
void RS485_WriteOn(void)
{
    HAL_GPIO_WritePin(rs485_handler.rs485_write, rs485_handler.rs485_write_pin, GPIO_PIN_SET);
}
void RS485_ReadOn(uint8_t *buff, uint16_t size)
{
    HAL_GPIO_WritePin(rs485_handler.rs485_read, rs485_handler.rs485_read_pin, GPIO_PIN_RESET);
    rs485_handler.buff = buff; // Store the buffer in the handler
}
void RS485_Send(const uint8_t *pData, uint16_t size)
{
    osMutexWait(*(rs485_handler.rs485_mutex), osWaitForever);
    HAL_UART_Transmit(rs485_handler.rs485_uart, (uint8_t *)pData, size, HAL_MAX_DELAY);
    osMutexRelease(*(rs485_handler.rs485_mutex));
}
