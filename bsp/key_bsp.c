#include "key_bsp.h"

uint8_t key_val = 0;
uint8_t key_old = 0;
uint8_t key_down = 0;
uint8_t key_up = 0;

uint8_t key_read(void)
{
	uint8_t temp = 0;
	
//	if(HAL_GPIO_ReadPin(KEY1_GPIO_Port,KEY1_Pin) == GPIO_PIN_RESET)
	if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0) == GPIO_PIN_RESET)
		temp = 1;
	if(HAL_GPIO_ReadPin(KEY2_GPIO_Port,KEY2_Pin) == GPIO_PIN_RESET)
		temp = 2;
	if(HAL_GPIO_ReadPin(KEY3_GPIO_Port,KEY3_Pin) == GPIO_PIN_RESET)
		temp = 3;
	if(HAL_GPIO_ReadPin(KEY4_GPIO_Port,KEY4_Pin) == GPIO_PIN_RESET)
		temp = 4;
	return temp;
}

void key_proc(void)
{
	key_val = key_read();
	key_down = key_val & (key_val ^ key_old);
	key_up = ~key_val & (key_val ^ key_old);
	key_old = key_val;
	if(key_down==1)
	{
//		if(++set_index == 3)
//		{
//			set_index = 0;
//		}
//		pid_location_left.set += 1560;
//		pid_location_right.set += 1560;
	}
}
