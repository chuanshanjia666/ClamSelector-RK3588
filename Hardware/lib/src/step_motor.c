#include "step_motor.h"
#include <stdio.h>
StepMotor_TypeDef StepMotor_id_1;
StepMotor_TypeDef StepMotor_id_2;
StepMotor_TypeDef StepMotor_id_3;
CAN_HandleTypeDef *stepmotor_can;
static HAL_StatusTypeDef STEP_Motor_Send(uint32_t id, uint8_t type, uint8_t len, uint8_t *msg)
{
    CAN_TxHeaderTypeDef g_can2_txheader;
    uint16_t t = 0;
    uint32_t TxMailbox = CAN_TX_MAILBOX0;

    g_can2_txheader.StdId = id;
    g_can2_txheader.IDE = CAN_ID_STD;
    g_can2_txheader.RTR = type;
    g_can2_txheader.DLC = len;

    while (HAL_CAN_GetTxMailboxesFreeLevel(stepmotor_can) < 1)
        ; /* 等待发送邮箱有空闲 */

    if (HAL_CAN_AddTxMessage(stepmotor_can, &g_can2_txheader, msg, &TxMailbox) != HAL_OK)
    {
        return HAL_ERROR;
    }
    while (HAL_CAN_GetTxMailboxesFreeLevel(stepmotor_can) != 3)
    {
        t++;

        if (t > 0xFFF)
        {
            HAL_CAN_AbortTxRequest(stepmotor_can, TxMailbox);
            return HAL_TIMEOUT;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef can_ctrl_stepmotor(StepMotor_TypeDef *stepmotor)
{

    uint8_t canbuf[7];
    HAL_StatusTypeDef res;
    canbuf[0] = stepmotor->mode;
    canbuf[1] = stepmotor->dir;
    canbuf[2] = stepmotor->seg;
    canbuf[3] = stepmotor->pos_h;
    canbuf[4] = stepmotor->pos_l;
    canbuf[5] = stepmotor->speed_h;
    canbuf[6] = stepmotor->speed_l;

    res = STEP_Motor_Send(stepmotor->id, CAN_RTR_DATA, 7, canbuf);
    if (res == HAL_ERROR)
    {
        printf("CAN send error for motor ID %d\n", stepmotor->id);
    }

    return res;
}

void StepMotor_Init(CAN_HandleTypeDef *hcanx)
{
    stepmotor_can = hcanx; // Initialize the CAN handle for step motors

    StepMotor_id_1.id = STEPMOTOR_1;
    StepMotor_id_1.mode = STEPMOTOR_POSITION_MODE;
    StepMotor_id_1.dir = STEPMOTOR_DIRECTION_CW;
    StepMotor_id_1.seg = STEPMOTOR_SEGMENT_32; // 32 segments
    StepMotor_id_1.pos_h = 0x03;               // Initial position high byte
    StepMotor_id_1.pos_l = 0x84;               // Initial position low byte
    StepMotor_id_1.speed_h = 0x00;             // Initial speed high byte
    StepMotor_id_1.speed_l = 0x64;             // Initial speed low byte

    StepMotor_id_2.id = STEPMOTOR_2;
    StepMotor_id_2.mode = STEPMOTOR_POSITION_MODE;
    StepMotor_id_2.dir = STEPMOTOR_DIRECTION_CW;
    StepMotor_id_2.seg = STEPMOTOR_SEGMENT_32; // 32 segments
    StepMotor_id_2.pos_h = 0x03;               // Initial position high byte
    StepMotor_id_2.pos_l = 0x84;               // Initial position low byte
    StepMotor_id_2.speed_h = 0x00;             // Initial speed high byte
    StepMotor_id_2.speed_l = 0x64;             // Initial speed low byte

    StepMotor_id_3.id = STEPMOTOR_3;
    StepMotor_id_3.mode = STEPMOTOR_POSITION_MODE;
    StepMotor_id_3.dir = STEPMOTOR_DIRECTION_CW;
    StepMotor_id_3.seg = STEPMOTOR_SEGMENT_32; // 32
    StepMotor_id_3.pos_h = 0x03;               // Initial position high byte
    StepMotor_id_3.pos_l = 0x84;               // Initial position low byte
    StepMotor_id_3.speed_h = 0x00;             // Initial speed high byte
    StepMotor_id_3.speed_l = 0x64;             // Initial speed low byte

    printf("Step motors initialized successfully\n"); // Print success message
}
