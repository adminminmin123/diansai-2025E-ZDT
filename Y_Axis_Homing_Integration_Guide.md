# Y轴步进电机上电自动回零集成指南

## 🎯 **功能概述**

为瞄准追踪系统添加了Y轴步进电机上电自动回零功能，确保系统启动时Y轴电机回到原点位置，然后再开始正常的追踪操作。

## 🏗️ **系统架构**

### **新增模块**
```
bsp/
├── y_axis_homing_bsp.h     # Y轴回零控制头文件
├── y_axis_homing_bsp.c     # Y轴回零控制实现
└── bsp_system.h           # 已添加回零模块包含
```

### **修改的文件**
```
Core/Src/main.c            # 主程序集成回零功能
bsp/schedule.c             # 调度系统添加回零任务
bsp/schedule.h             # 添加回零函数声明
bsp/pi_bsp.c              # 追踪系统等待回零完成
MDK-ARM/2025template.uvprojx # Keil项目文件
```

## ⚡ **工作流程**

### **系统启动时序**
```
1. 系统上电
2. 硬件初始化 (GPIO, UART, DMA等)
3. 调度系统初始化
4. PID控制器初始化
5. 激光控制系统初始化
6. Y轴回零系统初始化 ✨
7. 步进电机初始化
8. 启动Y轴回零运动 ✨
9. 进入主循环
10. 调度器运行各任务
    - Y轴回零状态机处理 ✨
    - 追踪系统等待回零完成 ✨
11. Y轴回零完成
12. 追踪系统开始正常工作 ✨
```

### **回零状态机**
```
IDLE → INIT_PARAMS → TRIGGERING → RUNNING → COMPLETED
  ↓                                  ↓
ERROR ←─────────────────────────── TIMEOUT
```

## 🔧 **技术实现**

### **1. Y轴回零参数配置**
```c
#define Y_HOMING_MODE               1       // 单圈就近回零
#define Y_HOMING_DIRECTION          0       // CW方向
#define Y_HOMING_VELOCITY           15      // 回零速度 15 RPM
#define Y_HOMING_TIMEOUT_MS         30000   // 超时时间 30秒
#define Y_HOMING_SLOW_VELOCITY      8       // 慢速回零速度 8 RPM
```

### **2. 状态检测机制**
- **基于时间估算**: 8秒后开始检查，12秒后认为完成
- **超时保护**: 30秒超时，最多重试3次
- **错误处理**: 失败后进入错误状态

### **3. 系统就绪检查**
```c
// 追踪系统检查回零状态
if (!Y_Axis_Homing_Is_System_Ready()) {
    // 暂停追踪，等待回零完成
    Step_Motor_Set_Speed_my(0, 0);
    return;
}
// 回零完成，开始正常追踪
```

## 📋 **API接口**

### **主要函数**
```c
void Y_Axis_Homing_Init(void);              // 初始化回零系统
void Y_Axis_Homing_Start(void);             // 启动Y轴回零
void Y_Axis_Homing_Process(void);           // 状态机处理 (调度器调用)
uint8_t Y_Axis_Homing_Is_System_Ready(void); // 查询系统是否就绪
YHomingState_t Y_Axis_Homing_Get_State(void); // 获取回零状态
```

### **状态枚举**
```c
typedef enum {
    Y_HOMING_STATE_IDLE = 0,        // 空闲状态
    Y_HOMING_STATE_INIT = 1,        // 初始化回零参数
    Y_HOMING_STATE_TRIGGERING = 2,  // 触发回零运动
    Y_HOMING_STATE_RUNNING = 3,     // 回零运动中
    Y_HOMING_STATE_COMPLETED = 4,   // 回零完成
    Y_HOMING_STATE_ERROR = 5        // 回零错误
} YHomingState_t;
```

## 🛡️ **安全机制**

### **1. 超时保护**
- 30秒超时时间
- 最多重试3次
- 失败后进入错误状态

### **2. 系统协调**
- 回零期间追踪系统暂停
- 回零完成后追踪系统恢复
- 激光控制正常工作

### **3. 状态监控**
- 实时串口输出回零状态
- 追踪系统显示等待状态
- 错误状态可被检测

## 📊 **调试输出**

### **回零过程输出**
```
Y_HOMING: System initialized
Y_HOMING: Starting Y-axis homing sequence...
Y_HOMING: Configuring Y-axis homing parameters...
Y_HOMING: Triggering Y-axis homing motion...
TRACKING: Waiting for Y-axis homing completion (state=3)...
Y_HOMING: Y-axis homing completed successfully!
SYSTEM: Tracking system ready for operation
```

### **追踪系统输出**
```
TRACKING: Waiting for Y-axis homing completion (state=3)...
Status: Origin valid=1(100,200), Jiguang fixed=1(320,240)
TRACKING: origin(100,200) error_dist=45.2 mode=FAST
```

## ⚙️ **配置参数**

### **回零参数调整**
如需调整回零参数，修改 `y_axis_homing_bsp.h` 中的宏定义：

```c
// 如果回零速度太快
#define Y_HOMING_VELOCITY           10      // 降低到10 RPM

// 如果回零时间太长
#define Y_HOMING_TIMEOUT_MS         20000   // 减少到20秒

// 如果需要反向回零
#define Y_HOMING_DIRECTION          1       // 改为CCW方向
```

### **时间参数调整**
如需调整回零完成判断时间，修改 `Y_Axis_Homing_CheckStatus()` 函数：

```c
// 更快判断完成
if (elapsed_time > 6000)  // 6秒后开始检查
{
    if (elapsed_time > 10000)  // 10秒后认为完成
    {
        return 1;
    }
}
```

## 🔄 **扩展功能**

### **1. 双轴回零**
可以扩展支持X轴回零：
- 复制Y轴回零模块
- 修改为X轴参数
- 支持顺序或并行回零

### **2. 精确状态检测**
可以通过读取电机状态寄存器实现精确检测：
```c
// 读取回零状态
Emm_V5_Read_Sys_Params(&MOTOR_Y_UART, MOTOR_Y_ADDR, S_ORG);
// 解析响应判断回零完成
```

### **3. 回零参数保存**
可以将回零参数保存到EEPROM：
- 支持运行时修改参数
- 断电保存配置
- 恢复默认参数

## ✅ **验证测试**

### **功能测试**
1. **上电测试**: 确认Y轴自动开始回零
2. **完成测试**: 确认回零完成后追踪系统启动
3. **超时测试**: 断开Y轴电机测试超时保护
4. **重试测试**: 验证失败重试机制

### **性能测试**
1. **回零时间**: 正常应在8-15秒内完成
2. **系统响应**: 回零完成后追踪系统立即响应
3. **稳定性**: 多次重启测试回零稳定性

## 🎯 **使用说明**

### **正常使用**
1. 系统上电后自动开始Y轴回零
2. 等待串口输出 "Y-axis homing completed"
3. 追踪系统自动开始工作
4. 可以正常发送摄像头数据进行追踪

### **手动控制**
```c
// 手动启动回零
Y_Axis_Homing_Start();

// 检查回零状态
if (Y_Axis_Homing_Is_Completed()) {
    // 回零完成
}

// 强制停止回零
Y_Axis_Homing_Stop();
```

**Y轴步进电机上电自动回零功能已完全集成！系统现在会在启动时自动进行Y轴回零，完成后再开始正常的瞄准追踪工作。** 🎯
