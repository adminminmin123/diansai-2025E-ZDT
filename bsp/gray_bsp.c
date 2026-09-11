#include "gray_bsp.h"

unsigned char Digtal;

void gray_proc(void)
{
	Digtal = IIC_Get_Digtal();
	my_printf(&huart1, "Digtal %d-%d-%d-%d-%d-%d-%d-%d\r\n",(Digtal>>0)&0x01,(Digtal>>1)&0x01,(Digtal>>2)&0x01,(Digtal>>3)&0x01,(Digtal>>4)&0x01,(Digtal>>5)&0x01,(Digtal>>6)&0x01,(Digtal>>7)&0x01);
}
