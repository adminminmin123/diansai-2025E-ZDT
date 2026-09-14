/**
 * @file    app_freertos.c
 * @brief   FreeRTOS 任务层：原协作调度器 4 个周期槽拆分为 4 个 RTOS 任务
 *          uart_proc 10ms / pi_proc 10ms / key_proc 20ms / Laser_Process 10ms
 */
#include "app_freertos.h"
#include "bsp_system.h"   /* 声明 uart_proc/pi_proc/key_proc/Laser_Process */

/* 周期: 与原协作调度器 4 个槽位完全一致 (10/10/20/10 ms) */
#define UART_TASK_PERIOD_MS    10
#define PI_TASK_PERIOD_MS      10
#define KEY_TASK_PERIOD_MS     20
#define LASER_TASK_PERIOD_MS   10

/* 任务栈深度(单位: word=4字节)。
   128 words 不够: my_printf() 有 512 字节局部缓冲 + vsnprintf(标准库, 非 microlib),
   uart_proc 另有 Emm_V5_Response_t 等较大局部变量, 均在此栈上消耗。 */
#define UART_TASK_STACK_WORDS   512
#define PI_TASK_STACK_WORDS     512
#define KEY_TASK_STACK_WORDS    512
#define LASER_TASK_STACK_WORDS  512

static void vUartProcTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        uart_proc();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(UART_TASK_PERIOD_MS));
    }
}

static void vPiProcTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        pi_proc();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PI_TASK_PERIOD_MS));
    }
}

static void vKeyProcTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        key_proc();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(KEY_TASK_PERIOD_MS));
    }
}

static void vLaserProcTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        Laser_Process();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(LASER_TASK_PERIOD_MS));
    }
}

void app_freertos_init(void)
{
    /* 优先级设计: configMAX_PRIORITIES=8, 空闲任务=0。
       4 个任务同优先级 2, 时间片轮转, 尽量贴近原顺序执行的语义;
       key 任务周期最长(20ms), 优先级 1, 让 10ms 实时性任务优先。
       任何任务优先级均与中断无关: 外设 IRQ(优先级 0)永远抢占所有任务。 */
    xTaskCreate(vUartProcTask,  "uart_task",  UART_TASK_STACK_WORDS,  NULL, 2, NULL);
    xTaskCreate(vPiProcTask,    "pi_task",    PI_TASK_STACK_WORDS,    NULL, 2, NULL);
    xTaskCreate(vLaserProcTask, "laser_task", LASER_TASK_STACK_WORDS, NULL, 2, NULL);
    xTaskCreate(vKeyProcTask,   "key_task",   KEY_TASK_STACK_WORDS,   NULL, 1, NULL);
}

/* configCHECK_FOR_STACK_OVERFLOW=2 要求提供此钩子, 缺省会链接失败 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}
