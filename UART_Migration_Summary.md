# 摄像头通信从UART6迁移到UART3总结

## 修改概述

已成功将摄像头模块的通信接口从UART6迁移到UART3，所有相关代码和配置都已更新。

## 详细修改列表

### 1. **Core/Src/usart.c**
- ✅ **回调函数修改**：将`HAL_UARTEx_RxEventCallback`中的`USART6`改为`USART3`
- ✅ **DMA重启修改**：将`HAL_UARTEx_ReceiveToIdle_DMA(&huart6, ...)`改为`HAL_UARTEx_ReceiveToIdle_DMA(&huart3, ...)`
- ✅ **串口6初始化清理**：移除USART6_Init中的摄像头DMA启动代码

### 2. **Core/Src/main.c**
- ✅ **DMA启动修改**：将`HAL_UARTEx_ReceiveToIdle_DMA(&huart6, ...)`改为`HAL_UARTEx_ReceiveToIdle_DMA(&huart3, ...)`
- ✅ **启动信息更新**：调试输出中将"UART6: Camera data"改为"UART3: Camera data"
- ✅ **外部声明保持**：`pi_rx_buf`等缓冲区声明保持不变

### 3. **Core/Src/stm32f4xx_it.c**
- ✅ **中断处理清理**：移除USART6_IRQHandler中的摄像头相关DMA重启代码

### 4. **Camera_Test_Guide.md**
- ✅ **硬件连接更新**：
  - 原：摄像头 TX → STM32 PC7 (USART6_RX)
  - 新：摄像头 TX → STM32 PD9 (USART3_RX)
  - 原：摄像头 RX → STM32 PC6 (USART6_TX)
  - 新：摄像头 RX → STM32 PD8 (USART3_TX)
- ✅ **测试方法更新**：串口助手连接改为UART3
- ✅ **预期输出更新**：调试信息中UART6改为UART3

## 硬件连接变更

### 原连接 (UART6)
```
摄像头 TX  →  STM32 PC7 (USART6_RX)
摄像头 RX  →  STM32 PC6 (USART6_TX)
摄像头 GND →  STM32 GND
```

### 新连接 (UART3)
```
摄像头 TX  →  STM32 PD9 (USART3_RX)
摄像头 RX  →  STM32 PD8 (USART3_TX)
摄像头 GND →  STM32 GND
```

## 软件配置确认

### UART3配置
- ✅ **波特率**：115200
- ✅ **数据位**：8位
- ✅ **停止位**：1位
- ✅ **校验位**：无
- ✅ **DMA配置**：已配置DMA1_Stream1用于接收
- ✅ **中断配置**：USART3_IRQn已启用

### 数据流程
1. **摄像头发送数据** → UART3_RX (PD9)
2. **DMA1_Stream1接收** → `pi_rx_buf[64]`
3. **中断触发** → `HAL_UARTEx_RxEventCallback`
4. **数据存储** → `ringbuffer_pi`
5. **数据处理** → `uart_proc()` → `pi_parse_data()`

## 预期系统输出

修改后，您应该看到以下启动信息：

```
=== 2D Gimbal Tracking System Started ===
UART1: Debug output
UART2: X Motor (Addr:1)
UART3: Camera data
UART4: Y Motor (Addr:2)
Motor Max Speed: 50 RPM
Waiting for camera data...
UART DMA reception started for UART2, UART4, UART3
System ready for camera data reception test.
```

## 测试验证

### 1. 硬件测试
- 重新连接摄像头到UART3引脚 (PD8/PD9)
- 确认电源和地线连接正确

### 2. 软件测试
- 编译并烧录修改后的代码
- 观察串口1输出，确认UART3 DMA启动成功
- 使用串口助手向UART3发送测试数据验证接收

### 3. 功能测试
- 启动摄像头程序
- 观察是否能接收到摄像头数据
- 验证PID控制和电机响应

## 注意事项

1. **引脚复用**：确认PD8/PD9没有被其他功能占用
2. **DMA冲突**：UART3使用DMA1_Stream1，确认无冲突
3. **中断优先级**：USART3_IRQn优先级已设置为0
4. **缓冲区共享**：`pi_rx_buf`仍然用于摄像头数据接收

## 回滚方案

如果需要回滚到UART6，只需要：
1. 将所有`USART3`改回`USART6`
2. 将所有`&huart3`改回`&huart6`
3. 恢复硬件连接到PC6/PC7
4. 恢复相关的DMA和中断配置

所有修改都已完成，系统现在使用UART3与摄像头通信。
