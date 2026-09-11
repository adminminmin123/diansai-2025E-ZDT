#include "motor_bsp.h"

Motor_t motor1;  // 左侧电机
Motor_t motor2;  // 右侧电机

float speed_set[3] = {-65,65,0};
uint8_t set_index = 0;

void motor_init(void)
{
	// 创建电机1 - 左侧电机
	Motor_Create(&motor1, 
             &htim1,           // TIM1定时器
             TIM_CHANNEL_2,    // 通道2 (PE11)
						 GPIOE,            // GPIOE端口
             GPIO_PIN_9,       // PE9引脚
             GPIOE,            // GPIOE端口
             GPIO_PIN_10,       // PE9引脚
             GPIO_PIN_SET);    // 正转时输出高电平

	// 创建电机2 - 右侧电机
	Motor_Create(&motor2, 
             &htim1,           // TIM1定时器
             TIM_CHANNEL_4,    // 通道4 (PE14)
             GPIOE,            // GPIOE端口
             GPIO_PIN_12,      // PE13引脚
	           GPIOE,            // GPIOE端口
             GPIO_PIN_13,      // PE13引脚
             GPIO_PIN_SET);    // 正转时输出高电平
}

float pid_speed_left_out ,pid_speed_right_out;//速度环pid计算值

void motor_proc(void)
{
//    //电机前进
//    Motor_SetSpeed(&motor1, 50);   // 左电机50%速度
//    Motor_SetSpeed(&motor2, 50);   // 右电机50%速度
//    
//    //电机停止
//    Motor_Stop(&motor1);
//    Motor_Stop(&motor2);

//	pid_speed_left_out = pid_calc(&pid_speed_left,left_encoder.speed_cm_s,speed_set[set_index],0);
//	Motor_SetSpeed(&motor1, pid_speed_left_out);
	
//	pid_speed_right_out = pid_calc(&pid_speed_right,right_encoder.speed_cm_s,speed_set[set_index],0);
//	Motor_SetSpeed(&motor2, pid_speed_right_out);
//	my_printf(&huart1,"%.1f,%.1f\r\n",right_encoder.speed_cm_s,pid_speed_right_out);
	
	Location_Speed_control();
	
}


float location_left_Outval,location_right_Outval;//位置环pid计算值
void Location_Speed_control(void) 
{
	/*速度环*/
	location_left_Outval  =  pid_calc(&pid_location_left, left_encoder.total_count  ,pid_location_left.set ,0 );
	location_right_Outval =  pid_calc(&pid_location_right,right_encoder.total_count ,pid_location_right.set ,0 );
	/*位置环*/
	pid_speed_left_out  = pid_calc(&pid_speed_left,left_encoder.speed_cm_s,location_left_Outval,0);
	pid_speed_right_out = pid_calc(&pid_speed_right,right_encoder.speed_cm_s,location_right_Outval,0);
	
	Motor_SetSpeed(&motor1, pid_speed_left_out);
	Motor_SetSpeed(&motor2, pid_speed_right_out);
}








