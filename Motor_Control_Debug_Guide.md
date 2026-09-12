# 步进电机角度控制调试指南

## 问题修复总结

### 🔧 已修复的问题

1. **按键冲突问题**
   - **问题**: 角度控制按键与激光控制按键(KEY1/KEY2)发生冲突
   - **原因**: KEY1(PE9)和Right_135(PE2)共享GPIOE端口，可能产生信号干扰
   - **修复**: 在`read_angle_keys()`中添加优先级检查，传统按键优先

2. **电机不转动问题**
   - **问题**: 按下角度按键后电机无响应
   - **可能原因**: 
     - 电机初始化问题
     - UART通信问题
     - 命令参数错误
   - **修复**: 增加详细调试信息和错误检查

### 🛠️ 代码修改详情

#### 1. 按键冲突修复 (key_bsp.c)
```c
uint8_t read_angle_keys(void)
{
    // 首先检查传统按键，避免冲突
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET ||
        HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
    {
        return 0; // 传统按键优先
    }
    // 然后检查角度控制按键...
}
```

#### 2. 增强调试信息 (key_bsp.c)
- 增加按键事件详细日志
- 增加防抖时间到500ms
- 增加按键状态周期性检查
- 增加错误处理和状态报告

#### 3. 电机控制增强 (step_motor_bsp.c)
- 增加输入参数验证
- 增加UART状态检查
- 增加脉冲计算验证
- 增加详细的执行流程日志

## 🔍 调试步骤

### 第一步：检查按键硬件
1. 使用万用表测试按键连接
2. 检查上拉电阻是否正常
3. 验证按键按下时GPIO电平变化

### 第二步：监控调试输出
通过UART1监控以下调试信息：

#### 按键检测调试信息
```
DEBUG: Angle key detected: X        // 每100次循环检查
=== ANGLE KEY EVENT ===            // 按键事件开始
Key down detected: X               // 检测到的按键值
Current time: XXXX, Last time: XXXX // 时间戳
Key: LEFT_45, Angle: -45 degrees   // 按键和角度信息
Calling Step_Motor_Rotate_X_Angle(-45)... // 调用电机函数
```

#### 电机控制调试信息
```
=== MOTOR CONTROL START ===
Input angle: -45 degrees
Motor config: Speed=50 RPM, Accel=0
Direction: CCW (Left), Angle: -45
Calculated pulses: 400
Motor address: 0x01, UART: UART2
Sending Emm_V5_Pos_Control command...
UART state: READY
Motor command completed
=== MOTOR CONTROL END ===
```

### 第三步：故障排除

#### 问题1：按键无响应
**症状**: 按下按键没有任何调试输出
**检查**:
1. GPIO配置是否正确
2. 按键硬件连接
3. 上拉电阻工作状态

**解决方案**:
```c
// 在key_proc()中添加基础GPIO测试
my_printf(&huart1, "GPIO Test: PB8=%d, PB6=%d, PB4=%d\r\n",
    HAL_GPIO_ReadPin(Left_45_GPIO_Port, Left_45_Pin),
    HAL_GPIO_ReadPin(Ledt_90_GPIO_Port, Ledt_90_Pin),
    HAL_GPIO_ReadPin(Left_135_GPIO_Port, Left_135_Pin));
```

#### 问题2：检测到按键但电机不转
**症状**: 有按键调试输出，但电机无动作
**检查**:
1. 电机初始化是否成功
2. UART2通信是否正常
3. 电机电源和连接

**解决方案**:
```c
// 测试电机基本通信
Emm_V5_Read_Sys_Params(&huart2, MOTOR_X_ADDR, S_VER);
```

#### 问题3：激光意外触发
**症状**: 按角度按键时激光被触发
**原因**: 按键冲突或GPIO干扰
**解决方案**: 已在代码中修复，传统按键优先级更高

## 🧪 测试程序

### 基础按键测试
```c
void test_all_keys(void)
{
    my_printf(&huart1, "\r\n=== KEY TEST START ===\r\n");
    
    // 测试所有角度按键
    my_printf(&huart1, "Left_45 (PB8): %d\r\n", 
        HAL_GPIO_ReadPin(Left_45_GPIO_Port, Left_45_Pin));
    my_printf(&huart1, "Left_90 (PB6): %d\r\n", 
        HAL_GPIO_ReadPin(Ledt_90_GPIO_Port, Ledt_90_Pin));
    my_printf(&huart1, "Left_135 (PB4): %d\r\n", 
        HAL_GPIO_ReadPin(Left_135_GPIO_Port, Left_135_Pin));
    my_printf(&huart1, "Left_180 (PE0): %d\r\n", 
        HAL_GPIO_ReadPin(Left_180_GPIO_Port, Left_180_Pin));
    my_printf(&huart1, "Right_45 (PC3): %d\r\n", 
        HAL_GPIO_ReadPin(Right_45_GPIO_Port, Right_45_Pin));
    my_printf(&huart1, "Right_90 (PC1): %d\r\n", 
        HAL_GPIO_ReadPin(Right_90_GPIO_Port, Right_90_Pin));
    my_printf(&huart1, "Right_135 (PE2): %d\r\n", 
        HAL_GPIO_ReadPin(Right_135_GPIO_Port, Right_135_Pin));
    
    my_printf(&huart1, "=== KEY TEST END ===\r\n\r\n");
}
```

### 电机通信测试
```c
void test_motor_communication(void)
{
    my_printf(&huart1, "\r\n=== MOTOR COMM TEST ===\r\n");
    
    // 测试电机使能
    Emm_V5_En_Control(&MOTOR_X_UART, MOTOR_X_ADDR, true, false);
    HAL_Delay(100);
    
    // 测试小角度旋转
    Step_Motor_Rotate_X_Angle(5);  // 5度测试
    HAL_Delay(1000);
    
    Step_Motor_Rotate_X_Angle(-5); // 反向5度
    
    my_printf(&huart1, "=== MOTOR COMM TEST END ===\r\n\r\n");
}
```

## 📋 检查清单

### 硬件检查
- [ ] 按键连接正确
- [ ] 上拉电阻工作正常
- [ ] 电机电源供应充足
- [ ] UART2连接正确

### 软件检查
- [ ] GPIO初始化正确
- [ ] 电机初始化成功
- [ ] 调度系统运行正常
- [ ] UART1调试输出正常

### 功能测试
- [ ] 单个按键响应正常
- [ ] 电机旋转方向正确
- [ ] 角度精度符合预期
- [ ] 无按键冲突
- [ ] 激光控制不受影响

## 🚀 优化建议

1. **增加位置反馈**: 使用编码器确认实际旋转角度
2. **改进防抖**: 使用硬件防抖或更智能的软件防抖
3. **错误恢复**: 添加通信错误自动重试机制
4. **用户界面**: 添加LED指示当前状态
5. **参数调整**: 根据实际测试调整速度和加速度参数

## 📞 技术支持

如果问题仍然存在，请提供：
1. 完整的UART1调试输出
2. 按键测试结果
3. 电机型号和参数
4. 硬件连接图
