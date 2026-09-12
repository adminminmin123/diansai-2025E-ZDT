#include "mypid.h"

float maxout = 100;
float i_out_limit = 500;
float Kp;
float Ki;
float Kd;
float target, actual, pid_out;

// ???????
pid_t pid_x;
pid_t pid_y;

// ??????
pid_t pid_speed_left, pid_speed_right;
pid_t pid_location_left, pid_location_right;

void PID_INIT(void)
{
  // ??PID?? - ????????
  PID_struct_init(&pid_x, POSITION_PID, 30, 1.5, 0.035, 0.00015, 0.012); // ???30 RPM
  PID_struct_init(&pid_y, POSITION_PID, 30, 1.5, 0.035, 0.00015, 0.012); // ???30 RPM

  // ??????????????
  pid_x.input_deadband = 6; // ????????
  pid_y.input_deadband = 6; // ????????
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
float pid_calc(pid_t *pid, float get, float set, uint8_t smoth)
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

  float raw_error = set - get; // ??????????

  // ???????????
  // ??????????????????? input_deadband??????????0
  if (pid->input_deadband != 0 && fabs(raw_error) <= pid->input_deadband)
  {
    pid->err[NOW] = 0;
    // ??????????????????????????????????????????integral wind-up??
    pid->iout = 0; // ????????????
  }
  else
  {
    // ??????????????????? smoth ????????????
    if (smoth == 0)
    {
      pid->err[NOW] = raw_error;
    }
    else
    {
      // ????????? pid_err= set - get; pid->err[NOW] = pid_err*0.7+ pid->err[LAST]*0.3;
      // ???????????? raw_error ???????
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
// ???????pid
float pid_calc_i_separation(pid_t *pid, float get, float set, uint8_t smoth, float i_separationThreshold)
{
  pid->get = get;
  pid->set = set;
  if (smoth == 0)
  {
    pid->err[NOW] = set - get;
  }
  else
  {
    float pid_err;
    pid_err = set - get;
    pid->err[NOW] = pid_err * 0.7f + pid->err[LAST] * 0.3f;
  }

  if ((pid->input_max_err != 0) && (fabs(pid->err[NOW]) > pid->input_max_err)) // ????????????
    return 0;

  if (pid->pid_mode == POSITION_PID) // position PID
  {
    pid->pout = pid->p * pid->err[NOW];
    // ???????
    uint8_t i_separation_index = 1;
    if (fabs(pid->err[NOW]) >= i_separationThreshold)
    {
      i_separation_index = 0;
      // ????1?????????
      // ?????? ?? ??????????????
      // ???? ?? ?????????????????????
      //				pid->iout *= 0.95;//????????????????????//?????????????????????????????????

      // ????2???????????????//???????????

      //				if (pid->err[NOW] * pid->err[LAST] < 0)
      //				{
      ////						if (sign_change_count >= 2) //????????
      ////							{
      ////								pid->iout *= 0.6;
      ////								sign_change_count = 0;
      ////							}
      //					pid->iout *= 0.6;  // ????????????//??????????????????????????????????????????0~1??

      /***************** ?????? *****************
      // ??????????????????
      float delta_err = fabs(pid->err[NOW] - pid->err[LAST]);
      if (delta_err > noise_threshold) {
          pid->iout *= 0.6;
      }
      *******************************************/
      //				}
    }
    else
    {
      // ????3????????????????
      float ramp_factor = 1.0;
      //				if (fabs(pid->err[NOW]) > i_separationThreshold * 0.7)
      //				{
      //					ramp_factor = (i_separationThreshold - fabs(pid->err[NOW])) / (0.3 * i_separationThreshold);
      //				}

      i_separation_index = 1;
      pid->iout += ramp_factor * pid->i * pid->err[NOW];
    }
    // ????????????????????????????????????????????????????//??????????????????????????????????????????????????
    abs_limit(&(pid->iout), pid->integral_limit);
    pid->dout = pid->d * (pid->err[NOW] - pid->err[LAST]);

    pid->out = pid->pout + i_separation_index * pid->iout + pid->dout;
    abs_limit(&(pid->out), pid->max_out);
  }
  else if (pid->pid_mode == DELTA_PID) // delta PID
  {
    pid->pout = pid->p * (pid->err[NOW] - pid->err[LAST]);
    // ???????
    uint8_t i_separation_index = 1;
    if (fabs(pid->err[NOW]) >= i_separationThreshold)
    {
      i_separation_index = 0;
    }
    else
    {
      i_separation_index = 1;
      pid->iout = pid->i * pid->err[NOW];
    }
    pid->dout = pid->d * (pid->err[NOW] - 2 * pid->err[LAST] + pid->err[LLAST]);

    pid->out += pid->pout + i_separation_index * pid->iout + pid->dout;
    abs_limit(&(pid->out), pid->max_out);
  }

  pid->err[LLAST] = pid->err[LAST];
  pid->err[LAST] = pid->err[NOW];

  if ((pid->output_deadband != 0) && (fabs(pid->out) < pid->output_deadband)) // ???????????
    return 0;
  else
    return pid->out;
}
////////////////
// ???????pid
float pid_calc_d(pid_t *pid, float get, float set, float actual, float last_actual, uint8_t smoth)
{
  pid->get = get;
  pid->set = set;
  if (smoth == 0)
  {
    pid->err[NOW] = set - get;
  }
  else
  {
    float pid_err;
    pid_err = set - get;
    pid->err[NOW] = pid_err * 0.7f + pid->err[LAST] * 0.3f;
  }

  if ((pid->input_max_err != 0) && (fabs(pid->err[NOW]) > pid->input_max_err))
    return 0;

  if (pid->pid_mode == POSITION_PID) // position PID
  {
    pid->pout = pid->p * pid->err[NOW];

    pid->iout += pid->i * pid->err[NOW];

    abs_limit(&(pid->iout), pid->integral_limit);
    // ???????
    pid->dout = -pid->d * (actual - last_actual);

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
float pid_angle_calc(pid_t *pid, float get, float set, uint8_t smoth) // 0~360
{
  pid->get = get;
  pid->set = set;
  if (smoth == 0)
  {
    pid->err[NOW] = set - get;
  }
  else
  {
    float pid_err;
    pid_err = set - get;
    pid->err[NOW] = pid_err * 0.7f + pid->err[LAST] * 0.3f;
  }

  if ((pid->input_max_err != 0) && (fabs(pid->err[NOW]) > pid->input_max_err))
    return 0;

  if (pid->pid_mode == POSITION_PID) // position PID
  {
    float e = pid->err[NOW];
    e = fmod(e + 180.0f, 360.0f);
    if (e < 0)
      e += 360.0f;
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

float pid_yaw_calc(pid_t *pid, float get, float set, uint8_t smoth) //-180~180
{
  pid->get = get;
  pid->set = set;
  if (smoth == 0)
  {
    pid->err[NOW] = set - get;
  }
  else
  {
    float pid_err;
    pid_err = set - get;
    pid->err[NOW] = pid_err * 0.7f + pid->err[LAST] * 0.3f;
  }

  if ((pid->input_max_err != 0) && (fabs(pid->err[NOW]) > pid->input_max_err))
    return 0;

  if (pid->err[NOW] > 180)
  {
    pid->err[NOW] = -(360 - pid->err[NOW]);
  } // ????????180????????-180??????????????
  else if (pid->err[NOW] < -180)
  {
    pid->err[NOW] = 360 + pid->err[NOW];
  }
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
// float position_pid_calc(pid_t *pid, float get, float set)
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
