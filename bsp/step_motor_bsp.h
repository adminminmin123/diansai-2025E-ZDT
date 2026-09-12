
#ifndef __STEP_MOTOR_BSP_H__
#define __STEP_MOTOR_BSP_H__

#include "bsp_system.h"

/* Motor Configuration */
#define MOTOR_X_ADDR 0x01     // X motor address
#define MOTOR_Y_ADDR 0x02     // Y motor address
#define MOTOR_X_UART huart2   // X motor UART (horizontal)
#define MOTOR_Y_UART huart4   // Y motor UART (vertical)
#define MOTOR_MAX_SPEED 2000  // Maximum motor speed (RPM) - 进一步提高速度
#define MOTOR_ACCEL 100       // Motor acceleration (100 = 更快加速)
#define MOTOR_SYNC_FLAG false // Motor synchronization flag
#define MOTOR_MAX_ANGLE 50    // Maximum rotation angle (±50°)

/* Function Declarations */
void Step_Motor_Init(void);                                    // Initialize motors
void Step_Motor_Set_Speed(int8_t x_percent, int8_t y_percent); // Set XY motor speed (percentage)
void Step_Motor_Set_Speed_my(float x_rpm, float y_rpm);        // Set motor speed in RPM
void Step_Motor_Stop(void);                                    // Stop all motors
void step_motor_proc(void);
void Step_Motor_Set_Pwm(int32_t x_distance, int32_t y_distance);
void Step_Motor_Rotate_X_Angle(int16_t angle);                   // Rotate X motor by specific angle
void Step_Motor_Rotate_Y_Angle(int16_t angle);                   // Rotate Y motor by specific angle
uint32_t calculate_rotation_time(int16_t angle, uint16_t speed); // Calculate rotation time

#endif
