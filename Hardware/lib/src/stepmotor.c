#include "stepmotor.h"


StepMotor_TypeDef StepMotor_id_1;
StepMotor_TypeDef StepMotor_id_2;
StepMotor_TypeDef StepMotor_id_3;

HAL_StatusTypeDef can_ctrl_stepmotor(StepMotor_TypeDef *stepmotor){
    uint8_t canbuf[7];
    HAL_StatusTypeDef res;
    canbuf[0] = stepmotor->mode;
    canbuf[1] = stepmotor->dir;
    canbuf[2] = stepmotor->seg;
    canbuf[3] = stepmotor->pos_h;
    canbuf[4] = stepmotor->pos_l;
    canbuf[5] = stepmotor->speed_h;
    canbuf[6] = stepmotor->speed_l;
    res = can2_send_msg(stepmotor->id, CAN_RTR_DATA, 7, canbuf); // Send CAN message
    if (res == HAL_ERROR) {
        printf("CAN send error for motor ID %d\n", stepmotor->id); // Error handling
    } else {
        printf("CAN send success for motor ID %d\n", stepmotor->id); // Success message
    }
    return res;
}

HAL_StatusTypeDef StepMotor_Init(void){
    
    StepMotor_id_1.id = STEPMOTOR_1;
    StepMotor_id_1.mode = STEPMOTOR_POSITION_MODE;
    StepMotor_id_1.dir = STEPMOTOR_DIRECTION_CW;
    StepMotor_id_1.seg = STEPMOTOR_SEGMENT_32; // 32 segments
    StepMotor_id_1.pos_h = 0x03; // Initial position high byte
    StepMotor_id_1.pos_l = 0x84; // Initial position low byte
    StepMotor_id_1.speed_h = 0x00; // Initial speed high byte
    StepMotor_id_1.speed_l = 0x64; // Initial speed low byte
    
    StepMotor_id_2.id = STEPMOTOR_2;
    StepMotor_id_2.mode = STEPMOTOR_POSITION_MODE;
    StepMotor_id_2.dir = STEPMOTOR_DIRECTION_CW;
    StepMotor_id_2.seg = STEPMOTOR_SEGMENT_32; // 32 segments
    StepMotor_id_2.pos_h = 0x0e; // Initial position high byte
    StepMotor_id_2.pos_l = 0x10; // Initial position low byte
    StepMotor_id_2.speed_h = 0x00; // Initial speed high byte
    StepMotor_id_2.speed_l = 0x64; // Initial speed low byte

    StepMotor_id_3.id = STEPMOTOR_3;
    StepMotor_id_3.mode = STEPMOTOR_POSITION_MODE;
    StepMotor_id_3.dir = STEPMOTOR_DIRECTION_CW;
    StepMotor_id_3.seg = STEPMOTOR_SEGMENT_32; // 32
    StepMotor_id_3.pos_h = 0x07; // Initial position high byte
    StepMotor_id_3.pos_l = 0x08; // Initial position low byte
    StepMotor_id_3.speed_h = 0x00; // Initial speed high byte
    StepMotor_id_3.speed_l = 0x64; // Initial speed low byte
    
    if (can_ctrl_stepmotor(&StepMotor_id_1) || 
        can_ctrl_stepmotor(&StepMotor_id_2) || 
        can_ctrl_stepmotor(&StepMotor_id_3) == HAL_ERROR) 
    {
        return HAL_ERROR; // Return error if any motor initialization fails
    }   
    printf("Step motors initialized successfully\n"); // Print success message
    return HAL_OK; // Return success status
}
