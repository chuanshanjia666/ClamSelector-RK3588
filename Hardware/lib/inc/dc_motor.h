#pragma once
#include "main.h"

typedef enum TIM_Channel
{
    CHANNEL_1 = 0x00000000U,
    CHANNEL_2 = 0x00000004U,
    CHANNEL_3 = 0x00000008U,
    CHANNEL_4 = 0x0000000CU,
    CHANNEL_ALL = 0x0000003CU
} TIM_Channel;

// typedef enum DC_Monitor_Speed
// {
//     SPEED_B_VERYFAST = 10U,
//     SPEED_B_FAST = 20U,
//     SPEED_B_MID = 30U,
//     SPEED_B_SLOW = 40U,
//     SPEED_STOP = 50U,
//     SPEED_F_SLOW = 60U,
//     SPEED_F_MID = 70U,
//     SPEED_F_FAST = 80U,
//     SPEED_F_VERYFAST = 90U,
// } DC_Monitor_Speed;

typedef uint16_t DC_Monitor_Speed;

typedef struct DC_Monitor_TypeDef
{
    TIM_HandleTypeDef *speed_htim;
    TIM_Channel channel;
    uint8_t duty;
    uint16_t max_tim;
    uint16_t encode_num;
    TIM_HandleTypeDef *encode_htim;
} DC_Monitor_TypeDef;

void DC_Motor_Init(DC_Monitor_TypeDef *dc_monitior_handler, TIM_HandleTypeDef *speed_htim, TIM_HandleTypeDef *encode_htim, TIM_Channel channel, uint16_t max_tim);
void DC_Motor_Start(DC_Monitor_TypeDef *dc_monitior_handler);
void DC_Motor_Stop(DC_Monitor_TypeDef *dc_monitior_handler);
void DC_Motor_SetSpeed(DC_Monitor_TypeDef *dc_monitior_handler, DC_Monitor_Speed speed);
void DC_Motor_GetEncodeNum(DC_Monitor_TypeDef *dc_monitior_handler);