#include "pi_bsp.h"

// 默认值设为无效状态，X, Y 为 0
LaserCoord_t latest_red_laser_coord = {RED_LASER_ID, 0, 0, 0};
LaserCoord_t latest_green_laser_coord = {GREEN_LASER_ID, 0, 0, 0};

// MaixCam 数据解析函数，解析格式：red:(x,y)\n 或 gre:(x,y)\n
// 这个函数现在直接更新全局变量 latest_red_laser_coord 和 latest_green_laser_coord
int pi_parse_data(char *buffer)
{
    if (!buffer)
        return -1; // 空指针检查

    int parsed_x, parsed_y; // 临时变量用于存储解析出的X,Y坐标
    int parsed_count;

    // 尝试匹配 "red:(x,y)" 格式
    if (strncmp(buffer, "red:", 4) == 0)
    {
        parsed_count = sscanf(buffer, "red:(%d,%d)", &parsed_x, &parsed_y);
        if (parsed_count != 2) // 必须解析出X和Y两个值
            return -2; // 解析失败

        // 解析成功，更新全局红色激光坐标
        latest_red_laser_coord.x = parsed_x;
        latest_red_laser_coord.y = parsed_y;
        latest_red_laser_coord.isValid = 1; // 标记数据为有效

        // 打印调试信息
		my_printf(&huart1, "Parsed RED: X=%d, Y=%d\r\n", latest_red_laser_coord.x, latest_red_laser_coord.y);
    }
    // 尝试匹配 "gre:(x,y)" 格式
    else if (strncmp(buffer, "gre:", 4) == 0)
    {
        parsed_count = sscanf(buffer, "gre:(%d,%d)", &parsed_x, &parsed_y);
        if (parsed_count != 2) // 必须解析出X和Y两个值
            return -2; // 解析失败

        // 解析成功，更新全局绿色激光坐标
        latest_green_laser_coord.x = parsed_x;
        latest_green_laser_coord.y = parsed_y;
        latest_green_laser_coord.isValid = 1; // 标记数据为有效

        // 打印调试信息
		my_printf(&huart1, "Parsed GRE: X=%d, Y=%d\r\n", latest_green_laser_coord.x, latest_green_laser_coord.y);
    }
    else
    {
        // 既不是 "red:" 也不是 "gre:" 开头，认为是未知格式或无效数据
        return -3; // 未知或无效格式
    }

    return 0; // 成功
}



void pi_proc(void)
{
	float pos_out_x,pos_out_y=0;


		pos_out_x = pid_calc(&pid_x,latest_green_laser_coord.x, latest_red_laser_coord.x, 0);
		pos_out_y = pid_calc(&pid_y,latest_green_laser_coord.y, latest_red_laser_coord.y, 0);
		Step_Motor_Set_Speed_my(-pos_out_x,pos_out_y);

	
}

