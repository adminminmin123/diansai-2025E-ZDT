# 摄像头数据接收测试指南

## 修改总结

### 1. 已修复的问题
- ✅ 移除了电机测试代码
- ✅ 修复了所有中文乱码问题，改为英文输出
- ✅ 移除了模拟数据功能
- ✅ 减少了调试信息打印频率，避免刷屏

### 2. 当前系统状态
系统现在专注于接收和处理真实的摄像头数据，您应该看到以下输出：

```
=== 2D Gimbal Tracking System Started ===
UART1: Debug output
UART2: X Motor (Addr:1)
UART3: Camera data
UART4: Y Motor (Addr:2)
Motor Max Speed: 50 RPM
Waiting for camera data...
System ready for camera data reception test.
Camera data length: 0
Camera Status: Origin valid=0(0,0), Jiguang valid=0(0,0)
X Motor Addr:1 Func:0xF6 Unknown
Y Motor Addr:2 Func:0xF6 Unknown
```

## 摄像头数据接收测试

### 3. 硬件连接检查
确认以下连接：
- 摄像头 TX → STM32 PD9 (USART3_RX)
- 摄像头 RX → STM32 PD8 (USART3_TX)
- 摄像头 GND → STM32 GND

### 4. 数据格式要求
摄像头应发送以下格式的数据：
```
origin:(x,y)\r\n
jiguang:(x,y)\r\n
```

例如：
```
origin:(100,200)
jiguang:(320,240)
```

### 5. 测试方法

#### 方法1：使用串口助手测试
1. 打开串口助手
2. 连接到UART3 (波特率115200)
3. 发送测试数据：
   ```
   origin:(100,200)
   jiguang:(320,240)
   ```

#### 方法2：检查摄像头输出
1. 将摄像头TX连接到USB转串口模块
2. 使用串口助手查看摄像头实际发送的数据格式
3. 确认数据格式是否符合要求

### 6. 预期的调试输出

当摄像头数据正常接收时，您应该看到：

```
Received camera data: 15 bytes
Raw data: 6F 72 69 67 69 6E 3A 28 31 30 30 2C 32 30 30 29 0D 0A
Successfully parsed camera data: 'origin:(100,200)'
Parsed origin: X=100, Y=200

Received camera data: 17 bytes  
Raw data: 6A 69 67 75 61 6E 67 3A 28 33 32 30 2C 32 34 30 29 0D 0A
Successfully parsed camera data: 'jiguang:(320,240)'
Parsed jiguang: X=320, Y=240

Camera Status: Origin valid=1(100,200), Jiguang valid=1(320,240)
PID: origin(100,200) jiguang(320,240) out_x=-22.00 out_y=-4.00
Motor: X_rpm=22.0(dir=0,speed=220) Y_rpm=4.0(dir=0,speed=40)
```

### 7. 故障排除

如果看不到摄像头数据：

1. **检查"Camera data length: 0"**
   - 如果一直显示0，说明UART6没有接收到数据
   - 检查硬件连接和波特率设置

2. **检查原始数据输出**
   - 如果看到"Raw data:"但解析失败，检查数据格式
   - 确认数据以\r\n结尾

3. **检查摄像头程序**
   - 确认摄像头程序正在运行
   - 确认摄像头程序发送的数据格式正确

### 8. 成功标志

系统正常工作的标志：
- ✅ 能看到"Received camera data: X bytes"
- ✅ 能看到"Successfully parsed camera data"
- ✅ 能看到"Camera Status: Origin valid=1, Jiguang valid=1"
- ✅ 能看到PID输出和电机控制命令
- ✅ 电机开始根据坐标差值转动

## 下一步

一旦摄像头数据接收正常，系统就会：
1. 自动进行PID计算
2. 控制电机转动
3. 实现目标追踪功能
