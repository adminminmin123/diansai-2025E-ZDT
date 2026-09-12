#include "uart_bsp.h"

struct rt_ringbuffer ringbuffer_x;
struct rt_ringbuffer ringbuffer_y;
struct rt_ringbuffer ringbuffer_pi;

uint8_t ringbuffer_pool_x[64];
uint8_t ringbuffer_pool_y[64];
uint8_t ringbuffer_pool_pi[64];

uint8_t output_buffer_x[64];
uint8_t output_buffer_y[64];
uint8_t output_buffer_pi[64];

static char line_buffer[128];   // Line buffer for camera data parsing
static int line_buffer_idx = 0; // Current write position in line buffer

int my_printf(UART_HandleTypeDef *huart, const char *format, ...)
{
    char buffer[512];
    int retval;
    va_list local_argv;

    va_start(local_argv, format);
    retval = vsnprintf(buffer, sizeof(buffer), format, local_argv);
    va_end(local_argv);

    HAL_UART_Transmit(huart, (uint8_t *)buffer, retval, HAL_MAX_DELAY);
    return retval;
}

// ??????????›¥XY??????
float x_motor_angle = 0.0f;
float y_motor_angle = 0.0f;
// ???????????¦¶???
uint8_t x_angle_limit_flag = 0;
uint8_t y_angle_limit_flag = 0;

// ??????????????????????????????????
uint8_t motor_angle_limit_check_enabled = 0;

// ?¦Ï?¦Ë??????????????
uint32_t x_reference_position = 0;
uint32_t y_reference_position = 0;
uint8_t x_reference_initialized = 0;
uint8_t y_reference_initialized = 0;
float x_relative_angle = 0.0f;
float y_relative_angle = 0.0f;

// ?????¦Ë??›¥
uint32_t x_initial_position = 0;
uint32_t y_initial_position = 0;
uint8_t x_initial_direction = 0;
uint8_t y_initial_direction = 0;
uint8_t initial_position_saved = 0;

// ????????üD??
void check_motor_angle_limits(void)
{
    // ?????????ðÐ??????????
    if (!motor_angle_limit_check_enabled)
        return;

    // ???X?????????????????
    if (x_relative_angle > MOTOR_MAX_ANGLE || x_relative_angle < -MOTOR_MAX_ANGLE)
    {
        if (x_angle_limit_flag == 0)
        {
            my_printf(&huart1, "X????????????????(??%d??)???????!\r\n", MOTOR_MAX_ANGLE);
            x_angle_limit_flag = 1;
            // ?????
            Emm_V5_Stop_Now(&MOTOR_X_UART, MOTOR_X_ADDR, MOTOR_SYNC_FLAG);
        }
    }
    else
    {
        x_angle_limit_flag = 0;
    }

    // ???Y?????????????????
    if (y_relative_angle > MOTOR_MAX_ANGLE || y_relative_angle < -MOTOR_MAX_ANGLE)
    {
        if (y_angle_limit_flag == 0)
        {
            my_printf(&huart1, "Y????????????????(??%d??)???????!\r\n", MOTOR_MAX_ANGLE);
            y_angle_limit_flag = 1;
            // ?????
            Emm_V5_Stop_Now(&MOTOR_Y_UART, MOTOR_Y_ADDR, MOTOR_SYNC_FLAG);
        }
    }
    else
    {
        y_angle_limit_flag = 0;
    }
}

// ?????¦Ë??????????
float calc_motor_angle(uint8_t dir, uint32_t position)
{
    float angle;
    // ???¦Ë?????0-65535??¦¶??
    position = position % 65536;

    // ???????
    angle = ((float)position * 360.0f) / 65536.0f;

    // ???????????????
    if (dir)
    {
        angle = -angle;
    }

    return angle;
}

// ?????????????
float calc_relative_angle(uint8_t dir, uint32_t current_position, uint32_t reference_position)
{
    // ???¦Ë?????0-65535??¦¶??
    current_position = current_position % 65536;
    reference_position = reference_position % 65536;

    // ???????¦Ë?¨°?
    int32_t relative_position;
    if (current_position >= reference_position)
    {
        relative_position = current_position - reference_position;
    }
    else
    {
        // ????????????
        relative_position = 65536 - reference_position + current_position;
    }

    // ??????¦Ë?????????????????????????
    if (relative_position > 32768)
    {
        relative_position = relative_position - 65536;
    }

    // ?????????
    float angle = ((float)relative_position * 360.0f) / 65536.0f;

    // ???????????????
    if (dir)
    {
        angle = -angle;
    }

    return angle;
}

// X???????????????
void parse_x_motor_data(Emm_V5_Response_t *resp)
{
    // ???????????????????????
    switch (resp->func)
    {
    case 0x35: // ????????
        my_printf(&huart1, "X Motor Addr:%d Speed:%d RPM\r\n", resp->addr, resp->speed);
        break;

    case 0x36: // ?????¦Ë??
        // ????X??????????
        x_motor_angle = calc_motor_angle(resp->dir, resp->position);

        // ??????¦Ï?¦Ë???????????
        if (!x_reference_initialized)
        {
            x_reference_position = resp->position;
            x_reference_initialized = 1;
            x_relative_angle = 0.0f;
            my_printf(&huart1, "X?????¦Ï?¦Ë????????: %ld ????\r\n", x_reference_position);
        }
        else
        {
            // ?????????
            x_relative_angle = calc_relative_angle(resp->dir, resp->position, x_reference_position);
        }

        // ??????¦Ë?????
        if (!initial_position_saved && y_reference_initialized)
        {
            x_initial_position = resp->position;
            x_initial_direction = resp->dir;
            initial_position_saved = 1;
            my_printf(&huart1, "???¦Ë???????: X=%ld Y=%ld\r\n", x_initial_position, y_initial_position);
        }

        my_printf(&huart1, "X Motor Addr:%d Pos:%ld Dir:%d Angle:%.2f RelAngle:%.2f\r\n",
                  resp->addr, resp->position, resp->dir, x_motor_angle, x_relative_angle);
        break;

    case 0x1F: // ???????·Ú
        my_printf(&huart1, "X Motor Addr:%d Version:%s\r\n", resp->addr, resp->version);
        break;

    case 0x24: // ?????????
        my_printf(&huart1, "X Motor Addr:%d Voltage:%d V\r\n", resp->addr, resp->voltage);
        break;

    case 0x27: // ????????
        my_printf(&huart1, "X Motor Addr:%d Current:%d mA\r\n", resp->addr, resp->current);
        break;

    case 0x33: // ????????
        my_printf(&huart1, "X???????:%d ???:0x%02X\r\n", resp->addr, resp->status);
        // ??????????????????????
        if (resp->status & 0x01)
            my_printf(&huart1, "  X?????????\r\n");
        if (resp->status & 0x02)
            my_printf(&huart1, "  X???????¦Ë\r\n");
        if (resp->status & 0x04)
            my_printf(&huart1, "  X???????????\r\n");
        break;

    case 0x3B: // ?????????
        my_printf(&huart1, "X???????:%d ??????:0x%02X\r\n", resp->addr, resp->origin_state);
        // ???????????????????
        if (resp->origin_state == 0)
            my_printf(&huart1, "  X??¦Ä?????????\r\n");
        else if (resp->origin_state == 1)
            my_printf(&huart1, "  X?????????\r\n");
        else if (resp->origin_state == 2)
            my_printf(&huart1, "  X????????\r\n");
        else if (resp->origin_state == 3)
            my_printf(&huart1, "  X????????\r\n");
        break;

    default:
        // ??????????????????resp?§Ö????????????????????
        my_printf(&huart1, "X Motor Addr:%d Func:0x%02X Unknown\r\n", resp->addr, resp->func);
        break;
    }
}

// Y???????????????
void parse_y_motor_data(Emm_V5_Response_t *resp)
{
    // ???????????????????????
    switch (resp->func)
    {
    case 0x35: // ????????
        my_printf(&huart1, "Y Motor Addr:%d Speed:%d RPM\r\n", resp->addr, resp->speed);
        break;

    case 0x36: // ?????¦Ë??
        // ????Y??????????
        y_motor_angle = calc_motor_angle(resp->dir, resp->position);

        // ??????¦Ï?¦Ë???????????
        if (!y_reference_initialized)
        {
            y_reference_position = resp->position;
            y_reference_initialized = 1;
            y_relative_angle = 0.0f;
            my_printf(&huart1, "Y?????¦Ï?¦Ë????????: %ld ????\r\n", y_reference_position);
        }
        else
        {
            // ?????????
            y_relative_angle = calc_relative_angle(resp->dir, resp->position, y_reference_position);
        }

        // ??????¦Ë?????
        if (!initial_position_saved && x_reference_initialized)
        {
            y_initial_position = resp->position;
            y_initial_direction = resp->dir;
            initial_position_saved = 1;
            my_printf(&huart1, "???¦Ë???????: X=%ld Y=%ld\r\n", x_initial_position, y_initial_position);
        }

        my_printf(&huart1, "Y Motor Addr:%d Pos:%ld Dir:%d Angle:%.2f RelAngle:%.2f\r\n",
                  resp->addr, resp->position, resp->dir, y_motor_angle, y_relative_angle);
        break;

    case 0x1F: // ???????·Ú
        my_printf(&huart1, "Y Motor Addr:%d Version:%s\r\n", resp->addr, resp->version);
        break;

    case 0x24: // ?????????
        my_printf(&huart1, "Y Motor Addr:%d Voltage:%d V\r\n", resp->addr, resp->voltage);
        break;

    case 0x27: // ????????
        my_printf(&huart1, "Y Motor Addr:%d Current:%d mA\r\n", resp->addr, resp->current);
        break;

    case 0x33: // ????????
        my_printf(&huart1, "Y???????:%d ???:0x%02X\r\n", resp->addr, resp->status);
        // ??????????????????????
        if (resp->status & 0x01)
            my_printf(&huart1, "  Y?????????\r\n");
        if (resp->status & 0x02)
            my_printf(&huart1, "  Y???????¦Ë\r\n");
        if (resp->status & 0x04)
            my_printf(&huart1, "  Y???????????\r\n");
        break;

    case 0x3B: // ?????????
        my_printf(&huart1, "Y???????:%d ??????:0x%02X\r\n", resp->addr, resp->origin_state);
        // ???????????????????
        if (resp->origin_state == 0)
            my_printf(&huart1, "  Y??¦Ä?????????\r\n");
        else if (resp->origin_state == 1)
            my_printf(&huart1, "  Y?????????\r\n");
        else if (resp->origin_state == 2)
            my_printf(&huart1, "  Y????????\r\n");
        else if (resp->origin_state == 3)
            my_printf(&huart1, "  Y????????\r\n");
        break;

    default:
        // ??????????????????resp?§Ö????????????????????
        my_printf(&huart1, "Y Motor Addr:%d Func:0x%02X Unknown\r\n", resp->addr, resp->func);
        break;
    }
}

// ??????¦Ë?????????????¦Ë??
void process_reset_command(void)
{
    // ??§Ö????¦Ë?????????????
    if (initial_position_saved)
    {
        my_printf(&huart1, "?????¦Ë????????¦Ë??...%d\r\n", x_initial_direction); // My:%d x_initial_direction(??????????bug)
        // ??????¦Ë??????????¦Ë??
        // X????
        Emm_V5_Pos_Control(&MOTOR_X_UART, MOTOR_X_ADDR, x_initial_direction,
                           MOTOR_MAX_SPEED / 2, MOTOR_ACCEL, x_initial_position,
                           true, MOTOR_SYNC_FLAG);

        // Y????
        Emm_V5_Pos_Control(&MOTOR_Y_UART, MOTOR_Y_ADDR, y_initial_direction,
                           MOTOR_MAX_SPEED / 2, MOTOR_ACCEL, y_initial_position,
                           true, MOTOR_SYNC_FLAG);
    }
    else
    {
        my_printf(&huart1, "????¦Ä??????¦Ë????????¦Ë\r\n");
    }
}

// ??????¦Ë?????
void save_initial_position(void)
{
    // ??????????¦Ë?¨°?????????¦Ë??
    if (!initial_position_saved)
    {
        // ???X??¦Ë??
        Emm_V5_Read_Sys_Params(&MOTOR_X_UART, MOTOR_X_ADDR, S_CPOS);
        // ???Y??¦Ë??
        Emm_V5_Read_Sys_Params(&MOTOR_Y_UART, MOTOR_Y_ADDR, S_CPOS);

        // ???¦Ë???????????????§Ø???????parse_x_motor_data??parse_y_motor_data????
        // ???¦Ë???????????§¹¦Ë?????????????????
        my_printf(&huart1, "?????????¦Ë??...\r\n");
    }
}

// Process debug commands
void process_command(const char *cmd, uint16_t len)
{
    // Handle reset command
    if (strncmp(cmd, "reset", 5) == 0)
    {
        // Reset system
        process_reset_command();
    }
    // Handle set(x,y) command - manual coordinate setting
    else if (strncmp(cmd, "set(", 6) == 0)
    {
        int target_x, target_y;
        // Parse coordinates
        if (sscanf(cmd, "set(%d,%d)", &target_x, &target_y) == 2)
        {
            // Set PID target manually
        }
        else
        {
            my_printf(&huart1, "Invalid format, use: set(x,y)\r\n");
        }
    }
}

void uart_proc(void)
{
    uint16_t length_x, length_y, length_pi;
    Emm_V5_Response_t resp_x, resp_y; // X and Y motor response data

    // Process X motor data
    length_x = rt_ringbuffer_data_len(&ringbuffer_x);
    if (length_x > 0)
    {
        rt_ringbuffer_get(&ringbuffer_x, output_buffer_x, length_x);
        output_buffer_x[length_x] = '\0';

        // Parse X motor response
        if (Emm_V5_Parse_Response(output_buffer_x, length_x, &resp_x))
        {
            parse_x_motor_data(&resp_x);
            // Reduce print frequency
            static uint32_t last_x_print = 0;
            uint32_t current_time = HAL_GetTick();
            if (current_time - last_x_print > 1000) // Print every 1 second
            {
                my_printf(&huart1, "X Motor Addr:%d Func:0x%02X Received\r\n", resp_x.addr, resp_x.func);
                last_x_print = current_time;
            }
        }
        else
        {
            my_printf(&huart1, "X Motor data parse failed!\r\n");
        }
        memset(output_buffer_x, 0, length_x);
    }

    // Process Y motor data
    length_y = rt_ringbuffer_data_len(&ringbuffer_y);
    if (length_y > 0)
    {
        rt_ringbuffer_get(&ringbuffer_y, output_buffer_y, length_y);
        output_buffer_y[length_y] = '\0';

        // Parse Y motor response
        if (Emm_V5_Parse_Response(output_buffer_y, length_y, &resp_y))
        {
            parse_y_motor_data(&resp_y);
            // Reduce print frequency
            static uint32_t last_y_print = 0;
            uint32_t current_time = HAL_GetTick();
            if (current_time - last_y_print > 1000) // Print every 1 second
            {
                my_printf(&huart1, "Y Motor Addr:%d Func:0x%02X Received\r\n", resp_y.addr, resp_y.func);
                last_y_print = current_time;
            }
        }
        else
        {
            my_printf(&huart1, "Y Motor data parse failed!\r\n");
        }
        memset(output_buffer_y, 0, length_y);
    }

    // ??????????????????????
    //	check_motor_angle_limits();

    // Simplified camera data processing
    length_pi = rt_ringbuffer_data_len(&ringbuffer_pi);

    if (length_pi > 0)
    {
        rt_ringbuffer_get(&ringbuffer_pi, output_buffer_pi, length_pi);

        // Process each byte to build complete lines
        for (int i = 0; i < length_pi; i++)
        {
            char current_char = output_buffer_pi[i];

            // Add character to line buffer
            if (line_buffer_idx < sizeof(line_buffer) - 1)
            {
                line_buffer[line_buffer_idx++] = current_char;
            }
            else
            {
                line_buffer_idx = 0; // Reset on overflow
                continue;
            }

            // Check for line endings
            if (current_char == '\n' || current_char == '\r')
            {
                if (line_buffer_idx > 1)
                {
                    line_buffer[line_buffer_idx - 1] = '\0';

                    // Only parse origin coordinates - jiguang is fixed at (320,340)
                    char *origin_pos = strstr(line_buffer, "origin:");
                    if (origin_pos)
                    {
                        pi_parse_data(origin_pos);
                    }
                }
                line_buffer_idx = 0;
            }
        }
    }
}
