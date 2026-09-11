#include "mypid.h"

float maxout = 100;
float i_out_limit = 500;
float Kp;
float Ki;
float Kd;
float target,actual,pid_out;

pid_t pid_x;//步进电机
pid_t pid_y;

pid_t pid_speed_left,pid_speed_right;
pid_t pid_location_left,pid_location_right;
void PID_INIT(void)
{
	/********************pid参数************************/													     
	PID_struct_init(&pid_x ,POSITION_PID , 3, 1, 0.02, 0, 0); 	//
	PID_struct_init(&pid_y ,POSITION_PID , 3, 1, 0.02, 0, 0); 	//
//	pid_x.input_deadband = TOLERANCE_CM_X * PIXELS_PER_CM_X;
//  pid_y.input_deadband = TOLERANCE_CM_Y * PIXELS_PER_CM_Y;
	
	PID_struct_init(&pid_speed_left ,DELTA_PID , 200, 50, 0.5, 0.2, 0); 	//速度环pid
	PID_struct_init(&pid_speed_right ,DELTA_PID , 200, 50, 0.5, 0.2, 0); 	//
	
	PID_struct_init(&pid_location_left ,POSITION_PID , 40, 0, 0.1, 0, 0); 	//位置外环pid
	PID_struct_init(&pid_location_right ,POSITION_PID , 40, 0, 0.1, 0, 0); 	//
}





void abs_limit(float *a, float ABS_MAX)
{
  if (*a > ABS_MAX)
    *a = ABS_MAX;
  if (*a < -ABS_MAX)
    *a = -ABS_MAX;
}

static void pid_param_init(
    pid_t *pid,
    uint32_t mode,
    uint32_t maxout,
    uint32_t intergral_limit,
    float kp,
    float ki,
    float kd)
{

  pid->integral_limit = intergral_limit;
  pid->max_out = maxout;
  pid->pid_mode = mode;

  pid->p = kp;
  pid->i = ki;
  pid->d = kd;
}
/**
 * @brief     modify pid parameter when code running
 * @param[in] pid: control pid struct
 * @param[in] p/i/d: pid parameter
 * @retval    none
 */
static void pid_reset(pid_t *pid, float kp, float ki, float kd)
{
  pid->p = kp;
  pid->i = ki;
  pid->d = kd;

  pid->pout = 0;
  pid->iout = 0;
  pid->dout = 0;
  pid->out = 0;
}

/**
 * @brief     calculate delta PID and position PID
 * @param[in] pid: control pid struct
 * @param[in] get: measure feedback value
 * @param[in] set: target value
 * @retval    pid calculate output
 */
float pid_calc(pid_t *pid, float get, float set ,uint8_t smoth)
{
	pid->get = get;
	pid->set = set;
//	if(smoth==0){
//	pid->err[NOW] = set - get;}	  
//	else{
//			float   pid_err;
//			pid_err= set - get;
//			pid->err[NOW] = pid_err*0.7+ pid->err[LAST]*0.3;
//	}	

    float raw_error = set - get; // 先计算原始误差

    // 应用输入死区
    // 如果原始误差的绝对值小于或等于 input_deadband，则将误差视为0
    if (pid->input_deadband != 0 && fabs(raw_error) <= pid->input_deadband)
    {
        pid->err[NOW] = 0;
        // 如果误差在死区内，通常需要清零积分项，以防止积分饱和（integral wind-up）
        pid->iout = 0; // 直接清零积分项
    }
    else
    {
        // 如果不在死区内，则根据 smoth 参数计算当前误差
        if (smoth == 0)
        {
            pid->err[NOW] = raw_error;
        }
        else
        {
            // 这里原来是 pid_err= set - get; pid->err[NOW] = pid_err*0.7+ pid->err[LAST]*0.3;
            // 现在应该是基于 raw_error 进行平滑
            pid->err[NOW] = raw_error * 0.7f + pid->err[LAST] * 0.3f;
        }
    }
 	
  if ((pid->input_max_err != 0) && (fabs(pid->err[NOW]) > pid->input_max_err))
    return 0;

  if (pid->pid_mode == POSITION_PID) // position PID
  {
    pid->pout = pid->p * pid->err[NOW];
	  	  
    pid->iout += pid->i * pid->err[NOW];
    abs_limit(&(pid->iout), pid->integral_limit);	
	  
    pid->dout = pid->d * (pid->err[NOW] - pid->err[LAST]);
	  	  
    pid->out = pid->pout + pid->iout + pid->dout;
    abs_limit(&(pid->out), pid->max_out);
  }
  else if (pid->pid_mode == DELTA_PID) // delta PID
  {
    pid->pout = pid->p * (pid->err[NOW] - pid->err[LAST]);
    pid->iout = pid->i * pid->err[NOW];
    pid->dout = pid->d * (pid->err[NOW] - 2 * pid->err[LAST] + pid->err[LLAST]);

    pid->out += pid->pout + pid->iout + pid->dout;
    abs_limit(&(pid->out), pid->max_out);
  }

	pid->err[LLAST] = pid->err[LAST];
	pid->err[LAST] = pid->err[NOW];

  if ((pid->output_deadband != 0) && (fabs(pid->out) < pid->output_deadband))
    return 0;
  else
    return pid->out;
}


//////////////////
//积分分离pid
float pid_calc_i_separation(pid_t *pid, float get, float set ,uint8_t smoth,float i_separationThreshold)
{
	pid->get = get;
	pid->set = set;
	if(smoth==0){
	pid->err[NOW] = set - get;}	  
	else{
			float   pid_err;
			pid_err= set - get;
			pid->err[NOW] = pid_err*0.7f+ pid->err[LAST]*0.3f;
	}	
  	
  if ((pid->input_max_err != 0) && (fabs(pid->err[NOW]) > pid->input_max_err))//输入误差超限处理
    return 0;

  if (pid->pid_mode == POSITION_PID) // position PID
  {
    pid->pout = pid->p * pid->err[NOW];
		//积分分离
		uint8_t i_separation_index=1;
			if(fabs(pid->err[NOW]) >= i_separationThreshold)
			{
				i_separation_index=0;
				// 方案1：积分衰减
					//系数越大 → 保留更多积分记忆
					//系数越小 → 更平滑但抗静差能力减弱
//				pid->iout *= 0.95;//在阈值外时积分每周期衰减//使用积分衰减时取消注释，系数根据实际情况选择
				
				// 方案2：过零检测复位（可选）//减少过冲，防止振荡
				
//				if (pid->err[NOW] * pid->err[LAST] < 0) 
//				{
////						if (sign_change_count >= 2) //多步过零检测
////							{
////								pid->iout *= 0.6;
////								sign_change_count = 0;
////							}
//					pid->iout *= 0.6;  // 过零时额外衰减//超调过大时，系数可减小；对于静差敏感时，系数可增大（0~1）
				
						/***************** 高级增强 *****************
						// 可选：添加变化率检测防噪声
						float delta_err = fabs(pid->err[NOW] - pid->err[LAST]);
						if (delta_err > noise_threshold) {
								pid->iout *= 0.6;
						}
						*******************************************/
//				}
			}
			else
			{
				// 方案3：渐变启用（可选）
				float ramp_factor = 1.0;
//				if (fabs(pid->err[NOW]) > i_separationThreshold * 0.7) 
//				{
//					ramp_factor = (i_separationThreshold - fabs(pid->err[NOW])) / (0.3 * i_separationThreshold);
//				}
				
				i_separation_index=1;
				pid->iout += ramp_factor * pid->i * pid->err[NOW];
			}
		//注意积分限幅值，不要太大，避免进入阈值范围内时突然的积分作用导致较大的抖动//如果确实需要积分限幅比较大导致较大的抖动，可以引入积分衰减或渐变启动
	  abs_limit(&(pid->iout), pid->integral_limit);
    pid->dout = pid->d * (pid->err[NOW] - pid->err[LAST]);
	  	  
    pid->out = pid->pout + i_separation_index*pid->iout + pid->dout;
    abs_limit(&(pid->out), pid->max_out);
  }
  else if (pid->pid_mode == DELTA_PID) // delta PID
  {
    pid->pout = pid->p * (pid->err[NOW] - pid->err[LAST]);
		//积分分离
		uint8_t i_separation_index=1;
			if(fabs(pid->err[NOW]) >= i_separationThreshold)
			{
				i_separation_index=0;
			}
			else
			{
				i_separation_index=1;
				pid->iout = pid->i * pid->err[NOW];
			}
    pid->dout = pid->d * (pid->err[NOW] - 2 * pid->err[LAST] + pid->err[LLAST]);

    pid->out += pid->pout + i_separation_index*pid->iout + pid->dout;
    abs_limit(&(pid->out), pid->max_out);
  }

	pid->err[LLAST] = pid->err[LAST];
	pid->err[LAST] = pid->err[NOW];

  if ((pid->output_deadband != 0) && (fabs(pid->out) < pid->output_deadband))//输出死区处理
    return 0;
  else
    return pid->out;
}
////////////////
//微分先行pid
float pid_calc_d(pid_t *pid, float get, float set ,float actual,float last_actual,uint8_t smoth)
{
	pid->get = get;
	pid->set = set;
	if(smoth==0){
	pid->err[NOW] = set - get;}	  
	else{
			float   pid_err;
			pid_err= set - get;
			pid->err[NOW] = pid_err*0.7f+ pid->err[LAST]*0.3f;
	}	
  	
  if ((pid->input_max_err != 0) && (fabs(pid->err[NOW]) > pid->input_max_err))
    return 0;

  if (pid->pid_mode == POSITION_PID) // position PID
  {
    pid->pout = pid->p * pid->err[NOW];
	  	  
    pid->iout += pid->i * pid->err[NOW];

    abs_limit(&(pid->iout), pid->integral_limit);	
		//微分先行
		pid->dout = - pid->d * (actual -last_actual);

    pid->out = pid->pout + pid->iout + pid->dout;
    abs_limit(&(pid->out), pid->max_out);
  }

	pid->err[LLAST] = pid->err[LAST];
	pid->err[LAST] = pid->err[NOW];

  if ((pid->output_deadband != 0) && (fabs(pid->out) < pid->output_deadband))
    return 0;
  else
    return pid->out;
}


////////////////
float pid_angle_calc(pid_t *pid, float get, float set ,uint8_t smoth)//0~360
{
	pid->get = get;
	pid->set = set;
	if(smoth==0){
	pid->err[NOW] = set - get;}	  
	else{
			float   pid_err;
			pid_err= set - get;
			pid->err[NOW] = pid_err*0.7f+ pid->err[LAST]*0.3f;
	}	
  	
  if ((pid->input_max_err != 0) && (fabs(pid->err[NOW]) > pid->input_max_err))
    return 0;
	
  if (pid->pid_mode == POSITION_PID) // position PID
  {
		float e = pid->err[NOW];
		e = fmod(e + 180.0f, 360.0f);
		if (e < 0) e += 360.0f;
		pid->err[NOW] = e - 180.0f;
		
    pid->pout = pid->p * pid->err[NOW];
	  	  
    pid->iout += pid->i * pid->err[NOW];
    abs_limit(&(pid->iout), pid->integral_limit);	
	  
    pid->dout = pid->d * (pid->err[NOW] - pid->err[LAST]);
	  	  
    pid->out = pid->pout + pid->iout + pid->dout;
    abs_limit(&(pid->out), pid->max_out);
  }

	pid->err[LLAST] = pid->err[LAST];
	pid->err[LAST] = pid->err[NOW];

  if ((pid->output_deadband != 0) && (fabs(pid->out) < pid->output_deadband))
    return 0;
  else
    return pid->out;
}

float pid_yaw_calc(pid_t *pid, float get, float set ,uint8_t smoth)//-180~180
{
	pid->get = get;
	pid->set = set;
	if(smoth==0)
	{
		pid->err[NOW] = set - get;
	}	  
	else
	{
			float   pid_err;
			pid_err= set - get;
			pid->err[NOW] = pid_err*0.7f+ pid->err[LAST]*0.3f;
	}	
  	
  if ((pid->input_max_err != 0) && (fabs(pid->err[NOW]) > pid->input_max_err))
    return 0;

	if(pid->err[NOW]>180) {pid->err[NOW] = -(360-pid->err[NOW]);}//陀螺仪在180度处跳变为-180，对此进行补偿处理
	else if(pid->err[NOW]<-180) {pid->err[NOW] = 360+pid->err[NOW];}
  if (pid->pid_mode == POSITION_PID) // position PID
  {
    pid->pout = pid->p * pid->err[NOW];
	  	  
    pid->iout += pid->i * pid->err[NOW];
    abs_limit(&(pid->iout), pid->integral_limit);	
	  
    pid->dout = pid->d * (pid->err[NOW] - pid->err[LAST]);
	  	  
    pid->out = pid->pout + pid->iout + pid->dout;
    abs_limit(&(pid->out), pid->max_out);
  }
	
	pid->err[LLAST] = pid->err[LAST];
	pid->err[LAST] = pid->err[NOW];

  if ((pid->output_deadband != 0) && (fabs(pid->out) < pid->output_deadband))
    return 0;
  else
    return pid->out;
}
///////////////////
void pid_clear(pid_t *pid)
{
  pid->pout = 0;
  pid->iout = 0;
  pid->dout = 0;
  pid->out = 0;
}

/**
 * @brief     initialize pid parameter
 * @retval    none
 */
void PID_struct_init(
    pid_t *pid,
    uint32_t mode,
    uint32_t maxout,
    uint32_t intergral_limit,

    float kp,
    float ki,
    float kd)
{
  pid->f_param_init = pid_param_init;
  pid->f_pid_reset = pid_reset;

  pid->f_param_init(pid, mode, maxout, intergral_limit, kp, ki, kd);
  pid->f_pid_reset(pid, kp, ki, kd);
}

///**
// * @brief     calculate position PID
// * @param[in] pid: control pid struct
// * @param[in] get: measure feedback value
// * @param[in] set: target value
// * @retval    pid calculate output
// */
//float position_pid_calc(pid_t *pid, float get, float set)
//{
//  pid->get = get;
//  pid->set = set;
//  pid->err[NOW] = set - get;
//  pid->pout = pid->p * pid->err[NOW];
//  pid->iout += pid->i * pid->err[NOW];
//  pid->dout = pid->d * (pid->err[NOW] - pid->err[LAST]);
//  abs_limit(&(pid->iout), pid->integral_limit);
//  pid->out = pid->pout + pid->iout + pid->dout;
//  abs_limit(&(pid->out), pid->max_out);

//  pid->err[LLAST] = pid->err[LAST];
//  pid->err[LAST] = pid->err[NOW];
//  return pid->out;
//}


