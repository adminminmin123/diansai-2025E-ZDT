#include "encoder_bsp.h"

// 左右编码器电机
Encoder left_encoder;
Encoder right_encoder;

/**
 * @brief 初始化编码器应用
 */
void encoder_init(void)
{
  Encoder_Driver_Init(&left_encoder, &htim3, 0);
  Encoder_Driver_Init(&right_encoder, &htim4, 1);
}

/**
 * @brief 编码器应用运行任务 (应由调度器周期性调用)
 */
void encoder_proc(void)
{
  Encoder_Driver_Update(&left_encoder);
  Encoder_Driver_Update(&right_encoder);
	
}
