#pragma once
#include "main.h"
#include "tim.h"

#define MAX_BUTTONS 6
#define DEBOUNCE_TIME_MS 100 // 消抖时间50ms

typedef enum
{
    BUTTON_STATE_IDLE,
    BUTTON_STATE_DEBOUNCING,
    BUTTON_STATE_PRESSED
} ButtonState_t;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState stable_state;  // 按钮按下时的稳定状态
    GPIO_PinState trigger_state; // 触发消抖时的状态
    ButtonState_t current_state; // 当前按钮状态
    void (*button_down_callback)(void);
    void (*button_up_callback)(void);
    uint8_t is_active;    // 按钮是否激活
    uint8_t timer_active; // 定时器是否激活
} ButtonElement_t;

typedef struct
{
    ButtonElement_t buttons[MAX_BUTTONS];
    uint8_t button_count;
    TIM_HandleTypeDef *timer_handle; // 用于消抖的单次定时器
    volatile uint8_t pending_button; // 等待处理的按钮索引
} ButtonSystem_t;

// 函数声明
void Button_System_Init(TIM_HandleTypeDef *htim);
int8_t Button_System_Add(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState stable_state,
                         void (*down_callback)(void), void (*up_callback)(void));
void Button_System_IRQ_Entry(uint16_t GPIO_Pin);
void Button_System_Timer_Callback(void);
GPIO_PinState Button_System_Read_Pin(GPIO_TypeDef *port, uint16_t pin);
int8_t Button_System_Find_Button(uint16_t pin);
void Button_System_Start_Debounce_Timer(uint8_t button_index);
