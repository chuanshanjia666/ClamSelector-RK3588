#ifndef __STEPMOTOR_H__
#define __STEPMOTOR_H__

#include "main.h"

#define    STEPMOTOR_1                      0x01 // Step motor 1
#define    STEPMOTOR_2                      0x02
#define    STEPMOTOR_3                      0x03

#define    STEPMOTOR_SPEED_MODE             0x01 // Speed mode
#define    STEPMOTOR_POSITION_MODE          0x02  // Position mode
#define    STEPMOTOR_MOMENT_MODE            0x03 // Moment mode
#define    STEPMOTOR_ABSOLUTE_ANGLE_MODE    0x04 // Absolute angle mode

#define    STEPMOTOR_DIRECTION_CCW           0x00 // Counterclockwise direction
#define    STEPMOTOR_DIRECTION_CW            0x01 // Clockwise direction

#define    STEPMOTOR_SEGMENT_2               0x02 // 2 segments
#define    STEPMOTOR_SEGMENT_4               0x04 // 4 segments
#define    STEPMOTOR_SEGMENT_8               0x08 // 8 segments
#define    STEPMOTOR_SEGMENT_16              0x10 // 16 segments
#define    STEPMOTOR_SEGMENT_32              0x20 // 32 segments

typedef struct {
    uint8_t id;           // Step motor ID
    uint8_t mode;   // Direction: 0 for forward, 1 for backward
    uint8_t dir; // Speed in steps per second
    uint8_t seg;  // Current segment
    uint8_t pos_h;
    uint8_t pos_l; 
    uint8_t speed_h; 
    uint8_t speed_l; 
} StepMotor_TypeDef;


extern StepMotor_TypeDef StepMotor_id_1;
extern StepMotor_TypeDef StepMotor_id_2;
extern StepMotor_TypeDef StepMotor_id_3;

HAL_StatusTypeDef can_ctrl_stepmotor(StepMotor_TypeDef *stepmotor);
HAL_StatusTypeDef StepMotor_Init(void);

#endif