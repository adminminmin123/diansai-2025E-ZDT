#include "uart_bsp.h"

struct rt_ringbuffer ringbuffer_x;
struct rt_ringbuffer ringbuffer_y;
struct rt_ringbuffer ringbuffer_pi;
	
uint8_t ringbuffer_pool_x[64];
uint8_t ringbuffer_pool_y[64];
uint8_t ringbuffer_pool_pi[64];

uint8_t output_buffer_x[64];
uint8_t output_buffer_y[64];
uint8_t output_buffer_pi[64];

static char line_buffer[128]; // 足够容纳最长的单行数据，例如 64 字节 + \n + \0
static int line_buffer_idx = 0; // 当前行缓冲区的写入位置

int my_printf(UART_HandleTypeDef *huart, const char *format, ...)
{
	char buffer[512];
	int retval;
	va_list local_argv;
	
	va_start(local_argv, format);
	retval = vsnprintf(buffer, sizeof(buffer), format, local_argv);
	va_end(local_argv);
	
	HAL_UART_Transmit(huart,(uint8_t *)buffer, retval, HAL_MAX_DELAY);
	return retval;
}

// 全局变量用于存储XY电机角度
float x_motor_angle = 0.0f;
float y_motor_angle = 0.0f;
// 电机是否超出角度范围标志
uint8_t x_angle_limit_flag = 0;
uint8_t y_angle_limit_flag = 0;

// 电机角度限幅检查标志，由主程序控制是否启用
uint8_t motor_angle_limit_check_enabled = 0;

// 参考位置变量和初始化标志
uint32_t x_reference_position = 0;
uint32_t y_reference_position = 0;
uint8_t x_reference_initialized = 0;
uint8_t y_reference_initialized = 0;
float x_relative_angle = 0.0f;
float y_relative_angle = 0.0f;

// 上电初始位置存储
uint32_t x_initial_position = 0;
uint32_t y_initial_position = 0;
uint8_t x_initial_direction = 0;
uint8_t y_initial_direction = 0;
uint8_t initial_position_saved = 0;

// 角度限幅检查函数
void check_motor_angle_limits(void)
{
    // 仅当限幅检查功能启用时执行
    if (!motor_angle_limit_check_enabled)
        return;

    // 检查X轴电机相对角度是否超出限制
    if (x_relative_angle > MOTOR_MAX_ANGLE || x_relative_angle < -MOTOR_MAX_ANGLE)
    {
        if (x_angle_limit_flag == 0)
        {
            my_printf(&huart1, "X轴电机相对角度超出限制(±%d°)，停止运动!\r\n", MOTOR_MAX_ANGLE);
            x_angle_limit_flag = 1;
            // 停止电机
            Emm_V5_Stop_Now(&MOTOR_X_UART, MOTOR_X_ADDR, MOTOR_SYNC_FLAG);
        }
    }
    else
    {
        x_angle_limit_flag = 0;
    }

    // 检查Y轴电机相对角度是否超出限制
    if (y_relative_angle > MOTOR_MAX_ANGLE || y_relative_angle < -MOTOR_MAX_ANGLE)
    {
        if (y_angle_limit_flag == 0)
        {
            my_printf(&huart1, "Y轴电机相对角度超出限制(±%d°)，停止运动!\r\n", MOTOR_MAX_ANGLE);
            y_angle_limit_flag = 1;
            // 停止电机
            Emm_V5_Stop_Now(&MOTOR_Y_UART, MOTOR_Y_ADDR, MOTOR_SYNC_FLAG);
        }
    }
    else
    {
        y_angle_limit_flag = 0;
    }
}

// 将电机位置值转换为角度
float calc_motor_angle(uint8_t dir, uint32_t position)
{
    float angle;
    // 确保位置值在0-65535范围内
    position = position % 65536;

    // 计算角度值
    angle = ((float)position * 360.0f) / 65536.0f;

    // 如果是负方向，角度取负
    if (dir)
    {
        angle = -angle;
    }

    return angle;
}

// 计算相对角度的函数
float calc_relative_angle(uint8_t dir, uint32_t current_position, uint32_t reference_position)
{
    // 确保位置值在0-65535范围内
    current_position = current_position % 65536;
    reference_position = reference_position % 65536;

    // 计算相对位置差
    int32_t relative_position;
    if (current_position >= reference_position)
    {
        relative_position = current_position - reference_position;
    }
    else
    {
        // 处理过零点情况
        relative_position = 65536 - reference_position + current_position;
    }

    // 如果相对位置大于半圈，认为是反方向的较短距离
    if (relative_position > 32768)
    {
        relative_position = relative_position - 65536;
    }

    // 计算相对角度
    float angle = ((float)relative_position * 360.0f) / 65536.0f;

    // 如果是负方向，角度取负
    if (dir)
    {
        angle = -angle;
    }

    return angle;
}

// X轴电机数据处理函数
void parse_x_motor_data(Emm_V5_Response_t *resp)
{
    // 根据功能码处理不同类型的数据
    switch (resp->func)
    {
    case 0x35: // 读取实时转速
        my_printf(&huart1, "X轴电机地址:%d 实时转速:%d RPM\r\n", resp->addr, resp->speed);
        break;

    case 0x36: // 读取实时位置
        // 计算X轴电机绝对角度
        x_motor_angle = calc_motor_angle(resp->dir, resp->position);

        // 初始化参考位置或计算相对角度
        if (!x_reference_initialized)
        {
            x_reference_position = resp->position;
            x_reference_initialized = 1;
            x_relative_angle = 0.0f;
            my_printf(&huart1, "X轴电机参考位置已初始化: %ld 脉冲\r\n", x_reference_position);
        }
        else
        {
            // 计算相对角度
            x_relative_angle = calc_relative_angle(resp->dir, resp->position, x_reference_position);
        }
        
        // 保存初始位置信息
        if (!initial_position_saved && y_reference_initialized)
        {
            x_initial_position = resp->position;
            x_initial_direction = resp->dir;
            initial_position_saved = 1;
            my_printf(&huart1, "初始位置已保存: X=%ld Y=%ld\r\n", x_initial_position, y_initial_position);
        }

        my_printf(&huart1, "X轴电机地址:%d 实时位置:%ld 脉冲 绝对角度:%.2f° 相对角度:%.2f°\r\n",
                  resp->addr, resp->position, x_motor_angle, x_relative_angle);
        break;

    case 0x1F: // 读取固件版本
        my_printf(&huart1, "X轴电机地址:%d 固件版本:%s\r\n", resp->addr, resp->version);
        break;

    case 0x24: // 读取总线电压
        my_printf(&huart1, "X轴电机地址:%d 总线电压:%d V\r\n", resp->addr, resp->voltage);
        break;

    case 0x27: // 读取相电流
        my_printf(&huart1, "X轴电机地址:%d 相电流:%d mA\r\n", resp->addr, resp->current);
        break;

    case 0x33: // 读取电机状态
        my_printf(&huart1, "X轴电机地址:%d 状态值:0x%02X\r\n", resp->addr, resp->status);
        // 根据状态值进一步解析具体状态
        if (resp->status & 0x01)
            my_printf(&huart1, "  X轴电机已使能\r\n");
        if (resp->status & 0x02)
            my_printf(&huart1, "  X轴电机已到位\r\n");
        if (resp->status & 0x04)
            my_printf(&huart1, "  X轴电机堵转保护\r\n");
        break;

    case 0x3B: // 读取回零状态
        my_printf(&huart1, "X轴电机地址:%d 回零状态:0x%02X\r\n", resp->addr, resp->origin_state);
        // 根据回零状态值进一步解析
        if (resp->origin_state == 0)
            my_printf(&huart1, "  X轴未处于回零状态\r\n");
        else if (resp->origin_state == 1)
            my_printf(&huart1, "  X轴正在回零\r\n");
        else if (resp->origin_state == 2)
            my_printf(&huart1, "  X轴回零完成\r\n");
        else if (resp->origin_state == 3)
            my_printf(&huart1, "  X轴回零失败\r\n");
        break;

    default:
        // 其他功能码，可直接访问resp中的结构体成员获取解析后的数据
        my_printf(&huart1, "X轴电机地址:%d 功能码:0x%02X 已接收未处理\r\n", resp->addr, resp->func);
        break;
    }
}

// Y轴电机数据处理函数
void parse_y_motor_data(Emm_V5_Response_t *resp)
{
    // 根据功能码处理不同类型的数据
    switch (resp->func)
    {
    case 0x35: // 读取实时转速
        my_printf(&huart1, "Y轴电机地址:%d 实时转速:%d RPM\r\n", resp->addr, resp->speed);
        break;

    case 0x36: // 读取实时位置
        // 计算Y轴电机绝对角度
        y_motor_angle = calc_motor_angle(resp->dir, resp->position);

        // 初始化参考位置或计算相对角度
        if (!y_reference_initialized)
        {
            y_reference_position = resp->position;
            y_reference_initialized = 1;
            y_relative_angle = 0.0f;
            my_printf(&huart1, "Y轴电机参考位置已初始化: %ld 脉冲\r\n", y_reference_position);
        }
        else
        {
            // 计算相对角度
            y_relative_angle = calc_relative_angle(resp->dir, resp->position, y_reference_position);
        }
        
        // 保存初始位置信息
        if (!initial_position_saved && x_reference_initialized)
        {
            y_initial_position = resp->position;
            y_initial_direction = resp->dir;
            initial_position_saved = 1;
            my_printf(&huart1, "初始位置已保存: X=%ld Y=%ld\r\n", x_initial_position, y_initial_position);
        }

        my_printf(&huart1, "Y轴电机地址:%d 实时位置:%ld 脉冲 绝对角度:%.2f° 相对角度:%.2f°\r\n",
                  resp->addr, resp->position, y_motor_angle, y_relative_angle);
        break;

    case 0x1F: // 读取固件版本
        my_printf(&huart1, "Y轴电机地址:%d 固件版本:%s\r\n", resp->addr, resp->version);
        break;

    case 0x24: // 读取总线电压
        my_printf(&huart1, "Y轴电机地址:%d 总线电压:%d V\r\n", resp->addr, resp->voltage);
        break;

    case 0x27: // 读取相电流
        my_printf(&huart1, "Y轴电机地址:%d 相电流:%d mA\r\n", resp->addr, resp->current);
        break;

    case 0x33: // 读取电机状态
        my_printf(&huart1, "Y轴电机地址:%d 状态值:0x%02X\r\n", resp->addr, resp->status);
        // 根据状态值进一步解析具体状态
        if (resp->status & 0x01)
            my_printf(&huart1, "  Y轴电机已使能\r\n");
        if (resp->status & 0x02)
            my_printf(&huart1, "  Y轴电机已到位\r\n");
        if (resp->status & 0x04)
            my_printf(&huart1, "  Y轴电机堵转保护\r\n");
        break;

    case 0x3B: // 读取回零状态
        my_printf(&huart1, "Y轴电机地址:%d 回零状态:0x%02X\r\n", resp->addr, resp->origin_state);
        // 根据回零状态值进一步解析
        if (resp->origin_state == 0)
            my_printf(&huart1, "  Y轴未处于回零状态\r\n");
        else if (resp->origin_state == 1)
            my_printf(&huart1, "  Y轴正在回零\r\n");
        else if (resp->origin_state == 2)
            my_printf(&huart1, "  Y轴回零完成\r\n");
        else if (resp->origin_state == 3)
            my_printf(&huart1, "  Y轴回零失败\r\n");
        break;

    default:
        // 其他功能码，可直接访问resp中的结构体成员获取解析后的数据
        my_printf(&huart1, "Y轴电机地址:%d 功能码:0x%02X 已接收未处理\r\n", resp->addr, resp->func);
        break;
    }
}



// 处理复位指令，让电机回到初始位置
void process_reset_command(void)
{
    // 只有当初始位置已保存时才执行
    if (initial_position_saved)
    {
        my_printf(&huart1, "正在复位电机到初始位置...\r\n");
        // 使用绝对位置模式回到初始位置
        // X轴电机
        Emm_V5_Pos_Control(&MOTOR_X_UART, MOTOR_X_ADDR, x_initial_direction, 
                           MOTOR_MAX_SPEED/2, MOTOR_ACCEL, x_initial_position, 
                           true, MOTOR_SYNC_FLAG);
        
        // Y轴电机
        Emm_V5_Pos_Control(&MOTOR_Y_UART, MOTOR_Y_ADDR, y_initial_direction, 
                           MOTOR_MAX_SPEED/2, MOTOR_ACCEL, y_initial_position, 
                           true, MOTOR_SYNC_FLAG);
    }
    else
    {
        my_printf(&huart1, "错误：未保存初始位置，无法复位\r\n");
    }
}

// 保存初始位置信息
void save_initial_position(void)
{
    // 上电时读取当前位置并保存为初始位置
    if (!initial_position_saved)
    {
        // 读取X轴位置
        Emm_V5_Read_Sys_Params(&MOTOR_X_UART, MOTOR_X_ADDR, S_CPOS);
        // 读取Y轴位置
        Emm_V5_Read_Sys_Params(&MOTOR_Y_UART, MOTOR_Y_ADDR, S_CPOS);
        
        // 注意：位置信息会通过串口中断接收并由parse_x_motor_data和parse_y_motor_data处理
        // 标记位置已保存在有效位置数据被解析后设置
        my_printf(&huart1, "正在读取初始位置...\r\n");
    }
}

// 指令处理函数
void process_command(const char* cmd, uint16_t len)
{
    // 处理reset指令
    if (strncmp(cmd, "reset", 5) == 0)
    {
        // 处理复位指令
        process_reset_command();
    }
    // set(x,y)指令 - 设置目标点
    else if (strncmp(cmd, "set(", 6) == 0)
    {
        int target_x, target_y;
        // 尝试解析坐标值
        if (sscanf(cmd, "set(%d,%d)", &target_x, &target_y) == 2)
        {
            // 设置PID目标值
        }
        else
        {
            my_printf(&huart1, "set指令格式错误，应为 set(x,y)\r\n");
        }
    }
}

void uart_proc(void)
{
	uint16_t length_x, length_y, length_pi;
	Emm_V5_Response_t resp_x, resp_y; // 为X轴和Y轴分别创建独立的响应结构体
	// 处理X轴电机数据
	length_x = rt_ringbuffer_data_len(&ringbuffer_x);
//	my_printf(&huart1, "abc%d\n",length_x);
	if (length_x > 0)
	{
		rt_ringbuffer_get(&ringbuffer_x, output_buffer_x, length_x);
		output_buffer_x[length_x] = '\0';
//		my_printf(&huart1, "%d\n",length_x);
		// 解析X轴数据
		if (Emm_V5_Parse_Response(output_buffer_x, length_x, &resp_x))
		{
			parse_x_motor_data(&resp_x);
			my_printf(&huart1,"id:%d",resp_x.addr);
		}
		else
		{
			my_printf(&huart1, "X轴数据解析失败!\r\n");
		}

		memset(output_buffer_x, 0, length_x);
	}

	// 处理Y轴电机数据
	length_y = rt_ringbuffer_data_len(&ringbuffer_y);
	if (length_y > 0)
	{
		rt_ringbuffer_get(&ringbuffer_y, output_buffer_y, length_y);
		output_buffer_y[length_y] = '\0';

		// 解析Y轴数据
		if (Emm_V5_Parse_Response(output_buffer_y, length_y, &resp_y))
		{
			parse_y_motor_data(&resp_y);
		}
		else
		{
			my_printf(&huart1, "Y轴数据解析失败!\r\n");
		}

		memset(output_buffer_y, 0, length_y);
	}

	// 在数据处理完成后执行限幅检查
//	check_motor_angle_limits();
	
	// 处理树莓派数据
	length_pi = rt_ringbuffer_data_len(&ringbuffer_pi);
//	my_printf(&huart1,"length:%d\n",length_pi);
	if(length_pi > 0)
	{
		
		rt_ringbuffer_get(&ringbuffer_pi, output_buffer_pi, length_pi);
        // 逐字节遍历从ringbuffer中取出的数据
        for (int i = 0; i < length_pi; i++)
        {
            char current_char = output_buffer_pi[i];
            
            // 将当前字符添加到行缓冲区，同时检查是否会溢出
            // 减去 1 是为了给字符串末尾的空终止符留出空间
            if (line_buffer_idx < sizeof(line_buffer) - 1)
            {
                line_buffer[line_buffer_idx++] = current_char;
            }
            else
            {
                // 如果行缓冲区满了，但是没有收到换行符，说明当前行太长或者格式错误
                // 此时可以清空缓冲区并返回错误，或者进行其他错误处理
                my_printf(&huart1, "Error: Line buffer overflow without newline. Discarding line.\r\n");
                line_buffer_idx = 0; // 清空缓冲区，避免影响后续数据
                continue; // 跳过此字符，等待下一个可能的帧头
            }
            
            // 如果收到换行符 ('\n')，表示一个完整的帧结束
            // 或者如果收到回车符 ('\r')，在某些系统上它可能也表示行结束，或者与 \n 组合成 \r\n
            // 这里我们主要检查 \n
            if (current_char == '\n')
            {
                line_buffer[line_buffer_idx] = '\0'; // 在行末添加空终止符
                
                // my_printf(&huart1, "Processing received line: '%s'\r\n", line_buffer); // 调试打印每一行
                
                // 将完整的行数据传递给 pi_parse_data 进行解析
                int result = pi_parse_data(line_buffer);
                
                if (result != 0) 
				{
                    my_printf(&huart1, "pi_parse_data returned error %d for line: '%s'\r\n", result, line_buffer);
                }
                
                line_buffer_idx = 0; // 重置行缓冲区索引，准备接收下一行数据
            }
        }
        
        // 不需要 memset(output_buffer_pi, 0, length_pi);
        // 因为数据已经从 ringbuffer 中取出并处理，output_buffer_pi 只是一个临时工作区
    }

}



