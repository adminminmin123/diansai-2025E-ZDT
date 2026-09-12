#ifndef __KEY_BSP_H__
#define __KEY_BSP_H__

#include "bsp_system.h"

/* 按键处理函数 */
void key_proc(void);
uint8_t key_read(void);

/* 按键控制函数 */
void key_laser_manual_control(void);
void key_mode_switch_control(void);

/* 步进电机角度控制按键处理函数 */
void key_motor_angle_control(void);
uint8_t read_angle_keys(void);

/* 调试和测试函数 */
void test_all_angle_keys(void);
void test_motor_basic_communication(void);

/* 角度累积控制函数 */
void reset_angle_accumulation(void);
void get_current_angle_status(void);

/* 角度控制相关定义 */
#define ANGLE_22_5 22.5f // 基础角度22.5度

/* X轴按键值定义 */
#define KEY_LEFT_22_5 1  // 左转22.5度按键
#define KEY_RIGHT_22_5 2 // 右转22.5度按键
#define KEY_START 3      // 启动旋转按键

/* Y轴按键值定义 */
#define KEY_Y_DOWN_45 4  // Y轴向下45度按键
#define KEY_Y_DOWN_135 5 // Y轴向下135度按键

/* 角度累积相关定义 */
#define ANGLE_45 36.0f                // 45度角度
#define ANGLE_135 125.0f              // 135度角度
#define MAX_ANGLE_ACCUMULATION 360.0f // 最大累积角度
#define ANGLE_RESET_THRESHOLD 0.1f    // 角度重置阈值

/* Y轴控制函数 */
uint8_t read_y_axis_keys(void);
void key_motor_y_axis_control(void);
void set_y_axis_angle(float angle);
float get_y_axis_angle(void);

#endif
