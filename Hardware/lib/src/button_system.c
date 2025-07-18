#include "botton_system.h"
#include <string.h>

// 全局按钮系统实例
static ButtonSystem_t g_button_system = {0};

/**
 * @brief 初始化按钮系统
 * @param htim 用于消抖的单次定时器句柄
 */
void Button_System_Init(TIM_HandleTypeDef *htim)
{
    memset(&g_button_system, 0, sizeof(ButtonSystem_t));
    g_button_system.timer_handle = htim;
    g_button_system.button_count = 0;
    g_button_system.pending_button = 0xFF; // 无效索引

    // 配置定时器为单次模式（在CubeMX中应设置为One Shot模式）
    // 这里只是确保定时器已经初始化，不启动
}

/**
 * @brief 添加一个按钮到系统中
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @param stable_state 按钮按下时的稳定状态
 * @param down_callback 按钮按下回调函数
 * @param up_callback 按钮松开回调函数
 * @return 按钮索引，-1表示失败
 */
int8_t Button_System_Add(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState stable_state,
                         void (*down_callback)(void), void (*up_callback)(void))
{
    if (g_button_system.button_count >= MAX_BUTTONS)
    {
        return -1; // 按钮数量已满
    }

    ButtonElement_t *btn = &g_button_system.buttons[g_button_system.button_count];

    btn->port = port;
    btn->pin = pin;
    btn->stable_state = stable_state;
    btn->current_state = BUTTON_STATE_IDLE;
    btn->button_down_callback = down_callback;
    btn->button_up_callback = up_callback;
    btn->is_active = 1;
    btn->timer_active = 0;

    return g_button_system.button_count++;
}

/**
 * @brief GPIO中断入口函数 - 边沿重启式消抖
 * @param GPIO_Pin 触发中断的GPIO引脚
 */
void Button_System_IRQ_Entry(uint16_t GPIO_Pin)
{
    int8_t btn_index = Button_System_Find_Button(GPIO_Pin);
    if (btn_index < 0)
        return;

    ButtonElement_t *btn = &g_button_system.buttons[btn_index];
    if (!btn->is_active)
        return;

    // 读取当前引脚状态
    GPIO_PinState current_state = Button_System_Read_Pin(btn->port, btn->pin);

    // 根据当前状态决定处理逻辑
    switch (btn->current_state)
    {
    case BUTTON_STATE_IDLE:
        // 空闲状态，启动消抖计时
        btn->trigger_state = current_state;
        btn->current_state = BUTTON_STATE_DEBOUNCING;
        Button_System_Start_Debounce_Timer(btn_index);
        break;

    case BUTTON_STATE_PRESSED:
        // 已按下状态，检测到状态变化，重新开始消抖
        btn->trigger_state = current_state;
        btn->current_state = BUTTON_STATE_DEBOUNCING;
        Button_System_Start_Debounce_Timer(btn_index);
        break;

    case BUTTON_STATE_DEBOUNCING:
        // 正在消抖中，检查状态是否变化
        if (current_state != btn->trigger_state)
        {
            // 状态发生变化，重新开始消抖计时
            btn->trigger_state = current_state;
            Button_System_Start_Debounce_Timer(btn_index);
        }
        // 如果状态没有变化，继续等待当前的定时器
        break;
    }
}

/**
 * @brief 启动消抖定时器 - 边沿重启式消抖
 * @param button_index 按钮索引
 *
 * 工作原理：
 * 1. 每次GPIO状态变化都会重启定时器
 * 2. 只有当信号稳定保持DEBOUNCE_TIME_MS后才确认状态
 * 3. 这确保了只有真正稳定的信号才会被识别
 *
 * 时序示例：
 * GPIO:  ↓__↑_↓___↑↓_____________________↑ (抖动后稳定)
 * Timer: |↻ |↻|↻   |↻|----------------------|✓ (重启后最终稳定)
 * 说明：  ↻=重启定时器  ✓=定时器到期确认状态
 */
void Button_System_Start_Debounce_Timer(uint8_t button_index)
{
    g_button_system.pending_button = button_index;
    g_button_system.buttons[button_index].timer_active = 1;

    // 停止定时器（如果在运行）
    HAL_TIM_Base_Stop_IT(g_button_system.timer_handle);

    // 重新加载计数值并启动单次定时
    // 假设定时器配置为1ms基准，需要DEBOUNCE_TIME_MS个计数
    __HAL_TIM_SET_COUNTER(g_button_system.timer_handle, 0);
    __HAL_TIM_SET_AUTORELOAD(g_button_system.timer_handle, DEBOUNCE_TIME_MS - 1);

    // 启动单次定时器
    HAL_TIM_Base_Start_IT(g_button_system.timer_handle);
}

/**
 * @brief 定时器超时回调 - 稳定状态确认处理
 */
void Button_System_Timer_Callback(void)
{
    if (g_button_system.pending_button == 0xFF)
        return;

    uint8_t btn_index = g_button_system.pending_button;
    ButtonElement_t *btn = &g_button_system.buttons[btn_index];

    // 读取当前稳定状态
    GPIO_PinState current_state = Button_System_Read_Pin(btn->port, btn->pin);

    // 验证状态是否与触发时一致且已经稳定了DEBOUNCE_TIME_MS
    if (current_state == btn->trigger_state)
    {
        // 状态稳定，确认有效事件
        if (current_state == btn->stable_state)
        {
            // 按钮被稳定按下
            if (btn->current_state != BUTTON_STATE_PRESSED)
            {
                btn->current_state = BUTTON_STATE_PRESSED;
                if (btn->button_down_callback)
                {
                    btn->button_down_callback();
                }
            }
        }
        else
        {
            // 按钮被稳定松开
            if (btn->current_state != BUTTON_STATE_IDLE)
            {
                btn->current_state = BUTTON_STATE_IDLE;
                if (btn->button_up_callback)
                {
                    btn->button_up_callback();
                }
            }
        }
    }
    else
    {
        // 状态在定时期间又发生了变化，这种情况理论上不应该发生
        // 因为任何状态变化都会重启定时器
        // 作为安全措施，重新开始消抖过程
        btn->trigger_state = current_state;
        Button_System_Start_Debounce_Timer(btn_index);
        return; // 不清理定时器状态，继续等待
    }

    // 清理定时器状态
    btn->timer_active = 0;
    g_button_system.pending_button = 0xFF;

    // 停止定时器
    HAL_TIM_Base_Stop_IT(g_button_system.timer_handle);
}

/**
 * @brief 读取GPIO引脚状态
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @return GPIO引脚状态
 */
GPIO_PinState Button_System_Read_Pin(GPIO_TypeDef *port, uint16_t pin)
{
    return HAL_GPIO_ReadPin(port, pin);
}

/**
 * @brief 根据引脚号查找按钮索引
 * @param pin GPIO引脚号
 * @return 按钮索引，-1表示未找到
 */
int8_t Button_System_Find_Button(uint16_t pin)
{
    for (uint8_t i = 0; i < g_button_system.button_count; i++)
    {
        if (g_button_system.buttons[i].pin == pin)
        {
            return i;
        }
    }
    return -1;
}