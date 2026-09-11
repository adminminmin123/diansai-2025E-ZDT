#include "oled_bsp.h"
#include "oled.h"
// 假设 OLED 宽度为 128 像素，使用 6x8 字体
// 每行 8 像素高，最多 64/8 = 8 行 (y=0~7) 或 32/8 = 4 行 (y=0~3)
// 每列 6 像素宽，最多 128/6 = 21 个字符 (x=0~20? 驱动库可能基于像素位置)
// **注意:** Oled_Printf 的 x, y 参数单位需要参考 OLED_ShowStr 实现，可能是字符位置或像素位置
// 文档中的注释 (0-127, 0-3) 暗示可能是 128x32 屏幕的像素 x 坐标和字符行 y 坐标

/**
 * @brief	使用类似printf的方式显示字符串，显示6x8大小的ASCII字符
 * @param x  起始 X 坐标 (像素) 或 字符列位置 (需要看 OLED_ShowStr)
 * @param y  起始 Y 坐标 (像素) 或 字符行位置 (需要看 OLED_ShowStr, 0-3 或 0-7)
 * @param format, ... 格式化字符串及参数
 * 例如：Oled_Printf(0, 0, "Data = %d", dat);
**/
int Oled_Printf(uint8_t x, uint8_t y, const char *format, ...)
{
	char buffer[128]; // 缓冲区大小根据需要调整
	va_list arg;
	int len;

	va_start(arg, format);
	len = vsnprintf(buffer, sizeof(buffer), format, arg);
	va_end(arg);

	// 假设 OLED_ShowStr 使用像素坐标 x 和字符行 y
	// 并且字体大小是 8 (高度)
	OLED_ShowStr(x, y * 8, (uint8_t*)buffer, 8); // 将 buffer 转为 uint8_t*
	return len;
}

void oled_proc(void)
{
	
}

