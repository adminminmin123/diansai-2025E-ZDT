#ifndef __OLED_BSP_H__
#define __OLED_BSP_H__

#include "bsp_system.h"

int Oled_Printf(uint8_t x, uint8_t y, const char *format, ...);
void oled_proc(void);

#endif
