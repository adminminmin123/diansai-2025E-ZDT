#include "laser_bsp.h"
#include "uart_bsp.h"

/* Private variables ---------------------------------------------------------*/
static LaserControl_t laser_control = {
    .state = LASER_STATE_OFF,
    .mode = LASER_MODE_AUTO,
    .system_mode = SYSTEM_MODE_0,
    .fire_start_time = 0,
    .fire_duration_ms = 0,
    .manual_override = 0,
    .target_locked = 0,
    .system_start_time = 0,
    .mode1_fired = 0,
    .mode2_start_time = 0,
    .mode2_timing = 0,
    .mode2_fired = 0,
    .mode3_start_time = 0,
    .mode3_tracking = 0,
    .mode3_fired = 0,
    .system_started = 0,
    .motors_enabled = 0};

/* Private function prototypes -----------------------------------------------*/
static void Laser_UpdateHardware(void);

/**
 * @brief 激光控制系统初始化
 */
void Laser_Init(void)
{
    // 初始化激光控制结构体
    laser_control.state = LASER_STATE_OFF;
    laser_control.mode = LASER_MODE_AUTO;      // 默认自动模式
    laser_control.system_mode = SYSTEM_MODE_0; // 默认模式0（待机模式）
    laser_control.fire_start_time = 0;
    laser_control.fire_duration_ms = LASER_AUTO_FIRE_DURATION_MS;
    laser_control.manual_override = 0;
    laser_control.target_locked = 0;
    laser_control.system_start_time = 0; // 初始化时不设置启动时间，等待START按键
    laser_control.mode1_fired = 0;       // 模式1未发射
    laser_control.mode2_start_time = 0;  // 模式2计时开始时间
    laser_control.mode2_timing = 0;      // 模式2未开始计时
    laser_control.mode2_fired = 0;       // 模式2未发射
    laser_control.mode3_start_time = 0;  // 模式3追踪开始时间
    laser_control.mode3_tracking = 0;    // 模式3未开始追踪
    laser_control.mode3_fired = 0;       // 模式3未发射
    laser_control.system_started = 0;    // 系统未启动
    laser_control.motors_enabled = 0;    // 电机未使能

    // 强制确保激光初始状态为关闭 - 设置高电平断开继电器
    HAL_GPIO_WritePin(Laser_GPIO_Port, Laser_Pin, GPIO_PIN_SET);

    // 初始化LED状态 - 模式1：LED1亮，LED2灭
    Laser_UpdateLEDs();

    // 延时确保GPIO状态稳定
    HAL_Delay(10);

    my_printf(&huart1, "LASER: System initialized - Mode: AUTO, System Mode: 0, State: OFF\r\n");
    my_printf(&huart1, "LASER: Mode 0 - Standby mode, motors disabled (LED OFF)\r\n");
    my_printf(&huart1, "LASER: Mode 1 - Y-axis reset + 1.5s auto fire, NO target tracking, NO X-axis (LED1 ON)\r\n");
    my_printf(&huart1, "LASER: Mode 2 - Start button timing 3.5s (LED2 ON)\r\n");
    my_printf(&huart1, "LASER: Mode 3 - Immediate fire on target + 20s continuous, enhanced stability (LED1+LED2 ON)\r\n");
    my_printf(&huart1, "LASER: KEY1 SHORT PRESS = Manual laser control\r\n");
    my_printf(&huart1, "LASER: KEY2 SHORT PRESS = Switch system mode\r\n");
    my_printf(&huart1, "LASER: Start button = Begin system operation\r\n");
}

/**
 * @brief 激光控制主处理函数 - 在调度器中定期调用
 */
void Laser_Process(void)
{
    uint32_t current_time = HAL_GetTick();
    static uint32_t last_status_print = 0;

    // 只有在系统启动后才执行模式逻辑
    if (!laser_control.system_started)
    {
        // 模式0下额外保护：确保激光关闭（除非手动覆盖）
        if (laser_control.system_mode == SYSTEM_MODE_0 && !laser_control.manual_override)
        {
            if (laser_control.state != LASER_STATE_OFF)
            {
                laser_control.state = LASER_STATE_OFF;
                my_printf(&huart1, "LASER: Mode 0 protection - Force OFF\r\n");
            }
        }

        // 更新硬件状态（手动控制仍可工作）
        Laser_UpdateHardware();
        return;
    }

    // 模式1特殊处理：系统启动后1.5秒后自动发射（除非手动覆盖）
    // 只有在系统已启动且未发射的情况下才执行，并且系统启动时间必须有效
    if (laser_control.system_mode == SYSTEM_MODE_1 && laser_control.system_started &&
        !laser_control.mode1_fired && !laser_control.manual_override &&
        laser_control.system_start_time > 0)
    {
        uint32_t elapsed_time = current_time - laser_control.system_start_time;

        if (elapsed_time >= MODE1_AUTO_FIRE_DELAY_MS)
        {
            // 0.5秒时间到，强制启动激光发射
            laser_control.state = LASER_STATE_AUTO_FIRING;
            laser_control.fire_start_time = current_time;
            laser_control.mode1_fired = 1; // 标记已发射，避免重复发射
            my_printf(&huart1, "LASER: Mode 1 - Auto fire started after 1.5s!\r\n");
        }
    }

    // 模式2特殊处理：系统启动且开始计时后3.5秒自动发射（除非手动覆盖）
    if (laser_control.system_mode == SYSTEM_MODE_2 && laser_control.system_started && laser_control.mode2_timing && !laser_control.mode2_fired && !laser_control.manual_override)
    {
        uint32_t elapsed_time = current_time - laser_control.mode2_start_time;

        if (elapsed_time >= MODE2_AUTO_FIRE_DELAY_MS)
        {
            // 3.5秒时间到，强制启动激光发射
            laser_control.state = LASER_STATE_AUTO_FIRING;
            laser_control.fire_start_time = current_time;
            laser_control.mode2_fired = 1;  // 标记已发射，避免重复发射
            laser_control.mode2_timing = 0; // 停止计时
            my_printf(&huart1, "LASER: Mode 2 - Auto fire started after 3.5s!\r\n");
        }
    }

    // 模式3特殊处理：系统启动后一旦识别到目标就立刻发射激光，持续20秒（除非手动覆盖）
    if (laser_control.system_mode == SYSTEM_MODE_3 && laser_control.system_started && !laser_control.manual_override)
    {
        // 检查是否有目标锁定且尚未开始追踪
        if (laser_control.target_locked && !laser_control.mode3_tracking)
        {
            // 目标锁定，立刻开始追踪模式和激光发射
            laser_control.mode3_start_time = current_time;
            laser_control.mode3_tracking = 1;
            laser_control.state = LASER_STATE_AUTO_FIRING;
            laser_control.fire_start_time = current_time;
            laser_control.mode3_fired = 1;
            my_printf(&huart1, "LASER: Mode 3 - Target detected, immediate laser firing started (20s continuous duration)\r\n");
        }

        if (laser_control.mode3_tracking)
        {
            uint32_t elapsed_time = current_time - laser_control.mode3_start_time;

            // 模式3新逻辑：一旦开始发射，激光保持开启状态20秒，无论目标是否丢失
            if (laser_control.mode3_fired && elapsed_time < MODE3_TRACKING_DURATION_MS)
            {
                // 强制确保激光保持开启状态，无论目标状态如何
                if (laser_control.state != LASER_STATE_AUTO_FIRING)
                {
                    laser_control.state = LASER_STATE_AUTO_FIRING;
                    my_printf(&huart1, "LASER: Mode 3 - Force maintaining laser ON state (continuous firing)\r\n");
                }

                // 每2秒打印一次状态
                static uint32_t last_mode3_print = 0;
                if (current_time - last_mode3_print > 2000)
                {
                    uint32_t remaining_ms = MODE3_TRACKING_DURATION_MS - elapsed_time;
                    my_printf(&huart1, "LASER: Mode 3 continuous firing - %.1fs remaining (target_locked=%d, forced ON)\r\n",
                              remaining_ms / 1000.0f, laser_control.target_locked);
                    last_mode3_print = current_time;
                }
            }

            // 检查20秒时间是否到达
            if (elapsed_time >= MODE3_TRACKING_DURATION_MS)
            {
                // 20秒时间到，停止追踪模式和激光发射
                laser_control.state = LASER_STATE_OFF;
                laser_control.mode3_tracking = 0;
                laser_control.mode3_fired = 0;
                my_printf(&huart1, "LASER: Mode 3 - 20s continuous firing completed, laser automatically stopped\r\n");
            }
        }
    }

    // 自动发射超时检查（模式3除外，模式3由自己的逻辑控制；手动覆盖时也不检查）
    if (laser_control.state == LASER_STATE_AUTO_FIRING && laser_control.system_mode != SYSTEM_MODE_3 && !laser_control.manual_override)
    {
        if (current_time - laser_control.fire_start_time >= laser_control.fire_duration_ms)
        {
            // 自动发射时间到，关闭激光
            laser_control.state = LASER_STATE_OFF;
            my_printf(&huart1, "LASER: Auto fire completed - OFF\r\n");
        }
    }

    // 更新硬件状态
    Laser_UpdateHardware();
}

/**
 * @brief 设置激光控制模式
 * @param mode 控制模式
 */
void Laser_SetMode(LaserMode_t mode)
{
    if (laser_control.mode != mode)
    {
        laser_control.mode = mode;
        laser_control.manual_override = 0;

        if (mode == LASER_MODE_AUTO)
        {
            // 切换到自动模式时关闭激光
            laser_control.state = LASER_STATE_OFF;
            my_printf(&huart1, "LASER: Mode set to AUTO\r\n");
        }
        else
        {
            my_printf(&huart1, "LASER: Mode set to MANUAL\r\n");
        }
    }
}

/**
 * @brief 手动控制激光开关
 * @param enable 1-开启, 0-关闭
 */
void Laser_ManualControl(uint8_t enable)
{
    // KEY1手动控制在任何模式下都可以工作，设置手动覆盖标志
    laser_control.manual_override = enable;

    if (enable)
    {
        // 手动开启激光
        laser_control.state = LASER_STATE_ON;
        my_printf(&huart1, "LASER: Manual control - ON (Override active)\r\n");
    }
    else
    {
        // 手动关闭激光
        laser_control.state = LASER_STATE_OFF;
        my_printf(&huart1, "LASER: Manual control - OFF (Override cleared)\r\n");
    }

    // 立即更新硬件状态，确保激光状态立即生效
    Laser_UpdateHardware();
}

/**
 * @brief 自动发射激光
 */
void Laser_AutoFire(void)
{
    if (laser_control.mode == LASER_MODE_AUTO && !laser_control.manual_override)
    {
        laser_control.state = LASER_STATE_AUTO_FIRING;
        laser_control.fire_start_time = HAL_GetTick();
        my_printf(&huart1, "LASER: Auto fire started - Duration: %dms\r\n", laser_control.fire_duration_ms);
    }
}

/**
 * @brief 停止激光发射
 */
void Laser_Stop(void)
{
    laser_control.state = LASER_STATE_OFF;
    laser_control.manual_override = 0;
    my_printf(&huart1, "LASER: Force stopped\r\n");
}

/**
 * @brief 目标锁定回调函数
 */
void Laser_OnTargetLocked(void)
{
    laser_control.target_locked = 1;

    // 只有在系统已启动的情况下才响应目标锁定
    if (!laser_control.system_started)
    {
        my_printf(&huart1, "LASER: Target locked but system not started - ignoring\r\n");
        return;
    }

    // 模式1特殊处理：即便识别到目标也不做任何处理
    if (laser_control.system_mode == SYSTEM_MODE_1)
    {
        my_printf(&huart1, "LASER: Mode 1 - Target locked but ignored (no target processing in Mode 1)\r\n");
        return;
    }

    // 根据系统模式决定是否自动发射
    if (laser_control.mode == LASER_MODE_AUTO && !laser_control.manual_override)
    {
        if (laser_control.system_mode == SYSTEM_MODE_2)
        {
            // 模式2：目标锁定时发射（仅在系统启动后）
            Laser_AutoFire();
            my_printf(&huart1, "LASER: Mode 2 - Target locked, auto fire triggered\r\n");
        }
        else if (laser_control.system_mode == SYSTEM_MODE_3)
        {
            // 模式3：目标锁定时立刻发射（由Laser_Process()中的逻辑处理）
            my_printf(&huart1, "LASER: Mode 3 - Target locked, will trigger immediate continuous firing (20s)\r\n");
        }
    }
}

/**
 * @brief 目标丢失回调函数
 */
void Laser_OnTargetLost(void)
{
    laser_control.target_locked = 0;

    // 模式3特殊处理：即使目标丢失，激光也要继续保持开启状态直到20秒结束
    if (laser_control.system_mode == SYSTEM_MODE_3 && laser_control.mode3_tracking)
    {
        my_printf(&huart1, "LASER: Mode 3 - Target lost but laser continues firing (20s continuous mode)\r\n");
        // 不停止激光，让20秒计时器自然结束
        return;
    }

    // 其他模式：如果正在自动发射，继续完成发射周期
    // 不立即停止，让激光完成1秒发射
}

/**
 * @brief 获取激光状态
 * @return 当前激光状态
 */
LaserState_t Laser_GetState(void)
{
    return laser_control.state;
}

/**
 * @brief 获取激光模式
 * @return 当前激光模式
 */
LaserMode_t Laser_GetMode(void)
{
    return laser_control.mode;
}

/**
 * @brief 设置系统工作模式
 * @param mode 系统模式
 */
void Laser_SetSystemMode(SystemMode_t mode)
{
    if (laser_control.system_mode != mode)
    {
        // 强制关闭激光并清除所有状态
        laser_control.state = LASER_STATE_OFF;
        laser_control.manual_override = 0; // 清除手动覆盖标志

        // 立即更新硬件状态确保激光关闭
        Laser_UpdateHardware();

        laser_control.system_mode = mode;

        // 切换模式时重置系统启动状态和时间
        laser_control.system_started = 0;
        laser_control.system_start_time = 0; // 重置系统启动时间，防止使用旧时间

        // 重置所有模式相关的状态标志，确保干净的状态切换
        laser_control.mode1_fired = 0;
        laser_control.mode2_start_time = 0;
        laser_control.mode2_timing = 0;
        laser_control.mode2_fired = 0;
        laser_control.mode3_start_time = 0;
        laser_control.mode3_tracking = 0;
        laser_control.mode3_fired = 0;
        laser_control.target_locked = 0; // 重置目标锁定状态

        if (mode == SYSTEM_MODE_0)
        {
            // 切换到模式0：待机模式，禁用电机
            laser_control.motors_enabled = 0;
            my_printf(&huart1, "LASER: Switched to Mode 0 - Standby mode, motors disabled\r\n");
            my_printf(&huart1, "LASER: Mode 0 - Laser forced OFF, all states reset\r\n");
            my_printf(&huart1, "LASER: Press START to enable motors and begin operation\r\n");
        }
        else if (mode == SYSTEM_MODE_1)
        {
            // 切换到模式1：所有状态已在上面重置
            my_printf(&huart1, "LASER: Switched to Mode 1 - Y-axis reset + auto fire, NO target tracking, NO X-axis\r\n");
            my_printf(&huart1, "LASER: Press START to execute Y-axis reset and begin timer-based firing\r\n");
        }
        else if (mode == SYSTEM_MODE_2)
        {
            // 切换到模式2：所有状态已在上面重置
            my_printf(&huart1, "LASER: Switched to Mode 2 - Start button timing mode\r\n");
            my_printf(&huart1, "LASER: Press START to enable motors and begin countdown\r\n");
        }
        else if (mode == SYSTEM_MODE_3)
        {
            // 切换到模式3：所有状态已在上面重置
            my_printf(&huart1, "LASER: Switched to Mode 3 - Enhanced stability target tracking + 20s continuous firing\r\n");
            my_printf(&huart1, "LASER: Press START to enable motors and begin enhanced tracking\r\n");
        }

        // 更新LED状态
        Laser_UpdateLEDs();
    }
}

/**
 * @brief 获取系统工作模式
 * @return 当前系统模式
 */
SystemMode_t Laser_GetSystemMode(void)
{
    return laser_control.system_mode;
}

/**
 * @brief 更新LED指示状态
 */
void Laser_UpdateLEDs(void)
{
    if (laser_control.system_mode == SYSTEM_MODE_0)
    {
        // 模式0：LED1和LED2都灭
        LED1_OFF();
        LED2_OFF();
    }
    else if (laser_control.system_mode == SYSTEM_MODE_1)
    {
        // 模式1：LED1亮，LED2灭
        LED1_ON();
        LED2_OFF();
    }
    else if (laser_control.system_mode == SYSTEM_MODE_2)
    {
        // 模式2：LED2亮，LED1灭
        LED1_OFF();
        LED2_ON();
    }
    else if (laser_control.system_mode == SYSTEM_MODE_3)
    {
        // 模式3：LED1和LED2都亮
        LED1_ON();
        LED2_ON();
    }
}

/**
 * @brief 启动模式2计时
 * @note 在模式2下按下Start按键时调用
 */
void Laser_StartMode2Timing(void)
{
    if (laser_control.system_mode == SYSTEM_MODE_2)
    {
        uint32_t current_time = HAL_GetTick();

        // 如果已经在计时，重新开始计时
        if (laser_control.mode2_timing)
        {
            my_printf(&huart1, "LASER: Mode 2 timing restarted - 3.5s countdown begins again\r\n");
        }
        else
        {
            my_printf(&huart1, "LASER: Mode 2 timing started - 3.5s countdown begins\r\n");
        }

        laser_control.mode2_start_time = current_time;
        laser_control.mode2_timing = 1;
        laser_control.mode2_fired = 0;
    }
}

/**
 * @brief 停止模式2计时
 * @note 可用于手动停止计时或重置状态
 */
void Laser_StopMode2Timing(void)
{
    laser_control.mode2_timing = 0;
    laser_control.mode2_start_time = 0;
    my_printf(&huart1, "LASER: Mode 2 timing stopped\r\n");
}

/**
 * @brief 启动系统
 * @note 设置系统启动标志和使能电机，重置系统启动时间
 */
void Laser_StartSystem(void)
{
    laser_control.system_started = 1;
    laser_control.motors_enabled = 1;
    laser_control.system_start_time = HAL_GetTick(); // 重置系统启动时间为当前时间
    my_printf(&huart1, "LASER: System started - Motors enabled, timer reset\r\n");
}

/**
 * @brief 检查系统是否已启动
 * @return 1-已启动，0-未启动
 */
uint8_t Laser_IsSystemStarted(void)
{
    return laser_control.system_started;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 更新激光硬件状态
 */
static void Laser_UpdateHardware(void)
{
    if (laser_control.state == LASER_STATE_ON || laser_control.state == LASER_STATE_AUTO_FIRING)
    {
        LASER_ON();
    }
    else
    {
        LASER_OFF();
    }
}
