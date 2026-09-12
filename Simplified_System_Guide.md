# Simplified 2D Gimbal Tracking System

## System Overview

This is a simplified 2D gimbal tracking system that uses:
- **UART1**: Debug output
- **UART2**: X-axis motor communication (Address: 1)
- **UART3**: Camera data input (simplified)
- **UART4**: Y-axis motor communication (Address: 2)

## Key Simplifications Made

### 1. **Fixed Jiguang Coordinate**
- Jiguang (laser crosshair) coordinate is now **fixed at (320, 340)**
- No longer parsed from camera data
- Reduces processing complexity and eliminates jiguang parsing errors

### 2. **Simplified Data Processing**
- Only parse `origin:(x,y)` coordinates from camera
- Removed complex multi-coordinate parsing logic
- Eliminated unnecessary debug output and hex dumps
- Streamlined line buffer processing

### 3. **Clean Code Structure**
- Fixed all Chinese comment encoding issues
- Replaced garbled comments with clear English
- Simplified function logic
- Removed redundant test modes and complex state machines

## Data Flow

```
Camera → UART3 → Ring Buffer → Line Parser → origin:(x,y) → PID Controller → Motors
                                                    ↓
                              Fixed jiguang:(320,340) ←
```

## Camera Data Format

**Input Expected:**
```
origin:(x,y)\r\n
```

**Example:**
```
origin:(301,202)\r\n
```

## System Behavior

### Normal Operation
1. **Camera sends origin coordinates** via UART3
2. **System uses fixed jiguang(320,340)** as reference
3. **PID calculates error**: `error_x = 320 - origin_x`, `error_y = 340 - origin_y`
4. **Motors adjust** to minimize error

### No Data Condition
- If no valid origin coordinate received, motors stop
- System waits for valid data
- Status printed every 3 seconds

## Debug Output

**System Startup:**
```
=== 2D Gimbal Tracking System Started ===
UART1: Debug output
UART2: X Motor (Addr:1)
UART3: Camera data
UART4: Y Motor (Addr:2)
Motor Max Speed: 50 RPM
Waiting for camera data...
UART DMA reception started for UART2, UART4, UART3
```

**Normal Operation:**
```
Origin: (301,202)
Status: Origin valid=1(301,202), Jiguang fixed=1(320,340)
PID: origin(301,202) -> jiguang(320,340) out_x=19.00 out_y=138.00
X Motor Addr:1 Func:0x06 Received
Y Motor Addr:2 Func:0x06 Received
```

## Hardware Connections

```
Camera Module:
  TX  → STM32 PD9 (USART3_RX)
  RX  → STM32 PD8 (USART3_TX)
  GND → STM32 GND

X-Axis Motor:
  Connected to UART2 (Address: 1)

Y-Axis Motor:
  Connected to UART4 (Address: 2)

Debug Output:
  UART1 → USB/Serial Converter
```

## Configuration

### UART Settings
- **Baud Rate**: 115200 for all UARTs
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None

### PID Parameters
- Configured in `mypid.h` and `step_motor_bsp.c`
- Tuned for smooth tracking response

### Motor Settings
- **Max Speed**: 50 RPM
- **X Motor**: Address 1 (UART2)
- **Y Motor**: Address 2 (UART4)

## Testing

### Manual Testing
Use a serial terminal to send test data to UART3:
```
origin:(100,200)
origin:(400,300)
origin:(320,340)  // Should result in zero error
```

### Expected Response
```
Origin: (100,200)
PID: origin(100,200) -> jiguang(320,340) out_x=220.00 out_y=140.00
```

## Troubleshooting

### No Camera Data
- Check UART3 connections (PD8/PD9)
- Verify camera is sending `origin:(x,y)\r\n` format
- Monitor debug output for parsing errors

### Motors Not Responding
- Check motor power and connections
- Verify motor addresses (1 for X, 2 for Y)
- Check UART2/UART4 communication

### Parsing Errors
- Ensure data format is exactly `origin:(x,y)\r\n`
- Check for proper line endings (\r\n)
- Verify coordinate values are integers

## Code Structure

### Key Files
- `bsp/pi_bsp.c`: Camera data processing and PID control
- `bsp/uart_bsp.c`: UART data handling and parsing
- `Core/Src/usart.c`: UART initialization and callbacks
- `Core/Src/stm32f4xx_it.c`: Interrupt handlers

### Main Functions
- `pi_parse_data()`: Parse origin coordinates only
- `pi_proc()`: Simplified PID control logic
- `uart_proc()`: Clean UART data processing
- `HAL_UARTEx_RxEventCallback()`: DMA reception handling

## Benefits of Simplification

1. **Reduced Complexity**: Eliminated complex parsing logic
2. **Fixed Reference**: No jiguang parsing errors
3. **Better Performance**: Faster processing with less overhead
4. **Easier Debugging**: Clear English comments and simplified flow
5. **More Reliable**: Fewer failure points in data processing

The system now focuses on the core functionality: tracking a target (origin) relative to a fixed reference point (jiguang) using PID control.
