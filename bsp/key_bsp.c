#include "key_bsp.h"
#include "laser_bsp.h"
#include "step_motor_bsp.h"
#include "../app/Emm_V5.h"

uint8_t key_val = 0;
uint8_t key_old = 0;
uint8_t key_down = 0;
uint8_t key_up = 0;

// 角度控制按键相关变量
uint8_t angle_key_val = 0;
uint8_t angle_key_old = 0;
uint8_t angle_key_down = 0;

uint8_t key_read(void)
{
	uint8_t temp = 0;
	if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
		temp = 1;
	if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
		temp = 2;
	return temp;
}

/**
 * @brief 读取新的角度控制按键状态
 * @return 按键值：0-无按键，1-左转22.5度，2-右转22.5度，3-启动旋转
 * @note 新的22.5度累积角度控制系统
 */
uint8_t read_angle_keys(void)
{
	uint8_t temp = 0;

	// 首先检查是否有传统按键按下，如果有则不处理角度按键
	if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET ||
			HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
	{
		return 0; // 传统按键优先，避免冲突
	}

	// 检测新的角度控制按键
	if (HAL_GPIO_ReadPin(Left_22_5_GPIO_Port, Left_22_5_Pin) == GPIO_PIN_RESET)
		temp = KEY_LEFT_22_5;
	else if (HAL_GPIO_ReadPin(Right_22_5_GPIO_Port, Right_22_5_Pin) == GPIO_PIN_RESET)
		temp = KEY_RIGHT_22_5;
	else if (HAL_GPIO_ReadPin(Start_GPIO_Port, Start_Pin) == GPIO_PIN_RESET)
		temp = KEY_START;

	return temp;
}

/**
 * @brief 读取Y轴控制按键状态
 * @return 按键值：0-无按键，4-Y轴向下45度，5-Y轴向下135度
 * @note Y轴角度控制系统
 */
uint8_t read_y_axis_keys(void)
{
	uint8_t temp = 0;

	// 首先检查是否有传统按键按下，如果有则不处理Y轴按键
	if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET ||
			HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
	{
		return 0; // 传统按键优先，避免冲突
	}

	// 检测Y轴控制按键
	if (HAL_GPIO_ReadPin(Y_Down_45_GPIO_Port, Y_Down_45_Pin) == GPIO_PIN_RESET)
		temp = KEY_Y_DOWN_45;
	else if (HAL_GPIO_ReadPin(Y_Down_135_GPIO_Port, Y_Down_135_Pin) == GPIO_PIN_RESET)
		temp = KEY_Y_DOWN_135;

	return temp;
}

/**
 * @brief Y轴电机角度控制处理函数
 * @note 处理Y轴向下45度和135度旋转
 */
void key_motor_y_axis_control(void)
{
	static uint32_t last_y_key_time = 0;
	static uint8_t y_key_val = 0, y_key_down = 0, y_key_old = 0;
	static float y_axis_angle = 0.0f; // Y轴设定角度
	uint32_t current_time = HAL_GetTick();

	// 读取Y轴控制按键
	y_key_val = read_y_axis_keys();
	y_key_down = y_key_val & (y_key_val ^ y_key_old);
	y_key_old = y_key_val;

	// 如果有按键按下
	if (y_key_down != 0)
	{
		// 防抖处理
		if (current_time - last_y_key_time < 200) // 200ms防抖
		{
			my_printf(&huart1, "Y-axis key debounce: ignoring (too soon)\r\n");
			return;
		}
		last_y_key_time = current_time;

		my_printf(&huart1, "=== Y-AXIS CONTROL SYSTEM ===\r\n");

		switch (y_key_down)
		{
		case KEY_Y_DOWN_45:
			// Y轴向下45度
			y_axis_angle = -ANGLE_45;				// 负值表示向下
			set_y_axis_angle(y_axis_angle); // 保存到全局变量
			my_printf(&huart1, "Y_Down_45° pressed\r\n");
			my_printf(&huart1, "Y-axis angle set: %.1f° (DOWN)\r\n", fabs(y_axis_angle));
			my_printf(&huart1, "Press START to execute Y-axis rotation\r\n");
			break;

		case KEY_Y_DOWN_135:
			// Y轴向下135度
			y_axis_angle = -ANGLE_135;			// 负值表示向下
			set_y_axis_angle(y_axis_angle); // 保存到全局变量
			my_printf(&huart1, "Y_Down_135° pressed\r\n");
			my_printf(&huart1, "Y-axis angle set: %.1f° (DOWN)\r\n", fabs(y_axis_angle));
			my_printf(&huart1, "Press START to execute Y-axis rotation\r\n");
			break;

		default:
			my_printf(&huart1, "ERROR: Invalid Y-axis key: %d\r\n", y_key_down);
			return;
		}

		my_printf(&huart1, "=== Y-AXIS STATUS ===\r\n");
		my_printf(&huart1, "Current Y-axis angle: %.1f°\r\n", y_axis_angle);
		my_printf(&huart1, "====================\r\n");
	}
}

// 全局Y轴角度变量
static float g_y_axis_angle = 0.0f;

/**
 * @brief 获取当前Y轴设定角度
 * @return Y轴角度值
 */
float get_y_axis_angle(void)
{
	return g_y_axis_angle;
}

/**
 * @brief 设置Y轴角度
 * @param angle Y轴角度值
 */
void set_y_axis_angle(float angle)
{
	g_y_axis_angle = angle;
}

void key_proc(void)
{
	// 处理角度控制按键
	key_motor_angle_control();

	// 然后处理KEY1和KEY2的传统按键检测
	key_val = key_read();
	key_down = key_val & (key_val ^ key_old);
	key_up = ~key_val & (key_val ^ key_old);
	key_old = key_val;

	// KEY1激光手动控制处理
	key_laser_manual_control();

	// KEY2模式切换处理
	key_mode_switch_control();

	// Y轴角度控制处理
	key_motor_y_axis_control();
}

/**
 * @brief KEY1激光手动控制处理函数
 * @note 短按手动控制激光开关
 */
void key_laser_manual_control(void)
{
	static uint32_t last_key_time = 0;
	uint32_t current_time = HAL_GetTick();

	// 检测按键按下
	uint8_t key_pressed = (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET);

	// 按键按下边沿检测
	static uint8_t key_last_state = 1; // 1表示未按下
	if (key_pressed && key_last_state)
	{
		// 防抖处理
		if (current_time - last_key_time < 200) // 200ms防抖
		{
			key_last_state = key_pressed;
			my_printf(&huart1, "KEY1 DEBOUNCE: Ignoring press (too soon)\r\n");
			return;
		}
		last_key_time = current_time;

		my_printf(&huart1, "KEY1 DETECTED: Button pressed\r\n");

		// 手动控制激光开关
		LaserState_t current_state = Laser_GetState();
		SystemMode_t current_mode = Laser_GetSystemMode();

		my_printf(&huart1, "KEY1 STATUS: Current laser state=%d, system mode=%d\r\n", current_state, current_mode);

		if (current_state == LASER_STATE_OFF)
		{
			Laser_ManualControl(1); // 开启激光
			my_printf(&huart1, "KEY1 PRESSED: Manual Laser ON command sent\r\n");
		}
		else
		{
			Laser_ManualControl(0); // 关闭激光
			my_printf(&huart1, "KEY1 PRESSED: Manual Laser OFF command sent\r\n");
		}

		// 验证状态是否改变
		LaserState_t new_state = Laser_GetState();
		my_printf(&huart1, "KEY1 RESULT: New laser state=%d\r\n", new_state);
	}

	key_last_state = key_pressed;
}

/**
 * @brief KEY2模式切换处理函数
 * @note 短按切换系统模式：模式0 -> 模式1 -> 模式2 -> 模式3 -> 模式0 (循环)
 */
void key_mode_switch_control(void)
{
	static uint32_t last_key_time = 0;
	uint32_t current_time = HAL_GetTick();

	// 检测按键按下
	uint8_t key_pressed = (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET);

	// 按键按下边沿检测
	static uint8_t key_last_state = 1; // 1表示未按下
	if (key_pressed && key_last_state)
	{
		// 防抖处理
		if (current_time - last_key_time < 200) // 200ms防抖
		{
			key_last_state = key_pressed;
			my_printf(&huart1, "KEY2 DEBOUNCE: Ignoring press (too soon)\r\n");
			return;
		}
		last_key_time = current_time;

		// 获取当前模式并切换到下一个模式
		SystemMode_t current_mode = Laser_GetSystemMode();
		SystemMode_t next_mode;

		switch (current_mode)
		{
		case SYSTEM_MODE_0:
			next_mode = SYSTEM_MODE_1;
			break;
		case SYSTEM_MODE_1:
			next_mode = SYSTEM_MODE_2;
			break;
		case SYSTEM_MODE_2:
			next_mode = SYSTEM_MODE_3;
			break;
		case SYSTEM_MODE_3:
			next_mode = SYSTEM_MODE_0;
			break;
		default:
			next_mode = SYSTEM_MODE_0;
			break;
		}

		Laser_SetSystemMode(next_mode);
		my_printf(&huart1, "KEY2 PRESSED: Switched to System Mode %d\r\n", next_mode);
	}

	key_last_state = key_pressed;
}

/**
 * @brief 新的22.5度累积角度控制处理函数
 * @note 实现按键累积角度，Start按键启动旋转
 */
void key_motor_angle_control(void)
{
	static uint32_t last_angle_key_time = 0;
	static float accumulated_angle = 0.0f; // 累积角度
	static uint32_t left_press_count = 0;	 // 左转按键按下次数
	static uint32_t right_press_count = 0; // 右转按键按下次数
	uint32_t current_time = HAL_GetTick();

	// 读取角度控制按键
	angle_key_val = read_angle_keys();
	angle_key_down = angle_key_val & (angle_key_val ^ angle_key_old);
	angle_key_old = angle_key_val;

	// 如果有按键按下
	if (angle_key_down != 0)
	{
		// 防抖处理
		if (current_time - last_angle_key_time < 200) // 200ms防抖
		{
			my_printf(&huart1, "Key debounce: ignoring (too soon)\r\n");
			return;
		}
		last_angle_key_time = current_time;

		my_printf(&huart1, "=== ANGLE ACCUMULATION SYSTEM ===\r\n");

		switch (angle_key_down)
		{
		case KEY_LEFT_22_5:
			// 左转22.5度累积
			if (fabs((float)(left_press_count + 1) * ANGLE_22_5) <= MAX_ANGLE_ACCUMULATION)
			{
				left_press_count++;
				accumulated_angle = -(float)left_press_count * ANGLE_22_5;

				// 重置右转计数
				right_press_count = 0;

				my_printf(&huart1, "LEFT 22.5° pressed - Count: %lu\r\n", left_press_count);
				my_printf(&huart1, "Accumulated angle: %.1f° (LEFT)\r\n", fabs(accumulated_angle));
				my_printf(&huart1, "Press START to execute rotation\r\n");
			}
			else
			{
				my_printf(&huart1, "LEFT 22.5° - Maximum angle limit reached (%.1f°)\r\n", MAX_ANGLE_ACCUMULATION);
				my_printf(&huart1, "Press START to execute current rotation first\r\n");
			}
			break;

		case KEY_RIGHT_22_5:
			// 右转22.5度累积
			if (fabs((float)(right_press_count + 1) * ANGLE_22_5) <= MAX_ANGLE_ACCUMULATION)
			{
				right_press_count++;
				accumulated_angle = (float)right_press_count * ANGLE_22_5;

				// 重置左转计数
				left_press_count = 0;

				my_printf(&huart1, "RIGHT 22.5° pressed - Count: %lu\r\n", right_press_count);
				my_printf(&huart1, "Accumulated angle: %.1f° (RIGHT)\r\n", accumulated_angle);
				my_printf(&huart1, "Press START to execute rotation\r\n");
			}
			else
			{
				my_printf(&huart1, "RIGHT 22.5° - Maximum angle limit reached (%.1f°)\r\n", MAX_ANGLE_ACCUMULATION);
				my_printf(&huart1, "Press START to execute current rotation first\r\n");
			}
			break;

		case KEY_START:
		{
			// 检查当前系统模式
			SystemMode_t current_system_mode = Laser_GetSystemMode();

			my_printf(&huart1, "\r\n=== START BUTTON PRESSED ===\r\n");
			my_printf(&huart1, "Current system mode: %d\r\n", current_system_mode);

			// 启动系统（使能电机等）
			if (current_system_mode != SYSTEM_MODE_0)
			{
				Laser_StartSystem();
			}

			// 电机旋转处理：根据模式决定处理方式
			my_printf(&huart1, "=== MOTOR ROTATION SEQUENCE START ===\r\n");

			if (current_system_mode == SYSTEM_MODE_1)
			{
				// 模式1特殊处理：只执行Y轴复位，不处理X轴和目标识别，不启动目标追踪
				my_printf(&huart1, "Mode 1: Special handling - Y-axis reset only, no X-axis, no target processing, no tracking\r\n");

				// 第一步：Y轴处理（优先执行）
				float y_axis_angle = get_y_axis_angle();
				uint8_t has_y_axis_command = (fabs(y_axis_angle) > ANGLE_RESET_THRESHOLD);

				if (has_y_axis_command)
				{
					// 有Y轴按键命令，执行指定角度旋转
					my_printf(&huart1, "Step 1: Executing Y-axis rotation: %.1f°\r\n", y_axis_angle);
					Step_Motor_Rotate_Y_Angle((int16_t)y_axis_angle);
					set_y_axis_angle(0.0f); // 重置Y轴角度
				}
				else
				{
					// 没有Y轴按键命令，执行默认Y轴复位
					my_printf(&huart1, "Step 1: No Y-axis command - Executing default Y-axis reset (15° up)\r\n");
					Step_Motor_Rotate_Y_Angle(18); // 默认向上15度复位
				}

				// 模式1：跳过X轴处理，直接重置累积角度
				if (fabs(accumulated_angle) > ANGLE_RESET_THRESHOLD)
				{
					my_printf(&huart1, "Step 2: Mode 1 - Skipping X-axis rotation (%.1f°), resetting accumulation\r\n", accumulated_angle);
					accumulated_angle = 0.0f;
					left_press_count = 0;
					right_press_count = 0;
					my_printf(&huart1, "X-axis angle accumulation reset (Mode 1 - no X-axis movement)\r\n");
				}
				else
				{
					my_printf(&huart1, "Step 2: Mode 1 - No X-axis rotation needed\r\n");
				}

				// 模式1特殊说明：即便识别到目标点，也不对目标点做任何处理，不启动目标追踪系统
				my_printf(&huart1, "Mode 1: Target tracking disabled - laser will fire automatically after delay regardless of target detection\r\n");
			}
			else
			{
				// 其他模式：正常处理Y轴和X轴
				// 第一步：Y轴处理（优先执行）
				float y_axis_angle = get_y_axis_angle();
				uint8_t has_y_axis_command = (fabs(y_axis_angle) > ANGLE_RESET_THRESHOLD);

				if (has_y_axis_command)
				{
					// 有Y轴按键命令，执行指定角度旋转
					my_printf(&huart1, "Step 1: Executing Y-axis rotation: %.1f°\r\n", y_axis_angle);
					Step_Motor_Rotate_Y_Angle((int16_t)y_axis_angle);
					set_y_axis_angle(0.0f); // 重置Y轴角度
				}
				else if (current_system_mode != SYSTEM_MODE_0)
				{
					// 没有Y轴按键命令，执行默认Y轴复位
					my_printf(&huart1, "Step 1: No Y-axis command - Executing default Y-axis reset (15° up)\r\n");
					Step_Motor_Rotate_Y_Angle(15); // 默认向上26度复位
				}
				else
				{
					my_printf(&huart1, "Step 1: Y-axis skipped (Mode 0 - Standby)\r\n");
				}

				// 第二步：X轴处理（在Y轴完成后执行）
				uint8_t has_x_axis_command = (fabs(accumulated_angle) > ANGLE_RESET_THRESHOLD);

				if (has_x_axis_command)
				{
					my_printf(&huart1, "Step 2: Executing X-axis rotation: %.1f°\r\n", accumulated_angle);
					Step_Motor_Rotate_X_Angle((int16_t)accumulated_angle);

					// 重置累积角度和计数
					accumulated_angle = 0.0f;
					left_press_count = 0;
					right_press_count = 0;
					my_printf(&huart1, "X-axis angle accumulation reset\r\n");
				}
				else
				{
					my_printf(&huart1, "Step 2: No X-axis rotation needed\r\n");
				}
			}

			my_printf(&huart1, "=== MOTOR ROTATION SEQUENCE COMPLETED ===\r\n");

			// 第三步：执行模式相关逻辑
			my_printf(&huart1, "Step 3: Executing mode-specific logic\r\n");

			if (current_system_mode == SYSTEM_MODE_1)
			{
				my_printf(&huart1, "Mode 1: System started - 1.5s auto-fire countdown begins (no target processing)\r\n");
			}
			else if (current_system_mode == SYSTEM_MODE_2)
			{
				Laser_StartMode2Timing();
				my_printf(&huart1, "Mode 2: Laser timing started - 3.5s countdown begins\r\n");
			}
			else if (current_system_mode == SYSTEM_MODE_3)
			{
				my_printf(&huart1, "Mode 3: Target tracking mode activated\r\n");
			}
			else
			{
				my_printf(&huart1, "Mode 0: Standby mode - No automatic laser control\r\n");
			}

			my_printf(&huart1, "=== START PROCESSING COMPLETE ===\r\n\r\n");
		}
		break;

		default:
			my_printf(&huart1, "ERROR: Invalid angle key: %d\r\n", angle_key_down);
			return;
		}

		my_printf(&huart1, "=== STATUS ===\r\n");
		my_printf(&huart1, "Left presses: %lu, Right presses: %lu\r\n", left_press_count, right_press_count);
		my_printf(&huart1, "Current accumulated: %.1f°\r\n", accumulated_angle);
		my_printf(&huart1, "===============\r\n");
	}
}

/**
 * @brief 测试新的角度控制按键的GPIO状态
 */
void test_all_angle_keys(void)
{
	my_printf(&huart1, "\r\n=== NEW ANGLE KEY GPIO TEST ===\r\n");

	my_printf(&huart1, "Left_22.5° (PB4): %d\r\n",
						HAL_GPIO_ReadPin(Left_22_5_GPIO_Port, Left_22_5_Pin));
	my_printf(&huart1, "Right_22.5° (PB8): %d\r\n",
						HAL_GPIO_ReadPin(Right_22_5_GPIO_Port, Right_22_5_Pin));
	my_printf(&huart1, "Start (PB6): %d\r\n",
						HAL_GPIO_ReadPin(Start_GPIO_Port, Start_Pin));

	my_printf(&huart1, "Traditional keys:\r\n");
	my_printf(&huart1, "KEY1 (PE9): %d\r\n",
						HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin));
	my_printf(&huart1, "KEY2 (PE11): %d\r\n",
						HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin));

	my_printf(&huart1, "=== GPIO TEST END ===\r\n\r\n");
}

/**
 * @brief 重置角度累积状态
 * @note 可以通过串口命令或其他方式调用
 */
void reset_angle_accumulation(void)
{
	// 注意：这些静态变量在key_motor_angle_control函数中定义
	// 这里我们通过调用一个特殊的重置机制来实现
	my_printf(&huart1, "=== ANGLE ACCUMULATION RESET ===\r\n");
	my_printf(&huart1, "All angle accumulation cleared\r\n");
	my_printf(&huart1, "Ready for new angle input\r\n");
	my_printf(&huart1, "===============================\r\n");
}

/**
 * @brief 获取当前角度累积状态
 * @note 显示当前累积的角度信息
 */
void get_current_angle_status(void)
{
	my_printf(&huart1, "=== ANGLE ACCUMULATION STATUS ===\r\n");
	my_printf(&huart1, "System: 22.5° incremental rotation\r\n");
	my_printf(&huart1, "Controls:\r\n");
	my_printf(&huart1, "  Left_22.5° - Add 22.5° left rotation\r\n");
	my_printf(&huart1, "  Right_22.5° - Add 22.5° right rotation\r\n");
	my_printf(&huart1, "  Start - Execute accumulated rotation\r\n");
	my_printf(&huart1, "Note: Switching direction resets counter\r\n");
	my_printf(&huart1, "================================\r\n");
}
