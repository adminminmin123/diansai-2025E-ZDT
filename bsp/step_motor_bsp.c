#include "step_motor_bsp.h"

/**
 * @brief 电机初始化函数
 */
void Step_Motor_Init(void)
{
    /* 使能X轴电机 */
    Emm_V5_En_Control(&MOTOR_X_UART, MOTOR_X_ADDR, true, MOTOR_SYNC_FLAG);

    /* 使能Y轴电机 */
    Emm_V5_En_Control(&MOTOR_Y_UART, MOTOR_Y_ADDR, true, MOTOR_SYNC_FLAG);

    /* 初始停止 */
    Step_Motor_Stop();
}

/**
 * @brief 设置XY轴电机速度
 * @param x_percent X轴速度百分比，范围-100到100
 * @param y_percent Y轴速度百分比，范围-100到100
 */
void Step_Motor_Set_Speed(int8_t x_percent, int8_t y_percent)
{
    uint8_t x_dir, y_dir;
    uint16_t x_speed, y_speed;

    /* 限制百分比范围 */
    if (x_percent > 100)
        x_percent = 100;
    if (x_percent < -100)
        x_percent = -100;
    if (y_percent > 100)
        y_percent = 100;
    if (y_percent < -100)
        y_percent = -100;

    /* 设置X轴方向 */
    if (x_percent >= 0)
    {
        x_dir = 0; /* CW方向 */
    }
    else
    {
        x_dir = 1;              /* CCW方向 */
        x_percent = -x_percent; /* 取绝对值 */
    }

    /* 设置Y轴方向 */
    if (y_percent >= 0)
    {
        y_dir = 0; /* CW方向 */
    }
    else
    {
        y_dir = 1;              /* CCW方向 */
        y_percent = -y_percent; /* 取绝对值 */
    }

    /* 计算实际速度值(百分比转换为RPM) */
    x_speed = (uint16_t)((x_percent * MOTOR_MAX_SPEED) / 100);
    y_speed = (uint16_t)((y_percent * MOTOR_MAX_SPEED) / 100);

	/* 添加调试打印确认计算 */
     my_printf(&huart1, "X: dir=%d, speed=%u; Y: dir=%d, speed=%u\r\n", x_dir, x_speed, y_dir, y_speed);
	
    /* 控制X轴电机 */
    Emm_V5_Vel_Control(&MOTOR_X_UART, MOTOR_X_ADDR, x_dir, x_speed, MOTOR_ACCEL, MOTOR_SYNC_FLAG);

    /* 控制Y轴电机 */
    Emm_V5_Vel_Control(&MOTOR_Y_UART, MOTOR_Y_ADDR, y_dir, y_speed, MOTOR_ACCEL, MOTOR_SYNC_FLAG);
}

/**
 * @brief 设置XY轴电机速度
 * @param x_rpm X轴目标速度，单位RPM (revolutions per minute)。
 *              支持负值表示反向。
 *              速度精度为0.1RPM。小于0.05 RPM的绝对值将被量化为0。
 * @param y_rpm Y轴目标速度，单位RPM (revolutions per minute)。
 *              支持负值表示反向。
 *              速度精度为0.1RPM。小于0.05 RPM的绝对值将被量化为0。
 */
void Step_Motor_Set_Speed_my(float x_rpm, float y_rpm)
{
    uint8_t x_dir, y_dir;
    uint16_t x_speed_scaled, y_speed_scaled; // 速度值，单位为 0.1 RPM
    float abs_x_rpm, abs_y_rpm;

    /* 1. 限制输入RPM范围，确保不超过电机最大物理速度 */
    // 将输入的RPM值钳位在 [-MOTOR_MAX_SPEED, MOTOR_MAX_SPEED] 之间
    if (x_rpm > MOTOR_MAX_SPEED)
    {
        x_rpm = MOTOR_MAX_SPEED;
    }
    else if (x_rpm < -MOTOR_MAX_SPEED)
    {
        x_rpm = -MOTOR_MAX_SPEED;
    }

    if (y_rpm > MOTOR_MAX_SPEED)
    {
        y_rpm = MOTOR_MAX_SPEED;
    }
    else if (y_rpm < -MOTOR_MAX_SPEED)
    {
        y_rpm = -MOTOR_MAX_SPEED;
    }

    /* 2. 处理X轴方向和获取绝对速度 */
    if (x_rpm >= 0.0f)
    {
        x_dir = 0; /* CW方向 (正转) */
        abs_x_rpm = x_rpm;
    }
    else
    {
        x_dir = 1; /* CCW方向 (反转) */
        abs_x_rpm = -x_rpm; /* 取绝对值 */
    }

    /* 3. 处理Y轴方向和获取绝对速度 */
    if (y_rpm >= 0.0f)
    {
        y_dir = 0; /* CW方向 (正转) */
        abs_y_rpm = y_rpm;
    }
    else
    {
        y_dir = 1; /* CCW方向 (反转) */
        abs_y_rpm = -y_rpm; /* 取绝对值 */
    }

    /* 4. 计算实际发送给电机控制器的速度值 (单位为 0.1 RPM) */
    // 将RPM值乘以10，得到以0.1RPM为单位的整数值。
    // 加上0.5f是为了进行四舍五入。
    // 这样，例如 0.04 RPM (0.4 scaled) + 0.5f = 0.9f -> 0 (uint16_t)
    // 0.05 RPM (0.5 scaled) + 0.5f = 1.0f -> 1 (uint16_t)
    x_speed_scaled = (uint16_t)(abs_x_rpm * 10 + 0.5f);
    y_speed_scaled = (uint16_t)(abs_y_rpm * 10 + 0.5f);
    
    // 再次检查计算出的 scaled speed 是否超出 uint16_t 的最大值，
    // 理论上在输入钳位后 (MOTOR_MAX_SPEED * RPM_TO_SCALED_FACTOR) 不会超过 uint16_t，
    // 但作为鲁棒性检查，可以添加此行。
//    uint16_t max_scaled_speed = (uint16_t)(MOTOR_MAX_SPEED * 10 + 0.5f);
//    if (x_speed_scaled > max_scaled_speed) {
//        x_speed_scaled = max_scaled_speed;
//    }
//    if (y_speed_scaled > max_scaled_speed) {
//        y_speed_scaled = max_scaled_speed;
//    }


//    /* 添加调试打印确认计算 */
//    my_printf(&huart1, "X: input_rpm=%.1f, dir=%d, abs_rpm=%.1f, scaled_speed=%u; Y: input_rpm=%.1f, dir=%d, abs_rpm=%.1f, scaled_speed=%u\r\n",
//              x_rpm, x_dir, abs_x_rpm, x_speed_scaled,
//              y_rpm, y_dir, abs_y_rpm, y_speed_scaled);
    
    /* 控制X轴电机 */
    Emm_V5_Vel_Control(&MOTOR_X_UART, MOTOR_X_ADDR, x_dir, x_speed_scaled, MOTOR_ACCEL, MOTOR_SYNC_FLAG);

    /* 控制Y轴电机 */
    Emm_V5_Vel_Control(&MOTOR_Y_UART, MOTOR_Y_ADDR, y_dir, y_speed_scaled, MOTOR_ACCEL, MOTOR_SYNC_FLAG);
}

/**
 * @brief 设置XY轴电机移动一段距离（使用位置模式）
 * @param x_distance X轴移动距离（脉冲数），正值为CW方向，负值为CCW方向
 * @param y_distance Y轴移动距离（脉冲数），正值为CW方向，负值为CCW方向
 */
void Step_Motor_Set_Pwm(int32_t x_distance, int32_t y_distance)
{
    uint8_t x_dir, y_dir;
    uint32_t x_clk, y_clk;
    uint16_t speed = MOTOR_MAX_SPEED;  /* 使用最大速度，或根据需要调整 */
    uint8_t acc = MOTOR_ACCEL;        /* 使用预定义加速度 */

    /* 设置X轴方向和脉冲数 */
    if (x_distance >= 0)
    {
        x_dir = 0; /* CW方向 */
        x_clk = (uint32_t)x_distance;
    }
    else
    {
        x_dir = 1; /* CCW方向 */
        x_clk = (uint32_t)(-x_distance); /* 取绝对值 */
    }

    /* 设置Y轴方向和脉冲数 */
    if (y_distance >= 0)
    {
        y_dir = 0; /* CW方向 */
        y_clk = (uint32_t)y_distance;
    }
    else
    {
        y_dir = 1; /* CCW方向 */
        y_clk = (uint32_t)(-y_distance); /* 取绝对值 */
    }

    /* 控制X轴电机（相对运动，不启用绝对模式） */
    Emm_V5_Pos_Control(&MOTOR_X_UART, MOTOR_X_ADDR, x_dir, speed, acc, x_clk, false, MOTOR_SYNC_FLAG);

    /* 控制Y轴电机（相对运动，不启用绝对模式） */
    Emm_V5_Pos_Control(&MOTOR_Y_UART, MOTOR_Y_ADDR, y_dir, speed, acc, y_clk, false, MOTOR_SYNC_FLAG);
}


/**
 * @brief 停止所有电机
 */
void Step_Motor_Stop(void)
{
    /* 停止X轴电机 */
    Emm_V5_Stop_Now(&MOTOR_X_UART, MOTOR_X_ADDR, MOTOR_SYNC_FLAG);

    /* 停止Y轴电机 */
    Emm_V5_Stop_Now(&MOTOR_Y_UART, MOTOR_Y_ADDR, MOTOR_SYNC_FLAG);
}

void step_motor_proc(void)
{
	
}

