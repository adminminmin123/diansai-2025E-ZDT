#ifndef __PI_BSP_H__
#define __PI_BSP_H__

#include "bsp_system.h"

// 激光类型标识符
#define RED_LASER_ID 'R'
#define GREEN_LASER_ID 'G'

// 激光坐标数据结构
typedef struct {
    char type;    // 激光类型: 'R'表示红色激光，'G'表示绿色激光
    int x;        // X坐标
    int y;        // Y坐标
    uint8_t isValid; // 新增：指示当前数据是否有效/已更新
} LaserCoord_t;

int pi_parse_data(char *buffer);
void pi_proc(void);

extern LaserCoord_t latest_red_laser_coord;
extern LaserCoord_t latest_green_laser_coord;

#endif
