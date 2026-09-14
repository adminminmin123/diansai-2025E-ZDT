#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
 #include <stdint.h>
 extern uint32_t SystemCoreClock;
#endif

#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    ( 8 )
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 128 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 16 * 1024 ) )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_TRACE_FACILITY                0
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       0
#define configQUEUE_REGISTRY_SIZE               0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_APPLICATION_TASK_TAG          0
#define configUSE_COUNTING_SEMAPHORES           0
#define configGENERATE_RUN_TIME_STATS           0
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configUSE_TICKLESS_IDLE                 0   /* 与 HAL 共享 SysTick，禁低功耗停表 */

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         ( 2 )

/* Software timer definitions. */
#define configUSE_TIMERS                        0
#define configTIMER_TASK_PRIORITY               ( 2 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* Include API functions. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     0
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_vTaskSuspend                    0
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1   /* SysTick_Handler 判断调度器状态 */
#define INCLUDE_uxTaskGetStackHighWaterMark     1   /* 验证栈余量 */
#define INCLUDE_xTaskGetCurrentTaskHandle       1   /* stream_buffer.c 引用此函数 */

/* Cortex-M4 specific definitions. */
#ifdef __NVIC_PRIO_BITS
  #define configPRIO_BITS                       __NVIC_PRIO_BITS
#else
  #define configPRIO_BITS                       4   /* 与 HAL_Init 的 NVIC_PRIORITYGROUP_4 一致 */
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority" function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY          0x0F

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY     5

/* 移位换算: 优先级组 4 => 抢占优先级占高 4 位, 库值 << 4 得到寄存器值
   configKERNEL_INTERRUPT_PRIORITY = 0x0F<<4 = 240 (PendSV/SVC 最低优先级)
   configMAX_SYSCALL_INTERRUPT_PRIORITY = 5<<4 = 80 */
#define configKERNEL_INTERRUPT_PRIORITY \
        ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!! */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
        ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler        SVC_Handler
#define xPortPendSVHandler     PendSV_Handler

/* IMPORTANT: SysTick 不重映射 —— 本工程 SysTick_Handler 由 HAL 与 FreeRTOS 共享,
   在 stm32f4xx_it.c 的 SysTick_Handler 里先 HAL_IncTick() 再调 xPortSysTickHandler()。
   #define xPortSysTickHandler SysTick_Handler   <-- 此行必须保持注释 */

#endif /* FREERTOS_CONFIG_H */
