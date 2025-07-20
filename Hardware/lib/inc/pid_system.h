#pragma once
#include "main.h"
#include "tim.h"

// PID参数结构体 (使用定点数，放大1000倍)
typedef struct
{
    int32_t kp; // 比例系数 (x1000)
    int32_t ki; // 积分系数 (x1000)
    int32_t kd; // 微分系数 (x1000)

    int32_t setpoint; // 目标值
    int32_t feedback; // 反馈值
    int32_t output;   // 输出值

    int32_t integral;   // 积分累积
    int32_t last_error; // 上次误差
    int32_t derivative; // 微分项

    int32_t max_output;   // 输出上限
    int32_t min_output;   // 输出下限
    int32_t max_integral; // 积分限幅

    uint8_t enable; // PID使能标志
} PID_TypeDef;

// PID控制器结构体
typedef struct
{
    PID_TypeDef pid;

    int32_t (*feedback_function)(void); // 反馈读取函数指针
    void (*output_function)(int32_t);   // 输出控制函数指针

    uint32_t sample_time_ms; // 采样周期(ms) - 仅供参考，实际由定时器控制

    // 统计信息
    uint32_t control_count; // 控制次数计数
    int32_t max_error;      // 最大误差记录
} PID_Controller_TypeDef;

// PID系统管理结构体
typedef struct
{
    PID_Controller_TypeDef controller; // 单个PID控制器
    TIM_HandleTypeDef *system_timer;   // 系统定时器
    uint8_t system_enable;             // 系统使能
} PID_System_TypeDef;

// 函数声明
void PID_System_Init(TIM_HandleTypeDef *htim);
void PID_Controller_Create(int32_t (*feedback_func)(void), void (*output_func)(int32_t),
                           int32_t kp, int32_t ki, int32_t kd,
                           uint32_t sample_time_ms);
void PID_Controller_SetParams(int32_t kp, int32_t ki, int32_t kd);
void PID_Controller_SetLimits(int32_t max_output, int32_t min_output, int32_t max_integral);
void PID_Controller_SetTarget(int32_t target);
void PID_Controller_Enable(uint8_t enable);

// 内部函数
void PID_Calculate(PID_Controller_TypeDef *controller);
void PID_System_Timer_Callback(void);

// 调试和监控函数
void PID_Print_Status(void);
int32_t PID_Get_Error(void);
int32_t PID_Get_Output(void);
