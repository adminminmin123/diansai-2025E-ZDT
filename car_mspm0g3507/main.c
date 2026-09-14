/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include "stdio.h"
#include <stdint.h>
#include "mian.h"
void xunxian(void);
uint8_t Key_Read(void);
void UART_SendString(const char* str);
void UART_SendTurnSignal(void);
volatile uint8_t gEchoData = 0;
uint8_t oled_buffer[32];
extern int32_t Motor_L_count;//编码器计数值
extern int32_t Motor_R_count;
int16_t Speed_R=0;
int16_t Speed_L=0;
uint16_t count=0;
uint16_t count1=0;
//角度
float angle_Target, angle_Actual, angle_Out;			//目标值，实际值，输出值
float angle_Kp, angle_Ki, angle_Kd;					//比例项，积分项，微分项的权重
float angle_Error0, angle_Error1, angle_ErrorInt;		//本次误差，上次误差，误差积分
//左速度
float leftspeed_Target, leftspeed_Actual, leftspeed_Out;			//目标值，实际值，输出值
float leftspeed_Kp, leftspeed_Ki, leftspeed_Kd;					//比例项，积分项，微分项的权重
float leftspeed_Error0, leftspeed_Error1, leftspeed_ErrorInt;		//本次误差，上次误差，误差积分
//右速度
float rightspeed_Target, rightspeed_Actual, rightspeed_Out;			//目标值，实际值，输出值
float rightspeed_Kp, rightspeed_Ki, rightspeed_Kd;					//比例项，积分项，微分项的权重
float rightspeed_Error0, rightspeed_Error1, rightspeed_ErrorInt;		//本次误差，上次误差，误差积分
unsigned char flag=0;
unsigned char stop_flag=1;
unsigned char countquan_flag=0;
unsigned char last_countflag=0;
unsigned char die_flag=0;
unsigned char zhuanxiang_time_flag;

int countquannum;
int quannum=1;
uint32_t timer_xms;
uint32_t timer_zhuanxiang;

uint8_t Key_Read(void)
{
    uint8_t temp = 0;
    if (!DL_GPIO_readPins(KEY_K2_PORT, KEY_K2_PIN)) temp = 1;
    if (!DL_GPIO_readPins(KEY_K1_PORT, KEY_K1_PIN)) temp = 2;
   // if (!DL_GPIO_readPins(KEY_PORT, KEY_K2_PIN)) temp = 3;
    return temp;
}

// 串口发送字符串函数
void UART_SendString(const char* str)
{
    while (*str) {
        // 等待发送缓冲区空闲
        while (!DL_UART_isTXFIFOEmpty(UART_OPENMV_INST));
        // 发送单个字符
        DL_UART_transmitData(UART_OPENMV_INST, *str);
        str++;
    }
}

// 发送转弯信号给STM32F407VET6
void UART_SendTurnSignal(void)
{
    UART_SendString("TURN_START\r\n");
}

int main(void)
{
    SYSCFG_DL_init();
    SysTick_Init();
    //PWM初始化
    DL_TimerG_startCounter(PWM_INST);
    /*使能编码器外部中断*/

    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    /*使能时钟定时器中断*/
    NVIC_EnableIRQ(TIMER_COUNT_INST_INT_IRQN);
    DL_TimerA_startCounter(TIMER_COUNT_INST);

    /*使能UART中断*/
    NVIC_EnableIRQ(UART_OPENMV_INST_INT_IRQN);


    
        leftspeed_Kp = 1.80;			//速度Kp
		leftspeed_Ki = 2/200 ;			//速度Ki
		leftspeed_Kd =0.08;				//速度Kd
		leftspeed_Target =00 ;	        //速度目标值
        //右轮
        rightspeed_Kp =1.80;			//速度Kp
		rightspeed_Ki = 2/200;			//速度Ki
		rightspeed_Kd =0.08;				//速度Kd
		rightspeed_Target =00;	        //速度目标值

        DL_GPIO_setPins(GPIO_R_PORT, GPIO_R_PIN_R_B_PIN);
        DL_GPIO_clearPins(GPIO_R_PORT,GPIO_R_PIN_R_A_PIN);
        DL_GPIO_setPins(GPIO_L_PORT, GPIO_L_PIN_L_B_PIN);
        DL_GPIO_clearPins(GPIO_L_PORT,GPIO_L_PIN_L_A_PIN); 

    while (1) 
    {   
        if(stop_flag==0)
        xunxian();
    }
}
/* 建议的积分限幅，防止积分饱和 */
#define INT_LIMIT  30.0f
/* PWM 最大计数（你的定时器上限是 7200)*/
#define PWM_MAX    200.0f
/* ======================================================================== */


/* ====================== 计数定时器中断 ====================== */
void TIMER_COUNT_INST_IRQHandler(void)
{
    if(zhuanxiang_time_flag)
    {

        if(++timer_zhuanxiang>=450)
        {zhuanxiang_time_flag=0;
        timer_zhuanxiang=0;
        DL_GPIO_clearPins(GPIO_L_PORT,GPIO_L_PIN_L_A_PIN); 
        }

    }
    if(die_flag==1)
    {
    if(++timer_xms>=3000)
    {
        timer_xms=0;
        die_flag=0;

    }
    }
    if(++count1>=10)
            {count1=0;
                static uint8_t Key_Old = 0;
                uint8_t Key_Temp = Key_Read();
                uint8_t Key_Val = (Key_Temp ^ Key_Old) & Key_Temp;
                Key_Old = Key_Temp;

                if (Key_Val==2)
                {
                    DL_GPIO_setPins(GPIO_R_PORT, GPIO_R_PIN_R_B_PIN);
        DL_GPIO_clearPins(GPIO_R_PORT,GPIO_R_PIN_R_A_PIN);
        DL_GPIO_setPins(GPIO_L_PORT, GPIO_L_PIN_L_B_PIN);
        DL_GPIO_clearPins(GPIO_L_PORT,GPIO_L_PIN_L_A_PIN); 
                    stop_flag=0;
                    leftspeed_Target=80;
                    rightspeed_Target=80;
                }
                else if(Key_Val==1){
                   if(++quannum==6)quannum=1;
                }
            }
    switch (DL_TimerG_getPendingInterrupt(TIMER_COUNT_INST)) {
        case DL_TIMER_IIDX_ZERO:

            if (++count >= 50) {          /* 50 次进入一次速度环 */
                count = 0;

                /* ----------- 1. 采样实际速度并取绝对值 ----------- */
                rightspeed_Actual = Motor_R_count < 0 ? -Motor_R_count : Motor_R_count;
                leftspeed_Actual  = Motor_L_count < 0 ? -Motor_L_count : Motor_L_count;
                Motor_L_count = 0;
                Motor_R_count = 0;

                /* ========== 2. 左轮 PID ========== */
                leftspeed_Error1 = leftspeed_Error0;
                leftspeed_Error0 = leftspeed_Target - leftspeed_Actual;   /* 目标 - 实际  */

                leftspeed_ErrorInt += leftspeed_Error0;                   /* 积分累加当前误差 */
                /* 积分限幅（防积分饱和） */
                if (leftspeed_ErrorInt >  INT_LIMIT) leftspeed_ErrorInt =  INT_LIMIT;
                if (leftspeed_ErrorInt < -INT_LIMIT) leftspeed_ErrorInt = -INT_LIMIT;

                leftspeed_Out =  leftspeed_Kp * leftspeed_Error0
                               + leftspeed_Ki * leftspeed_ErrorInt
                               + leftspeed_Kd * (leftspeed_Error0 - leftspeed_Error1);

                /* 输出限幅（不可小于 0，且 ≤ PWM_MAX） */
                if (leftspeed_Out > PWM_MAX) leftspeed_Out = PWM_MAX;
                if (leftspeed_Out < 0      ) leftspeed_Out = 0;


                /* ========== 3. 右轮 PID ========== */
                rightspeed_Error1 = rightspeed_Error0;
                rightspeed_Error0 = rightspeed_Target - rightspeed_Actual;

                rightspeed_ErrorInt += rightspeed_Error0;
                if (rightspeed_ErrorInt >  INT_LIMIT) rightspeed_ErrorInt =  INT_LIMIT;
                if (rightspeed_ErrorInt < -INT_LIMIT) rightspeed_ErrorInt = -INT_LIMIT;

                rightspeed_Out = rightspeed_Kp * rightspeed_Error0
                               + rightspeed_Ki * rightspeed_ErrorInt
                               + rightspeed_Kd * (rightspeed_Error0 - rightspeed_Error1);

                if (rightspeed_Out > PWM_MAX) rightspeed_Out = PWM_MAX;
                if (rightspeed_Out <       0) rightspeed_Out = 0;
            }

            /* 4. 更新 PWM 占空比 */
            DL_TimerG_setCaptureCompareValue(PWM_INST, rightspeed_Out, GPIO_PWM_C1_IDX);
            DL_TimerG_setCaptureCompareValue(PWM_INST, leftspeed_Out,  GPIO_PWM_C0_IDX);
            break;

        default:
            break;
    }
}




// //灰度巡线封装
// void xunxian(void)
// {

//     if (DL_GPIO_readPins(GPIO_HUIDU_PIN_L2_PORT,GPIO_HUIDU_PIN_L2_PIN)) 
//     {//1XXXX
       
//         rightspeed_Target=35;
//         leftspeed_Target=0;

//     	DL_GPIO_setPins(GPIO_L_PORT,GPIO_L_PIN_L_A_PIN);   
//         DL_GPIO_setPins(GPIO_L_PORT,GPIO_L_PIN_L_B_PIN); 
		
//         zhuanxiang_time_flag=1;
//             countquan_flag=1;
              
//     }
//     if (!DL_GPIO_readPins(GPIO_HUIDU_PIN_L2_PORT,GPIO_HUIDU_PIN_L2_PIN)) 
//     {//0XXXX
          
//         if(countquan_flag==1&&die_flag==0){countquannum++;
//         die_flag=1;
//         }
//         if(countquannum>=4*quannum){
//             rightspeed_Target=0;
//             leftspeed_Target=0;
//             stop_flag=1; 
//             DL_GPIO_clearPins(GPIO_R_PORT, GPIO_R_PIN_R_B_PIN);
//         DL_GPIO_clearPins(GPIO_R_PORT,GPIO_R_PIN_R_A_PIN);
//         DL_GPIO_clearPins(GPIO_L_PORT, GPIO_L_PIN_L_B_PIN);
//         DL_GPIO_clearPins(GPIO_L_PORT,GPIO_L_PIN_L_A_PIN); 
//         countquannum=0;
//         }
//             countquan_flag=0;
            
//     }
//     if(DL_GPIO_readPins(GPIO_HUIDU_PIN_L1_PORT,GPIO_HUIDU_PIN_L1_PIN)&&!DL_GPIO_readPins(GPIO_HUIDU_PIN_M_PORT,GPIO_HUIDU_PIN_M_PIN)) 
//     {//1X0XX
//         rightspeed_Target=80;
//         leftspeed_Target=40;
//     }
//     if (DL_GPIO_readPins(GPIO_HUIDU_PIN_M_PORT,GPIO_HUIDU_PIN_M_PIN)) 
//     {//xx1xx
//         rightspeed_Target=60;
//         leftspeed_Target=60;
//     }
//     if (DL_GPIO_readPins(GPIO_HUIDU_PIN_R1_PORT,GPIO_HUIDU_PIN_R1_PIN)&&!DL_GPIO_readPins(GPIO_HUIDU_PIN_M_PORT,GPIO_HUIDU_PIN_M_PIN)) 
//     {//xx01x
//         rightspeed_Target=60;
//         leftspeed_Target=80;
//     }
//     if (DL_GPIO_readPins(GPIO_HUIDU_PIN_R2_PORT,GPIO_HUIDU_PIN_R2_PIN)) 
//     {//xxxx1
//         rightspeed_Target=50;
//         leftspeed_Target=80;
//     }
//     if (DL_GPIO_readPins(GPIO_HUIDU_PIN_L1_PORT,GPIO_HUIDU_PIN_L1_PIN)&&DL_GPIO_readPins(GPIO_HUIDU_PIN_M_PORT,GPIO_HUIDU_PIN_M_PIN)) 
//     {//x11xx
//         rightspeed_Target=59;
//         leftspeed_Target=60;

//     }
//     if (DL_GPIO_readPins(GPIO_HUIDU_PIN_R2_PORT,GPIO_HUIDU_PIN_R2_PIN)&&DL_GPIO_readPins(GPIO_HUIDU_PIN_M_PORT,GPIO_HUIDU_PIN_M_PIN)) 
//     {//xx1x1
//         rightspeed_Target=60;
//         leftspeed_Target=60;
//     }
//         if (DL_GPIO_readPins(GPIO_HUIDU_PIN_R1_PORT,GPIO_HUIDU_PIN_R2_PIN)&&DL_GPIO_readPins(GPIO_HUIDU_PIN_M_PORT,GPIO_HUIDU_PIN_M_PIN)) 
//     {//xx11x
//         rightspeed_Target=60;
//         leftspeed_Target=59;
//     }
// }
void xunxian(void)
{
    // 如果正在转弯，不执行其他逻辑
    if (zhuanxiang_time_flag == 1) {
        return;
    }
    
    // 检测最左侧传感器
    if (DL_GPIO_readPins(GPIO_HUIDU_PIN_L2_PORT, GPIO_HUIDU_PIN_L2_PIN)) {
        rightspeed_Target = 49;
        leftspeed_Target = 0;

        DL_GPIO_setPins(GPIO_L_PORT, GPIO_L_PIN_L_A_PIN);
        DL_GPIO_setPins(GPIO_L_PORT, GPIO_L_PIN_L_B_PIN);

        zhuanxiang_time_flag = 1;
        countquan_flag = 1;

        // 发送转弯信号给STM32F407VET6
        UART_SendTurnSignal();

        return;
    }
    
    // 圈数计数逻辑
    if (countquan_flag == 1 && die_flag == 0) {
        countquannum++;
        die_flag = 1;
    }
    
    // if (countquannum >= 4 * quannum) {
    if (countquannum >= 4*quannum) {
        rightspeed_Target = 0;
        leftspeed_Target = 0;
        stop_flag = 1;
        // 停止电机代码...
        countquannum = 0;
        return;
    }
    countquan_flag = 0;

    // 正常巡线逻辑
    bool L1 = DL_GPIO_readPins(GPIO_HUIDU_PIN_L1_PORT, GPIO_HUIDU_PIN_L1_PIN);
    bool M = DL_GPIO_readPins(GPIO_HUIDU_PIN_M_PORT, GPIO_HUIDU_PIN_M_PIN);
    bool R1 = DL_GPIO_readPins(GPIO_HUIDU_PIN_R1_PORT, GPIO_HUIDU_PIN_R1_PIN);
    bool R2 = DL_GPIO_readPins(GPIO_HUIDU_PIN_R2_PORT, GPIO_HUIDU_PIN_R2_PIN);
    
    // 传感器逻辑...
    if (L1 && !M) {
        rightspeed_Target = 80;
        leftspeed_Target = 40;
    } else if (M) {
        if (L1 && !R1) {
            rightspeed_Target = 61;
            leftspeed_Target = 62;
        } else if (R2) {
            rightspeed_Target = 62;
            leftspeed_Target = 62;
        } else if (R1) {
            rightspeed_Target = 62;
            leftspeed_Target = 61;
        } else {
            rightspeed_Target = 62;
            leftspeed_Target = 62;
        }
    } else if (R1 && !M) {
        rightspeed_Target = 60;
        leftspeed_Target = 80;
    } else if (R2) {
        rightspeed_Target = 50;
        leftspeed_Target = 80;
    }
}

/* ====================== UART中断处理函数 ====================== */
void UART_OPENMV_INST_IRQHandler(void)
{
    switch (DL_UART_getPendingInterrupt(UART_OPENMV_INST)) {
        case DL_UART_IIDX_RX:
            gEchoData = DL_UART_receiveData(UART_OPENMV_INST);
            // 这里可以处理从STM32F407VET6接收到的数据
            break;
        default:
            break;
    }
}



