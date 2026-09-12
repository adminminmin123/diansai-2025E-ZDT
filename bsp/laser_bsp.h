#ifndef __LASER_BSP_H__
#define __LASER_BSP_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

    /* Exported types ------------------------------------------------------------*/
    typedef enum
    {
        LASER_STATE_OFF = 0,        // 激光关闭
        LASER_STATE_ON = 1,         // 激光开启
        LASER_STATE_AUTO_FIRING = 2 // 自动发射中
    } LaserState_t;

    typedef enum
    {
        LASER_MODE_MANUAL = 0, // 手动模式
        LASER_MODE_AUTO = 1    // 自动模式（保留兼容性）
    } LaserMode_t;

    typedef enum
    {
        SYSTEM_MODE_0 = 0, // 模式0：待机模式，电机未使能
        SYSTEM_MODE_1 = 1, // 模式1：Y轴复位后1.5秒自动发射，无目标追踪，无X轴运动
        SYSTEM_MODE_2 = 2, // 模式2：Start按键3.5秒后发射
        SYSTEM_MODE_3 = 3  // 模式3：识别到目标立刻发射，持续20秒，增强稳定性追踪
    } SystemMode_t;

    typedef struct
    {
        LaserState_t state;         // 当前状态
        LaserMode_t mode;           // 控制模式
        SystemMode_t system_mode;   // 系统工作模式
        uint32_t fire_start_time;   // 发射开始时间
        uint32_t fire_duration_ms;  // 发射持续时间(ms)
        uint8_t manual_override;    // 手动覆盖标志
        uint8_t target_locked;      // 目标锁定标志
        uint32_t system_start_time; // 系统启动时间
        uint8_t mode1_fired;        // 模式1是否已发射标志
        uint32_t mode2_start_time;  // 模式2计时开始时间
        uint8_t mode2_timing;       // 模式2是否正在计时
        uint8_t mode2_fired;        // 模式2是否已发射标志
        uint32_t mode3_start_time;  // 模式3追踪开始时间
        uint8_t mode3_tracking;     // 模式3是否正在追踪
        uint8_t mode3_fired;        // 模式3是否已发射标志
        uint8_t system_started;     // 系统是否已启动（按下Start按键）
        uint8_t motors_enabled;     // 电机是否已使能
    } LaserControl_t;

/* Exported constants --------------------------------------------------------*/
#define LASER_AUTO_FIRE_DURATION_MS 500  // 自动发射持续时间1秒
#define LASER_DEBOUNCE_TIME_MS 50        // 按键防抖时间
#define MODE1_AUTO_FIRE_DELAY_MS 1000    // 模式1自动发射延时1秒
#define MODE2_AUTO_FIRE_DELAY_MS 2800    // 模式2自动发射延时2秒
#define MODE3_TRACKING_DURATION_MS 20000 // 模式3追踪持续时间20秒
#define KEY_LONG_PRESS_TIME_MS 2000      // 长按时间2秒

/* Exported macro ------------------------------------------------------------*/
// 继电器模块通常是低电平触发：低电平吸合(激光开)，高电平断开(激光关)
#define LASER_ON() HAL_GPIO_WritePin(Laser_GPIO_Port, Laser_Pin, GPIO_PIN_RESET) // 低电平触发
#define LASER_OFF() HAL_GPIO_WritePin(Laser_GPIO_Port, Laser_Pin, GPIO_PIN_SET)  // 高电平断开
#define LASER_TOGGLE() HAL_GPIO_TogglePin(Laser_GPIO_Port, Laser_Pin)

// LED控制宏定义
#define LED1_ON() HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET)
#define LED1_OFF() HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET)
#define LED2_ON() HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET)
#define LED2_OFF() HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET)

    /* Exported functions prototypes ---------------------------------------------*/
    void Laser_Init(void);
    void Laser_Process(void);
    void Laser_SetMode(LaserMode_t mode);
    void Laser_SetSystemMode(SystemMode_t mode);
    void Laser_ManualControl(uint8_t enable);
    void Laser_AutoFire(void);
    void Laser_Stop(void);
    void Laser_OnTargetLocked(void);
    void Laser_OnTargetLost(void);
    LaserState_t Laser_GetState(void);
    LaserMode_t Laser_GetMode(void);
    SystemMode_t Laser_GetSystemMode(void);
    void Laser_UpdateLEDs(void);
    void Laser_StartMode2Timing(void);
    void Laser_StopMode2Timing(void);
    void Laser_StartSystem(void);
    uint8_t Laser_IsSystemStarted(void);

#ifdef __cplusplus
}
#endif

#endif /* __LASER_BSP_H__ */
