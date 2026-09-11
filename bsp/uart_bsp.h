#ifndef __UART_BSP_H__
#define __UART_BSP_H__

#include "bsp_system.h"

void uart_proc(void);
int my_printf(UART_HandleTypeDef *huart, const char *format, ...);

// 保存初始位置信息
void save_initial_position(void);

#endif
