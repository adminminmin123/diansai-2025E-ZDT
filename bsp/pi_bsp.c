#include "pi_bsp.h"
#include "laser_bsp.h"
#include <math.h>

// Camera coordinate data - origin is the target, jiguang is fixed reference point
LaserCoord_t latest_red_laser_coord = {RED_LASER_ID, 0, 0, 0};         // Origin target coordinate
LaserCoord_t latest_green_laser_coord = {GREEN_LASER_ID, 320, 240, 1}; // Fixed jiguang coordinate (320,340)

// Simplified camera data parser - only parse origin coordinates
// Jiguang coordinate is fixed at (320,340)
int pi_parse_data(char *buffer)
{
    if (!buffer)
        return -1;

    // Remove trailing whitespace
    int len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\r' || buffer[len - 1] == '\n' || buffer[len - 1] == ' '))
    {
        buffer[--len] = '\0';
    }

    if (len == 0)
        return -1;

    int parsed_x, parsed_y;
    int parsed_count;

    // Only parse origin coordinates - jiguang is fixed
    if (strncmp(buffer, "origin:", 7) == 0)
    {
        parsed_count = sscanf(buffer, "origin:(%d,%d)", &parsed_x, &parsed_y);
        if (parsed_count != 2)
        {
            return -2; // Parse failed
        }

        // Update origin coordinate
        latest_red_laser_coord.x = parsed_x;
        latest_red_laser_coord.y = parsed_y;
        latest_red_laser_coord.isValid = 1;

        my_printf(&huart1, "Origin: (%d,%d)\r\n", parsed_x, parsed_y);
        return 0;
    }

    return -3; // Unknown format
}

void pi_proc(void)
{
    static uint32_t last_status_print = 0;
    uint32_t current_time = HAL_GetTick();

    // Print status every 3 seconds
    if (current_time - last_status_print > 3000)
    {
        last_status_print = current_time;
        SystemMode_t current_mode = Laser_GetSystemMode();
        my_printf(&huart1, "Status: Origin valid=%d(%d,%d), Jiguang fixed=1(320,240), Mode=%d\r\n",
                  latest_red_laser_coord.isValid, latest_red_laser_coord.x, latest_red_laser_coord.y, current_mode);
    }

    // 获取当前系统模式
    SystemMode_t current_mode = Laser_GetSystemMode();

    // 模式1特殊处理：即便识别到目标点，也不对目标点做任何处理，不启动目标追踪
    if (current_mode == SYSTEM_MODE_1)
    {
        // 模式1下完全禁用目标追踪系统，即便有有效的目标坐标也不处理
        if (latest_red_laser_coord.isValid)
        {
            // 每5秒打印一次状态，说明模式1下忽略目标追踪
            static uint32_t last_mode1_print = 0;
            if (current_time - last_mode1_print > 5000)
            {
                my_printf(&huart1, "MODE1: Target detected at (%d,%d) but tracking disabled - laser fires by timer only\r\n",
                          latest_red_laser_coord.x, latest_red_laser_coord.y);
                last_mode1_print = current_time;
            }
        }
        // 模式1下直接返回，不执行任何目标追踪逻辑
        return;
    }

    // Adaptive PID control logic with gentle startup and Mode 3 stability enhancement
    if (latest_red_laser_coord.isValid)
    {
        // Get current system mode for mode-specific optimizations
        SystemMode_t current_mode = Laser_GetSystemMode();

        // Calculate error distance
        float error_x = latest_green_laser_coord.x - latest_red_laser_coord.x;
        float error_y = latest_green_laser_coord.y - latest_red_laser_coord.y;
        float error_distance = sqrtf(error_x * error_x + error_y * error_y);

        // Mode 3 stability enhancement: Apply advanced coordinate filtering for moving platform
        static float filtered_x = 0.0f, filtered_y = 0.0f;
        static float velocity_x = 0.0f, velocity_y = 0.0f;
        static float last_filtered_x = 0.0f, last_filtered_y = 0.0f;
        static uint8_t filter_initialized = 0;
        static uint32_t last_filter_time = 0;

        if (current_mode == SYSTEM_MODE_3)
        {
            // Initialize filter with first valid coordinate
            if (!filter_initialized)
            {
                filtered_x = latest_red_laser_coord.x;
                filtered_y = latest_red_laser_coord.y;
                last_filtered_x = filtered_x;
                last_filtered_y = filtered_y;
                velocity_x = 0.0f;
                velocity_y = 0.0f;
                last_filter_time = current_time;
                filter_initialized = 1;
                my_printf(&huart1, "MODE3: Advanced coordinate filter initialized at (%.1f,%.1f)\r\n", filtered_x, filtered_y);
            }

            // Calculate time delta for velocity estimation
            uint32_t time_delta = current_time - last_filter_time;
            if (time_delta > 0)
            {
                // Estimate velocity from filtered coordinates
                float dt = time_delta / 1000.0f; // Convert to seconds
                velocity_x = (filtered_x - last_filtered_x) / dt;
                velocity_y = (filtered_y - last_filtered_y) / dt;

                // Limit velocity to reasonable range (prevent noise spikes)
                if (velocity_x > 100.0f)
                    velocity_x = 100.0f;
                if (velocity_x < -100.0f)
                    velocity_x = -100.0f;
                if (velocity_y > 100.0f)
                    velocity_y = 100.0f;
                if (velocity_y < -100.0f)
                    velocity_y = -100.0f;
            }

            // Apply adaptive filtering based on movement detection
            float movement_x = fabs(latest_red_laser_coord.x - filtered_x);
            float movement_y = fabs(latest_red_laser_coord.y - filtered_y);
            float total_movement = sqrtf(movement_x * movement_x + movement_y * movement_y);

            // Adaptive filter strength based on movement magnitude
            float filter_alpha;
            if (total_movement > 50.0f)
            {
                filter_alpha = 0.4f; // Fast response for large movements
            }
            else if (total_movement > 20.0f)
            {
                filter_alpha = 0.25f; // Medium response for medium movements
            }
            else if (total_movement > 10.0f)
            {
                filter_alpha = 0.15f; // Slow response for small movements
            }
            else
            {
                filter_alpha = 0.1f; // Very slow response for minimal movements (noise reduction)
            }

            // Store previous filtered values for velocity calculation
            last_filtered_x = filtered_x;
            last_filtered_y = filtered_y;
            last_filter_time = current_time;

            // Apply filtering with predictive component
            filtered_x = filtered_x * (1.0f - filter_alpha) + latest_red_laser_coord.x * filter_alpha;
            filtered_y = filtered_y * (1.0f - filter_alpha) + latest_red_laser_coord.y * filter_alpha;

            // Use filtered coordinates for error calculation in Mode 3
            error_x = latest_green_laser_coord.x - filtered_x;
            error_y = latest_green_laser_coord.y - filtered_y;
            error_distance = sqrtf(error_x * error_x + error_y * error_y);

            // Print filtered coordinates every 2 seconds
            static uint32_t last_filter_print = 0;
            if (current_time - last_filter_print > 2000)
            {
                my_printf(&huart1, "MODE3: Raw(%.1f,%.1f) -> Filtered(%.1f,%.1f) Vel(%.1f,%.1f) Error=%.1f Alpha=%.2f\r\n",
                          (float)latest_red_laser_coord.x, (float)latest_red_laser_coord.y,
                          filtered_x, filtered_y, velocity_x, velocity_y, error_distance, filter_alpha);
                last_filter_print = current_time;
            }
        }
        else
        {
            // Reset filter for other modes
            filter_initialized = 0;
        }

        // Gentle startup mechanism - detect first few seconds after receiving data
        static uint32_t first_valid_time = 0;
        static uint8_t startup_phase = 1;

        if (first_valid_time == 0)
        {
            first_valid_time = current_time;
            startup_phase = 1;
        }

        uint32_t time_since_start = current_time - first_valid_time;
        if (time_since_start > 1000)
        { // 1秒后退出启动阶段，快速进入高速追踪模式
            startup_phase = 0;
        }

        // Adaptive parameters based on error distance
        float adaptive_smooth_factor, adaptive_max_speed;

        // Target reached detection with hysteresis - Mode 3 enhanced stability
        static uint8_t target_reached = 0;
        static uint32_t target_stable_time = 0;

        // Mode 3 specific thresholds for better stability on moving platform
        float stop_threshold, restart_threshold;
        uint32_t stable_time_ms;

        if (current_mode == SYSTEM_MODE_3)
        {
            // Mode 3: More relaxed thresholds for moving platform stability
            stop_threshold = 10.0f;    // 10像素内停止 (更宽松，适应平台运动)
            restart_threshold = 20.0f; // 20像素外重启 (更宽松)
            stable_time_ms = 500;      // 500ms稳定时间 (更长稳定时间)
        }
        else
        {
            // Other modes: Original precise thresholds
            stop_threshold = 6.0f;     // 6像素内停止 (更精确)
            restart_threshold = 12.0f; // 12像素外重启 (更快响应)
            stable_time_ms = 300;      // 300ms稳定时间 (更快锁定)
        }

        // Check if target is reached with hysteresis
        if (!target_reached && error_distance <= stop_threshold)
        {
            if (target_stable_time == 0)
            {
                target_stable_time = current_time;
            }
            else if (current_time - target_stable_time >= stable_time_ms)
            {
                target_reached = 1;
                if (current_mode == SYSTEM_MODE_3)
                {
                    my_printf(&huart1, "MODE3 TARGET REACHED - Motors stopped at error_dist=%.1f (stable platform tracking)\r\n", error_distance);
                }
                else
                {
                    my_printf(&huart1, "TARGET REACHED - Motors stopped at error_dist=%.1f\r\n", error_distance);
                }

                // 触发激光发射
                Laser_OnTargetLocked();
            }
        }
        else if (target_reached && error_distance > restart_threshold)
        {
            target_reached = 0;
            target_stable_time = 0;
            if (current_mode == SYSTEM_MODE_3)
            {
                my_printf(&huart1, "MODE3 TARGET LOST - Resuming tracking at error_dist=%.1f (platform movement detected)\r\n", error_distance);
            }
            else
            {
                my_printf(&huart1, "TARGET LOST - Resuming tracking at error_dist=%.1f\r\n", error_distance);
            }

            // 通知激光系统目标丢失
            Laser_OnTargetLost();
        }
        else if (error_distance > stop_threshold)
        {
            target_stable_time = 0; // Reset stability timer if moved away
        }

        // If target is reached, stop motors completely
        if (target_reached)
        {
            Step_Motor_Set_Speed_my(0, 0);

            // Print status every 2 seconds when stopped
            static uint32_t last_stop_print = 0;
            if (current_time - last_stop_print > 2000)
            {
                my_printf(&huart1, "STOPPED: origin(%d,%d) error_dist=%.1f - Target locked\r\n",
                          latest_red_laser_coord.x, latest_red_laser_coord.y, error_distance);
                last_stop_print = current_time;
            }
            return;
        }

        // 快速启动阶段 - 缩短启动时间，提高初始响应
        if (startup_phase)
        {
            // 快速启动 - 1秒内达到全速
            float startup_factor = (float)time_since_start / 1000.0f; // 0 to 1 over 1 second
            if (startup_factor > 1.0f)
                startup_factor = 1.0f;

            if (current_mode == SYSTEM_MODE_3)
            {
                // 模式3：更保守的启动参数，提高稳定性
                adaptive_smooth_factor = 0.2f + startup_factor * 0.3f; // 0.2 to 0.5 (更稳定)
                adaptive_max_speed = startup_factor * 15.0f;           // 0 to 15 RPM (更平稳启动)
                my_printf(&huart1, "MODE3_STARTUP: t=%.1fs factor=%.2f max_speed=%.1f (stable)\r\n",
                          time_since_start / 1000.0f, adaptive_smooth_factor, adaptive_max_speed);
            }
            else
            {
                // 其他模式：正常启动参数
                adaptive_smooth_factor = 0.3f + startup_factor * 0.5f; // 0.3 to 0.8 (快速响应)
                adaptive_max_speed = startup_factor * 20.0f;           // 0 to 20 RPM (更快启动)
                my_printf(&huart1, "FAST_STARTUP: t=%.1fs factor=%.2f max_speed=%.1f\r\n",
                          time_since_start / 1000.0f, adaptive_smooth_factor, adaptive_max_speed);
            }
        }
        else
        {
            // 根据模式和距离调整参数
            if (current_mode == SYSTEM_MODE_3)
            {
                // 模式3：稳定性优先的距离控制策略
                if (error_distance > 100.0f) // 超远距离 - 中等速度
                {
                    adaptive_smooth_factor = 0.5f; // 中等响应性
                    adaptive_max_speed = 18.0f;    // 适中速度
                }
                else if (error_distance > 60.0f) // 远距离 - 平稳接近
                {
                    adaptive_smooth_factor = 0.4f; // 平稳响应性
                    adaptive_max_speed = 15.0f;    // 平稳速度
                }
                else if (error_distance > 40.0f) // 中远距离 - 稳定接近
                {
                    adaptive_smooth_factor = 0.35f; // 稳定响应性
                    adaptive_max_speed = 12.0f;     // 稳定速度
                }
                else if (error_distance > 25.0f) // 中距离 - 精确控制
                {
                    adaptive_smooth_factor = 0.3f; // 精确响应性
                    adaptive_max_speed = 8.0f;     // 精确速度
                }
                else if (error_distance > 15.0f) // 中近距离 - 缓慢减速
                {
                    adaptive_smooth_factor = 0.25f; // 低响应性
                    adaptive_max_speed = 4.0f;      // 缓慢接近
                }
                else if (error_distance > 10.0f) // 近距离 - 精细控制
                {
                    adaptive_smooth_factor = 0.2f; // 精细响应性
                    adaptive_max_speed = 2.0f;     // 精细速度
                }
                else if (error_distance > 6.0f) // 很近距离 - 微调
                {
                    adaptive_smooth_factor = 0.15f; // 微调响应性
                    adaptive_max_speed = 1.0f;      // 微调速度
                }
                else // 极近距离 - 最精细调整
                {
                    adaptive_smooth_factor = 0.1f; // 最低响应性
                    adaptive_max_speed = 0.5f;     // 最精细速度
                }
            }
            else
            {
                // 其他模式：高速响应距离控制策略
                if (error_distance > 100.0f) // 超远距离 - 最高速度
                {
                    adaptive_smooth_factor = 0.8f; // 高响应性
                    adaptive_max_speed = 25.0f;    // 提高到25 RPM
                }
                else if (error_distance > 60.0f) // 远距离 - 高速接近
                {
                    adaptive_smooth_factor = 0.7f; // 高响应性
                    adaptive_max_speed = 20.0f;    // 提高到20 RPM
                }
                else if (error_distance > 40.0f) // 中远距离 - 快速接近
                {
                    adaptive_smooth_factor = 0.6f; // 较高响应性
                    adaptive_max_speed = 16.0f;    // 提高到16 RPM
                }
                else if (error_distance > 25.0f) // 中距离 - 平衡速度
                {
                    adaptive_smooth_factor = 0.5f; // 平衡响应性
                    adaptive_max_speed = 12.0f;    // 提高到12 RPM
                }
                else if (error_distance > 15.0f) // 中近距离 - 开始减速
                {
                    adaptive_smooth_factor = 0.4f; // 适中响应性
                    adaptive_max_speed = 5.0f;     // 减速接近
                }
                else if (error_distance > 10.0f) // 近距离 - 精确控制
                {
                    adaptive_smooth_factor = 0.3f; // 较低响应性
                    adaptive_max_speed = 3.0f;     // 低速精确
                }
                else if (error_distance > 6.0f) // 很近距离 - 精细调整
                {
                    adaptive_smooth_factor = 0.2f; // 低响应性
                    adaptive_max_speed = 1.5f;     // 精细速度
                }
                else // 极近距离 - 微调
                {
                    adaptive_smooth_factor = 0.15f; // 最低响应性
                    adaptive_max_speed = 0.8f;      // 微调速度
                }
            }
        }

        // 平衡的积分管理 - 保持响应性同时防止过冲
        if (error_distance < 15.0f) // 只在很近距离才管理积分
        {
            // 渐进式积分减少，保持响应性
            float integral_factor = error_distance / 15.0f; // 0 to 1
            if (integral_factor < 0.3f)
                integral_factor = 0.3f; // 最少保留30%积分

            pid_x.iout *= integral_factor; // 适度减少积分
            pid_y.iout *= integral_factor;
        }

        // 只在极近距离才大幅减少积分
        if (error_distance < 8.0f)
        {
            pid_x.iout *= 0.5f; // 保留50%积分
            pid_y.iout *= 0.5f;
        }

        // Calculate PID output
        float pos_out_x = pid_calc(&pid_x, latest_green_laser_coord.x, latest_red_laser_coord.x, 0);
        float pos_out_y = pid_calc(&pid_y, latest_green_laser_coord.y, latest_red_laser_coord.y, 0);

        // X轴和Y轴独立的平滑处理 - 根据模式调整
        static float smooth_x = 0.0f, smooth_y = 0.0f;
        static uint8_t smooth_filter_initialized = 0;

        // Initialize filter with first PID output to prevent sudden jumps
        if (!smooth_filter_initialized)
        {
            if (current_mode == SYSTEM_MODE_3)
            {
                // 模式3：更保守的初始化
                smooth_x = pos_out_x * 0.05f; // 更保守的X轴初始化
                smooth_y = pos_out_y * 0.05f; // 更保守的Y轴初始化
                my_printf(&huart1, "MODE3_FILTER_INIT: smooth_x=%.2f smooth_y=%.2f (conservative)\r\n", smooth_x, smooth_y);
            }
            else
            {
                // 其他模式：正常初始化
                smooth_x = pos_out_x * 0.1f; // X轴正常初始化
                smooth_y = pos_out_y * 0.1f; // Y轴正常初始化
                my_printf(&huart1, "FILTER_INIT: smooth_x=%.2f smooth_y=%.2f\r\n", smooth_x, smooth_y);
            }
            smooth_filter_initialized = 1;
        }

        // 根据模式调整平滑因子
        float smooth_factor_x, smooth_factor_y;
        if (current_mode == SYSTEM_MODE_3)
        {
            // 模式3：更强的平滑处理，提高稳定性
            smooth_factor_x = adaptive_smooth_factor * 0.7f; // X轴更保守
            smooth_factor_y = adaptive_smooth_factor * 0.8f; // Y轴稍保守
        }
        else
        {
            // 其他模式：正常平滑因子
            smooth_factor_x = adaptive_smooth_factor; // X轴正常
            smooth_factor_y = adaptive_smooth_factor; // Y轴正常
        }

        smooth_x = smooth_x * (1.0f - smooth_factor_x) + pos_out_x * smooth_factor_x;
        smooth_y = smooth_y * (1.0f - smooth_factor_y) + pos_out_y * smooth_factor_y;

        // X轴和Y轴独立的变化率限制 - X轴更严格
        static float last_smooth_x = 0.0f, last_smooth_y = 0.0f;

        if (error_distance < 15.0f) // 只在近距离限制变化率以保持响应性
        {
            // 简化的变化率限制，提高响应速度
            float max_change_x, max_change_y;

            if (error_distance < 6.0f)
            {
                max_change_x = 1.0f; // 允许较大变化率
                max_change_y = 1.0f; // 允许较大变化率
            }
            else
            {
                max_change_x = 3.0f; // 允许更大变化率
                max_change_y = 3.0f; // 允许更大变化率
            }

            float change_x = smooth_x - last_smooth_x;
            float change_y = smooth_y - last_smooth_y;

            // 独立应用变化率限制
            if (change_x > max_change_x)
                smooth_x = last_smooth_x + max_change_x;
            if (change_x < -max_change_x)
                smooth_x = last_smooth_x - max_change_x;
            if (change_y > max_change_y)
                smooth_y = last_smooth_y + max_change_y;
            if (change_y < -max_change_y)
                smooth_y = last_smooth_y - max_change_y;
        }

        // Update last values for next cycle
        last_smooth_x = smooth_x;
        last_smooth_y = smooth_y;

        // Apply adaptive speed limiting
        if (smooth_x > adaptive_max_speed)
            smooth_x = adaptive_max_speed;
        if (smooth_x < -adaptive_max_speed)
            smooth_x = -adaptive_max_speed;
        if (smooth_y > adaptive_max_speed)
            smooth_y = adaptive_max_speed;
        if (smooth_y < -adaptive_max_speed)
            smooth_y = -adaptive_max_speed;

        // Print detailed tracking info every 1 second
        static uint32_t last_pid_print = 0;
        if (current_time - last_pid_print > 1000)
        {
            my_printf(&huart1, "TRACKING: origin(%d,%d) error_dist=%.1f mode=%s raw_x=%.2f raw_y=%.2f out_x=%.2f out_y=%.2f\r\n",
                      latest_red_laser_coord.x, latest_red_laser_coord.y, error_distance,
                      (error_distance > 80) ? "VFAST" : (error_distance > 40) ? "FAST"
                                                    : (error_distance > 20)   ? "MED"
                                                    : (error_distance > 10)   ? "CLOSE"
                                                                              : "VCLOSE",
                      pos_out_x, pos_out_y, smooth_x, smooth_y);

            // X轴和Y轴独立的摆动检测和抑制
            if (error_distance < 35.0f) // Extended range for better detection
            {
                // Oscillation detection - track direction changes
                static float last_error_x = 0.0f, last_error_y = 0.0f;
                static uint8_t direction_changes_x = 0, direction_changes_y = 0;
                static uint32_t last_direction_check = 0;

                float current_error_x = latest_green_laser_coord.x - latest_red_laser_coord.x;
                float current_error_y = latest_green_laser_coord.y - latest_red_laser_coord.y;

                // X轴和Y轴独立的摆动检测阈值
                float oscillation_threshold_x, oscillation_threshold_y;
                if (error_distance < 5.0f)
                {
                    oscillation_threshold_x = 0.5f; // X轴更敏感
                    oscillation_threshold_y = 0.5f; // Y轴正常
                }
                else if (error_distance < 15.0f)
                {
                    oscillation_threshold_x = 1.0f; // X轴更敏感
                    oscillation_threshold_y = 1.0f; // Y轴正常
                }
                else if (error_distance < 25.0f)
                {
                    oscillation_threshold_x = 1.5f; // X轴更敏感
                    oscillation_threshold_y = 1.5f; // Y轴正常
                }
                else
                {
                    oscillation_threshold_x = 2.0f; // X轴更敏感
                    oscillation_threshold_y = 2.0f; // Y轴正常
                }

                // 独立检测X轴和Y轴的方向变化
                if ((current_error_x * last_error_x < 0) && (fabs(current_error_x) > oscillation_threshold_x))
                {
                    direction_changes_x++;
                }
                if ((current_error_y * last_error_y < 0) && (fabs(current_error_y) > oscillation_threshold_y))
                {
                    direction_changes_y++;
                }

                // Reset counters every 1.5 seconds for faster response
                if (current_time - last_direction_check > 1500)
                {
                    direction_changes_x = 0;
                    direction_changes_y = 0;
                    last_direction_check = current_time;
                }

                last_error_x = current_error_x;
                last_error_y = current_error_y;

                my_printf(&huart1, "ANTI_OSC: dist=%.1f I_x=%.3f I_y=%.3f osc_x=%d osc_y=%d thresh_x=%.1f thresh_y=%.1f\r\n",
                          error_distance, pid_x.iout, pid_y.iout, direction_changes_x, direction_changes_y,
                          oscillation_threshold_x, oscillation_threshold_y);

                // X轴和Y轴独立的摆动抑制
                if (direction_changes_x > 0)
                {
                    if (direction_changes_x > 3)
                    {
                        my_printf(&huart1, "X-AXIS SEVERE OSCILLATION - Emergency damping\r\n");
                        pid_x.iout *= 0.005f; // X轴更激进的抑制
                    }
                    else if (direction_changes_x > 2)
                    {
                        my_printf(&huart1, "X-AXIS MODERATE OSCILLATION - Strong damping\r\n");
                        pid_x.iout *= 0.02f; // X轴强力抑制
                    }
                    else if (direction_changes_x > 1)
                    {
                        my_printf(&huart1, "X-AXIS MILD OSCILLATION - Medium damping\r\n");
                        pid_x.iout *= 0.1f; // X轴中等抑制
                    }
                    else
                    {
                        my_printf(&huart1, "X-AXIS LIGHT OSCILLATION - Light damping\r\n");
                        pid_x.iout *= 0.5f; // X轴轻度抑制
                    }
                }

                if (direction_changes_y > 0)
                {
                    if (direction_changes_y > 3)
                    {
                        my_printf(&huart1, "Y-AXIS SEVERE OSCILLATION - Emergency damping\r\n");
                        pid_y.iout *= 0.005f; // Y轴正常抑制
                    }
                    else if (direction_changes_y > 2)
                    {
                        my_printf(&huart1, "Y-AXIS MODERATE OSCILLATION - Strong damping\r\n");
                        pid_y.iout *= 0.02f; // Y轴正常抑制
                    }
                    else if (direction_changes_y > 1)
                    {
                        my_printf(&huart1, "Y-AXIS MILD OSCILLATION - Medium damping\r\n");
                        pid_y.iout *= 0.1f; // Y轴正常抑制
                    }
                    else
                    {
                        my_printf(&huart1, "Y-AXIS LIGHT OSCILLATION - Light damping\r\n");
                        pid_y.iout *= 0.5f; // Y轴正常抑制
                    }
                }
            }

            last_pid_print = current_time;
        }

        // Control motors with adaptive output
        Step_Motor_Set_Speed_my(-smooth_x, smooth_y);
    }
    else
    {
        // No valid origin coordinate - gradually stop motors
        static float smooth_x = 0.0f, smooth_y = 0.0f;
        smooth_x *= 0.8f; // Gradual deceleration
        smooth_y *= 0.8f;

        if (fabs(smooth_x) < 0.1f)
            smooth_x = 0.0f;
        if (fabs(smooth_y) < 0.1f)
            smooth_y = 0.0f;

        Step_Motor_Set_Speed_my(smooth_x, smooth_y);
    }
}
