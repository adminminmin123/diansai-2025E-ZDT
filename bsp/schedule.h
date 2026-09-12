#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include "bsp_system.h"

// 前向声明激光处理函数
void Laser_Process(void);

// 前向声明Y轴回零处理函数
void Y_Axis_Homing_Process(void);

void schedule_init(void);
void schedule_run(void);

#endif
