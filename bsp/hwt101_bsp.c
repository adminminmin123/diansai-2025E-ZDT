#include "hwt101_bsp.h"

HWT101_t hwt101;  // HWT101陀螺仪实体

void hwt101_init(void)
{
	// 创建HWT101实体
	HWT101_Create(&hwt101, &huart3, 1000);  // 使用USART2，超时1000ms
	// 初始化区域启动DMA接收
	HAL_UART_Receive_DMA(&huart3, uart3_rx_buffer, sizeof(uart3_rx_buffer));
}

void hwt101_proc(void)
{
    // 读取陀螺仪数据
    float yaw = HWT101_GetYaw(&hwt101);           // 获取偏航角
    float gyro_z = HWT101_GetGyroZ(&hwt101);      // 获取角速度Z
    
    // 获取完整数据
    HWT101_Data_t* data = HWT101_GetData(&hwt101);
    if (data != NULL)
    {
        my_printf(&huart1,"Yaw: %.2f°, GyroZ: %.2f°/s\r\n", data->yaw, data->gyro_z);
    }
}
