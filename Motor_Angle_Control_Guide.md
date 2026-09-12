# 步进电机角度控制功能说明

## 功能概述
本功能实现了通过按键控制步进电机二维云台X轴按指定角度旋转的功能。

## 按键配置
根据您提供的引脚配置，以下按键已配置用于角度控制：

| 按键引脚 | 按键名称 | 功能 | 旋转角度 |
|---------|---------|------|---------|
| PB8     | Left_45 | 左转45度 | -45° |
| PB6     | Left_90 | 左转90度 | -90° |
| PB4     | Left_135| 左转135度| -135°|
| PE0     | Left_180| 左转180度| -180°|
| PC3     | Right_45| 右转45度 | +45° |
| PC1     | Right_90| 右转90度 | +90° |
| PE2     | Right_135|右转135度| +135°|

## 技术参数
- **步进电机参数**: 200步/圈，16细分，减速比1:1
- **角度精度**: 每度约8.89个脉冲
- **最大旋转角度**: ±180度
- **防抖时间**: 200ms
- **控制速度**: 使用配置的最大速度(MOTOR_MAX_SPEED)

## 代码实现

### 1. 新增的头文件定义 (key_bsp.h)
```c
/* 步进电机角度控制按键处理函数 */
void key_motor_angle_control(void);
uint8_t read_angle_keys(void);

/* 角度控制相关定义 */
#define ANGLE_45   45
#define ANGLE_90   90
#define ANGLE_135  135
#define ANGLE_180  180

/* 按键值定义 */
#define KEY_LEFT_45    1
#define KEY_LEFT_90    2
#define KEY_LEFT_135   3
#define KEY_LEFT_180   4
#define KEY_RIGHT_45   5
#define KEY_RIGHT_90   6
#define KEY_RIGHT_135  7
```

### 2. 新增的电机控制函数 (step_motor_bsp.h/c)
```c
void Step_Motor_Rotate_X_Angle(int16_t angle); // X轴按角度旋转
```

### 3. 主要功能函数

#### 按键读取函数
- `read_angle_keys()`: 读取所有角度控制按键状态
- 返回对应的按键值(1-7)，无按键时返回0

#### 角度控制处理函数
- `key_motor_angle_control()`: 处理角度控制按键逻辑
- 包含防抖处理，防止重复触发
- 根据按键值计算对应的旋转角度

#### 电机旋转函数
- `Step_Motor_Rotate_X_Angle(int16_t angle)`: 控制X轴电机旋转指定角度
- 正值为右转(CW)，负值为左转(CCW)
- 自动计算所需脉冲数并发送控制命令

## 使用方法

1. **编译和烧录**: 将修改后的代码编译并烧录到STM32开发板
2. **按键操作**: 按下对应的按键，电机将按指定角度旋转
3. **调试信息**: 通过UART1可以看到按键和电机旋转的调试信息

## 调试信息输出
系统会通过UART1输出以下调试信息：
- 按键按下信息: "Angle Key Pressed: X, Rotating X-axis by Y degrees"
- 电机旋转信息: "X Motor: Rotating Y degrees, dir=Z, pulses=W"

## 注意事项

1. **防抖处理**: 系统设置了200ms的防抖时间，避免按键重复触发
2. **角度限制**: 旋转角度被限制在±180度范围内
3. **按键优先级**: 如果同时按下多个按键，系统会按照代码中的优先级处理
4. **电机初始化**: 确保在使用角度控制功能前，电机已正确初始化

## 扩展功能
如需添加更多角度或修改现有角度，可以：
1. 在key_bsp.h中添加新的角度定义和按键值
2. 在read_angle_keys()函数中添加新的按键检测
3. 在key_motor_angle_control()函数的switch语句中添加新的case

## 故障排除
1. **按键无响应**: 检查GPIO配置和按键连接
2. **电机不转动**: 检查电机初始化和UART通信
3. **角度不准确**: 检查步进电机参数设置和脉冲计算
