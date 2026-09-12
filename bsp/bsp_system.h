#ifndef __BSP_SYSTEM_H__
#define __BSP_SYSTEM_H__

#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "main.h"
#include "dma.h"
#include "usart.h"
#include "schedule.h"
#include "ringbuffer.h"

#include "key_bsp.h"
#include "uart_bsp.h"
#include "step_motor_bsp.h"
#include "pi_bsp.h"
#include "laser_bsp.h"

#include "Emm_V5.h"
#include "mypid.h"

extern uint8_t uart3_rx_buffer[32]; // hwt101

extern struct rt_ringbuffer ringbuffer_x;
extern struct rt_ringbuffer ringbuffer_y;
extern struct rt_ringbuffer ringbuffer_pi;

extern uint8_t motor_x_buf[64];
extern uint8_t motor_y_buf[64];
extern uint8_t pi_rx_buf[64];
extern uint8_t car_rx_buf[64];  // 小车信号接收缓冲区

extern uint8_t ringbuffer_pool_x[64];
extern uint8_t ringbuffer_pool_y[64];
extern uint8_t ringbuffer_pool_pi[64];

extern uint8_t output_buffer_x[64];
extern uint8_t output_buffer_y[64];
extern uint8_t output_buffer_pi[64];

extern float speed_set[3];
extern uint8_t set_index;

#endif
