#ifndef __STEP_MOTOR_BSP_H__
#define __STEP_MOTOR_BSP_H__

#include "bsp_system.h"

/* 电机控制宏定义 */
#define MOTOR_X_ADDR        0x01          // X轴电机地址
#define MOTOR_Y_ADDR        0x01          // Y轴电机地址
#define MOTOR_X_UART        huart2        // X轴电机串口 xia
#define MOTOR_Y_UART        huart4        // Y轴电机串口 shang
#define MOTOR_MAX_SPEED     3            // 电机最大转速(RPM)
#define MOTOR_ACCEL         0             // 电机加速度(0表示直接启动)
#define MOTOR_SYNC_FLAG     false         // 电机同步标志
#define MOTOR_MAX_ANGLE     50            // 电机最大角度限制(±50°)

/* 函数声明 */
void Step_Motor_Init(void);                    // 电机初始化
void Step_Motor_Set_Speed(int8_t x_percent, int8_t y_percent);  // 设置XY电机速度(百分比)
void Step_Motor_Set_Speed_my(float x_rpm, float y_rpm);
void Step_Motor_Stop(void);                    // 停止所有电机
void step_motor_proc(void);
void Step_Motor_Set_Pwm(int32_t x_distance, int32_t y_distance);

#endif
