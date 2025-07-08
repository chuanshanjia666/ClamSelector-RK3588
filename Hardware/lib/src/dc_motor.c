#include "main.h"
#include "dc_motor.h"

void DC_Motor_Init(DC_Monitor_TypeDef *dc_monitior_handler, TIM_HandleTypeDef *speed_htim, TIM_HandleTypeDef *encode_htim, TIM_Channel channel, uint16_t max_tim)
{
    dc_monitior_handler->speed_htim = speed_htim;
    dc_monitior_handler->channel = channel;
    dc_monitior_handler->encode_htim = encode_htim;
    dc_monitior_handler->max_tim = max_tim;
}


void DC_Motor_Start(DC_Monitor_TypeDef *dc_monitior_handler)
{
    HAL_TIM_PWM_Start(dc_monitior_handler->speed_htim, dc_monitior_handler->channel);
}

void DC_Motor_Stop(DC_Monitor_TypeDef *dc_monitior_handler)
{
    HAL_TIM_PWM_Stop(dc_monitior_handler->speed_htim, dc_monitior_handler->channel);
}

void DC_Motor_SetSpeed(DC_Monitor_TypeDef *dc_monitior_handler, DC_Monitor_Speed speed)
{
    dc_monitior_handler->duty = speed;
    __HAL_TIM_SET_COMPARE(dc_monitior_handler->speed_htim, dc_monitior_handler->channel, speed);
}

void DC_Motor_GetEncodeNum(DC_Monitor_TypeDef *dc_monitior_handler)
{
    dc_monitior_handler->encode_num=__HAL_TIM_GET_COUNTER(dc_monitior_handler->encode_htim);
}