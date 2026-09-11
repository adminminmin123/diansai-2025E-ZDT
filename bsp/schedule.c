#include "schedule.h"

typedef struct {
	void (*task_func)(void);
	uint32_t rate_ms;
	uint32_t last_run;
}schedule_task_t;

uint8_t task_num;

static schedule_task_t schedule_task[] = {
//	{oled_proc,100,0},
	{uart_proc,1,0},
	{pi_proc,20,0}

//	{motor_proc,20,0},
//	{encoder_proc,20,0},
//	{key_proc,10,0},
//	{gray_proc,20,0}
	
};

/**
 * @brief 调度器初始化函数
 * 计算任务数组的元素个数，并将结果存储在 task_num 中
 */
void schedule_init(void)
{
	task_num = sizeof(schedule_task) / sizeof(schedule_task_t);
}

/**
 * @brief 调度器运行函数
 * 遍历任务数组，检查是否有任务需要执行。如果当前时间已经超过任务的执行周期，则执行该任务并更新上次运行时间
 */
void schedule_run(void)
{
    // 遍历任务数组中的所有任务
    for (uint8_t i = 0; i < task_num; i++)
    {
        // 获取当前的系统时间（毫秒）
        uint32_t now_time = HAL_GetTick();

        // 检查当前时间是否达到任务的执行时间
        if (now_time >= schedule_task[i].rate_ms + schedule_task[i].last_run)
        {
            // 更新任务的上次运行时间为当前时间
            schedule_task[i].last_run = now_time;

            // 执行任务函数
            schedule_task[i].task_func();
        }
    }
}
