#include "pid_system.h"
#include <stdio.h>
#include <string.h>

// PID定点数缩放因子
#define PID_SCALE_FACTOR 1000

// 全局PID系统实例
static PID_System_TypeDef g_pid_system = {0};

/**
 * @brief 初始化PID系统
 * @param htim 用于PID控制的定时器句柄
 */
void PID_System_Init(TIM_HandleTypeDef *htim)
{
    memset(&g_pid_system, 0, sizeof(PID_System_TypeDef));
    g_pid_system.system_timer = htim;
    g_pid_system.controller_count = 0;
    g_pid_system.system_enable = 1;

    // 启动定时器中断，定时器周期就是PID采样周期
    HAL_TIM_Base_Start_IT(htim);

    printf("PID System Initialized with Timer (Fixed-Point)\r\n");
}

/**
 * @brief 创建一个PID控制器
 * @param motor 关联的电机句柄
 * @param kp 比例系数 (已放大1000倍)
 * @param ki 积分系数 (已放大1000倍)
 * @param kd 微分系数 (已放大1000倍)
 * @param sample_time_ms 采样周期(ms) - 仅供参考，实际由定时器控制
 * @return 控制器ID，255表示失败
 */
uint8_t PID_Controller_Create(DC_Monitor_TypeDef *motor,
                              int32_t kp, int32_t ki, int32_t kd,
                              uint32_t sample_time_ms)
{
    if (g_pid_system.controller_count >= 4)
    {
        printf("PID Controller limit reached!\r\n");
        return 255;
    }

    uint8_t id = g_pid_system.controller_count;
    PID_Controller_TypeDef *controller = &g_pid_system.controllers[id];

    // 初始化PID参数
    controller->pid.kp = kp;
    controller->pid.ki = ki;
    controller->pid.kd = kd;
    controller->pid.setpoint = 0;
    controller->pid.feedback = 0;
    controller->pid.output = 0;
    controller->pid.integral = 0;
    controller->pid.last_error = 0;
    controller->pid.derivative = 0;

    // 设置默认限制
    controller->pid.max_output = 100000;    // 最大PWM占空比 (100.0 * 1000)
    controller->pid.min_output = 0;         // 最小PWM占空比
    controller->pid.max_integral = 1000000; // 积分限幅 (1000.0 * 1000)
    controller->pid.enable = 0;             // 默认禁用

    // 关联电机和设置参数
    controller->motor = motor;
    controller->sample_time_ms = sample_time_ms;
    controller->control_count = 0;
    controller->max_error = 0;
    controller->controller_id = id;

    g_pid_system.controller_count++;

    printf("PID Controller %d created: Kp=%ld, Ki=%ld, Kd=%ld (x1000)\r\n",
           id, kp, ki, kd);

    return id;
}

/**
 * @brief 设置PID参数
 * @param controller_id 控制器ID
 * @param kp 比例系数 (已放大1000倍)
 * @param ki 积分系数 (已放大1000倍)
 * @param kd 微分系数 (已放大1000倍)
 */
void PID_Controller_SetParams(uint8_t controller_id, int32_t kp, int32_t ki, int32_t kd)
{
    if (controller_id >= g_pid_system.controller_count)
        return;

    PID_TypeDef *pid = &g_pid_system.controllers[controller_id].pid;
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    // 参数改变时清零积分项，避免突变
    pid->integral = 0;
    pid->last_error = 0;
}

/**
 * @brief 设置输出限制
 * @param controller_id 控制器ID
 * @param max_output 最大输出
 * @param min_output 最小输出
 * @param max_integral 积分限幅
 */
void PID_Controller_SetLimits(uint8_t controller_id, int32_t max_output, int32_t min_output, int32_t max_integral)
{
    if (controller_id >= g_pid_system.controller_count)
        return;

    PID_TypeDef *pid = &g_pid_system.controllers[controller_id].pid;
    pid->max_output = max_output;
    pid->min_output = min_output;
    pid->max_integral = max_integral;
}

/**
 * @brief 设置目标值
 * @param controller_id 控制器ID
 * @param target 目标值
 */
void PID_Controller_SetTarget(uint8_t controller_id, int32_t target)
{
    if (controller_id >= g_pid_system.controller_count)
        return;

    g_pid_system.controllers[controller_id].pid.setpoint = target;
}

/**
 * @brief 使能/禁用控制器
 * @param controller_id 控制器ID
 * @param enable 1=使能，0=禁用
 */
void PID_Controller_Enable(uint8_t controller_id, uint8_t enable)
{
    if (controller_id >= g_pid_system.controller_count)
        return;

    PID_Controller_TypeDef *controller = &g_pid_system.controllers[controller_id];
    controller->pid.enable = enable;

    if (enable)
    {
        // 使能时重置状态
        controller->pid.integral = 0;
        controller->pid.last_error = 0;
        printf("PID Controller %d Enabled\r\n", controller_id);
    }
    else
    {
        // 禁用时停止电机
        DC_Motor_SetSpeed(controller->motor, 0);
        printf("PID Controller %d Disabled\r\n", controller_id);
    }
}

/**
 * @brief PID计算核心函数 (定点数运算)
 * @param controller PID控制器指针
 */
void PID_Calculate(PID_Controller_TypeDef *controller)
{
    if (!controller->pid.enable)
        return;

    // 读取反馈值（编码器计数转换为速度）
    DC_Motor_GetEncodeNum(controller->motor);
    controller->pid.feedback = PID_Get_Motor_Speed(controller->motor);

    // 计算误差
    int32_t error = controller->pid.setpoint - controller->pid.feedback;

    // 比例项 (Kp * error / 1000)
    int32_t proportional = (controller->pid.kp * error) / PID_SCALE_FACTOR;

    // 积分项 (累积误差并限幅)
    controller->pid.integral += error;
    if (controller->pid.integral > controller->pid.max_integral)
    {
        controller->pid.integral = controller->pid.max_integral;
    }
    else if (controller->pid.integral < -controller->pid.max_integral)
    {
        controller->pid.integral = -controller->pid.max_integral;
    }
    int32_t integral = (controller->pid.ki * controller->pid.integral) / PID_SCALE_FACTOR;

    // 微分项 (当前误差 - 上次误差)
    controller->pid.derivative = error - controller->pid.last_error;
    int32_t differential = (controller->pid.kd * controller->pid.derivative) / PID_SCALE_FACTOR;

    // PID输出
    controller->pid.output = proportional + integral + differential;

    // 输出限幅
    if (controller->pid.output > controller->pid.max_output)
    {
        controller->pid.output = controller->pid.max_output;
    }
    else if (controller->pid.output < controller->pid.min_output)
    {
        controller->pid.output = controller->pid.min_output;
    }

    // 设置电机速度 (转换回实际范围)
    int32_t motor_speed = controller->pid.output / PID_SCALE_FACTOR;
    if (motor_speed > 100)
        motor_speed = 100;
    if (motor_speed < 0)
        motor_speed = 0;
    DC_Motor_SetSpeed(controller->motor, (uint16_t)motor_speed);

    // 更新状态
    controller->pid.last_error = error;
    controller->control_count++;

    // 记录最大误差 (绝对值)
    int32_t abs_error = (error >= 0) ? error : -error;
    if (abs_error > controller->max_error)
    {
        controller->max_error = abs_error;
    }
}

/**
 * @brief 获取电机速度（从编码器）
 * @param motor 电机句柄
 * @return 速度值 (定点数)
 */
int32_t PID_Get_Motor_Speed(DC_Monitor_TypeDef *motor)
{
    // 这里可以根据编码器计数计算实际速度
    // 简化处理：直接返回编码器计数差值
    // 实际应用中需要根据编码器分辨率和采样时间计算RPM
    static uint16_t last_count = 0;
    uint16_t current_count = motor->encode_num;

    int16_t speed_raw = current_count - last_count;
    last_count = current_count;

    // 处理定时器溢出情况
    if (speed_raw > 32767)
        speed_raw -= 65536;
    if (speed_raw < -32768)
        speed_raw += 65536;

    // 放大到定点数范围 (可根据需要调整缩放)
    return (int32_t)speed_raw * PID_SCALE_FACTOR;
}

/**
 * @brief 定时器中断回调函数
 * 这个函数应该在定时器中断处理函数中被调用
 * 定时器周期就是PID采样周期，无需额外时间检查
 */
void PID_System_Timer_Callback(void)
{
    if (!g_pid_system.system_enable)
        return;

    // 依次处理所有使能的PID控制器
    for (uint8_t i = 0; i < g_pid_system.controller_count; i++)
    {
        PID_Calculate(&g_pid_system.controllers[i]);
    }
}

/**
 * @brief 打印PID状态信息
 * @param controller_id 控制器ID
 */
void PID_Print_Status(uint8_t controller_id)
{
    if (controller_id >= g_pid_system.controller_count)
        return;

    PID_Controller_TypeDef *controller = &g_pid_system.controllers[controller_id];
    PID_TypeDef *pid = &controller->pid;

    printf("=== PID Controller %d Status ===\r\n", controller_id);
    printf("Enable: %s\r\n", pid->enable ? "YES" : "NO");
    printf("Setpoint: %ld\r\n", pid->setpoint);
    printf("Feedback: %ld\r\n", pid->feedback);
    printf("Output: %ld\r\n", pid->output);
    printf("Error: %ld\r\n", pid->setpoint - pid->feedback);
    printf("Integral: %ld\r\n", pid->integral);
    printf("Derivative: %ld\r\n", pid->derivative);
    printf("Control Count: %lu\r\n", controller->control_count);
    printf("Max Error: %ld\r\n", controller->max_error);
    printf("============================\r\n");
}

/**
 * @brief 获取当前误差
 * @param controller_id 控制器ID
 * @return 当前误差值
 */
int32_t PID_Get_Error(uint8_t controller_id)
{
    if (controller_id >= g_pid_system.controller_count)
        return 0;

    PID_TypeDef *pid = &g_pid_system.controllers[controller_id].pid;
    return pid->setpoint - pid->feedback;
}

/**
 * @brief 获取当前输出
 * @param controller_id 控制器ID
 * @return 当前输出值
 */
int32_t PID_Get_Output(uint8_t controller_id)
{
    if (controller_id >= g_pid_system.controller_count)
        return 0;

    return g_pid_system.controllers[controller_id].pid.output;
}
