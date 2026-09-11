#ifndef  __MYPID_H__
#define  __MYPID_H__

/* Standard Includes */
#include "bsp_system.h"


//typedef unsigned char uint8;       // 无符号  8 bits
//typedef unsigned short int uint16; // 无符号 16 bits
//// typedef unsigned long int uint32;  // 无符号 32 bits
//// typedef unsigned long long uint64; // 无符号 64 bits

//typedef char int8;       // 有符号  8 bits
//typedef short int int16; // 有符号 16 bits
//// typedef long int int32;  // 有符号 32 bits
//// typedef long long int64; // 有符号 64 bits

//typedef volatile uint8 vuint8;   // 易变性修饰 无符号  8 bits
//typedef volatile uint16 vuint16; // 易变性修饰 无符号 16 bits
//// typedef volatile uint32 vuint32; // 易变性修饰 无符号 32 bits
//// typedef volatile uint64 vuint64; // 易变性修饰 无符号 64 bits

//typedef volatile int8 vint8;   // 易变性修饰 有符号  8 bits
//typedef volatile int16 vint16; // 易变性修饰 有符号 16 bits
//// typedef volatile int32 vint32; // 易变性修饰 有符号 32 bits
//// typedef volatile int64 vint64; // 易变性修饰 有符号 64 bits
//// #define uint32_t uint16
enum
{
  LLAST = 0,
  LAST,
  NOW,
  POSITION_PID,
  DELTA_PID,
};
typedef struct pid_t
{
  float p;
  float i;
  float d;

  float set;
  float get;
  float err[3];

  float pout;
  float iout;
  float dout;
  float out;

  float input_max_err;   // input max err;
  float output_deadband; // output deadband;
  float input_deadband; // 当 |误差| <= input_deadband 时，认为误差为0

  uint32_t pid_mode;
  uint32_t max_out;
  uint32_t integral_limit;

  void (*f_param_init)(struct pid_t *pid,
                       uint32_t pid_mode,
                       uint32_t max_output,
                       uint32_t inte_limit,
                       float p,
                       float i,
                       float d);
  void (*f_pid_reset)(struct pid_t *pid, float p, float i, float d);

} pid_t;

extern pid_t pid_motor_left;
extern pid_t pid_motor_right;
extern pid_t pid_left_location;
extern pid_t pid_right_location;


#if 0
#define PID_PARAM_DEFAULT \
  {                       \
    0,                    \
        0,                \
        0,                \
        0,                \
        0,                \
        {0, 0, 0},        \
        0,                \
        0,                \
        0,                \
        0,                \
        0,                \
        0,                \
  }\

typedef struct
{
  float p;
  float i;
  float d;

  float set;
  float get;
  float err[3]; //error

  float pout; 
  float iout; 
  float dout; 
  float out;

  float input_max_err;    //input max err;
  float output_deadband;  //output deadband; 

  float p_far;
  float p_near;
  float grade_range;
  
  uint32_t pid_mode;
  uint32_t max_out;
  uint32_t integral_limit;

  void (*f_param_init)(struct pid_t *pid, 
                       uint32_t      pid_mode,
                       uint32_t      max_output,
                       uint32_t      inte_limit,
                       float         p,
                       float         i,
                       float         d);
  void (*f_pid_reset)(struct pid_t *pid, float p, float i, float d);
 
} grade_pid_t;
#endif

void PID_struct_init(
    pid_t *pid,
    uint32_t mode,
    uint32_t maxout,
    uint32_t intergral_limit,

    float kp,
    float ki,
    float kd);
void PID_INIT(void);
float pid_calc(pid_t *pid, float get, float set ,uint8_t smoth);
float pid_angle_calc(pid_t *pid, float get, float set ,uint8_t smoth);
float pid_yaw_calc(pid_t *pid, float get, float set ,uint8_t smoth);
float pid_calc_i_separation(pid_t *pid, float get, float set ,uint8_t smoth,float i_separation_value);
float pid_calc_d(pid_t *pid, float get, float set ,float actual,float last_actual,uint8_t smoth);
void pid_clear(pid_t *pid);
//float position_pid_calc(pid_t *pid, float fdb, float ref);
//void ControlLoop(void);

extern pid_t pid_x;
extern pid_t pid_y;
		
extern pid_t pid_speed_left,pid_speed_right;
extern pid_t pid_location_left,pid_location_right;
		
#define TOLERANCE_CM_X 1.0f // X轴上允许的误差范围 (cm)
#define TOLERANCE_CM_Y 1.0f // Y轴上允许的误差范围 (cm)

// *** 需要根据你的实际摄像头和机械结构进行校准 ***
#define PIXELS_PER_CM_X 13.33f // 假设 1cm = 50 像素
#define PIXELS_PER_CM_Y 13.33f // 假设 1cm = 50 像素
	
#endif

