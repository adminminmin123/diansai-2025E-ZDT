# 激光发射控制系统

## 🎯 **功能概述**

为高响应动态追踪系统添加了完整的激光发射控制功能，支持自动发射和手动控制两种模式。

## 🔧 **硬件连接**

### **激光控制**
- **Laser引脚**: PA8 (GPIO输出)
- **控制方式**: 继电器控制激光笔开断
- **继电器类型**: 低电平触发继电器模块
- **电平逻辑**: **低电平开启激光，高电平关闭激光**
- **硬件连接**: STM32 PA8 → 继电器IN端，继电器COM-NO控制激光笔电源

### **按键控制**
- **KEY1引脚**: PE9 (GPIO输入，上拉)
- **按键逻辑**: 按下为低电平，松开为高电平
- **防抖时间**: 50ms

## ⚙️ **系统架构**

### **文件结构**
```
bsp/
├── laser_bsp.h        # 激光控制头文件
├── laser_bsp.c        # 激光控制实现
├── schedule.c         # 调度系统(已添加激光任务)
└── pi_bsp.c          # 追踪系统(已添加激光触发)

Core/Src/
└── main.c            # 主程序(已添加激光初始化)
```

### **调度任务**
```c
{Laser_Process, 20, 0}  // 激光控制处理，20ms周期
```

## 🎮 **控制模式**

### **1. 自动模式 (默认)**
- **触发条件**: 摄像头成功追踪并锁定目标点
- **发射时间**: 1000ms (1秒)
- **工作流程**:
  1. 目标进入6像素范围并稳定300ms
  2. 系统判定为"TARGET REACHED"
  3. 自动触发激光发射1秒
  4. 1秒后自动关闭激光

### **2. 手动模式**
- **切换方式**: 在自动模式下按KEY1切换到手动模式
- **控制方式**: 按KEY1切换激光开/关状态
- **优先级**: 手动模式优先级高于自动模式

## 📊 **状态机设计**

### **激光状态**
```c
typedef enum {
    LASER_STATE_OFF = 0,        // 激光关闭
    LASER_STATE_ON = 1,         // 激光开启
    LASER_STATE_AUTO_FIRING = 2 // 自动发射中
} LaserState_t;
```

### **控制模式**
```c
typedef enum {
    LASER_MODE_MANUAL = 0,      // 手动模式
    LASER_MODE_AUTO = 1         // 自动模式
} LaserMode_t;
```

### **状态转换**
```
自动模式:
OFF → AUTO_FIRING (目标锁定) → OFF (1秒后)

手动模式:
OFF ⇄ ON (按KEY1切换)

模式切换:
AUTO → MANUAL (按KEY1)
MANUAL → AUTO (调用API)
```

## 🔄 **工作流程**

### **自动发射流程**
```
1. 系统启动 → 激光关闭，自动模式
2. 摄像头追踪目标
3. 目标进入6像素范围
4. 稳定300ms后触发"TARGET REACHED"
5. 调用Laser_OnTargetLocked()
6. 激光开启，状态变为AUTO_FIRING
7. 1秒后自动关闭激光
8. 目标丢失时调用Laser_OnTargetLost()
```

### **手动控制流程**
```
1. 自动模式下按KEY1 → 切换到手动模式并开启激光
2. 手动模式下按KEY1 → 切换激光开/关状态
3. 可通过API切换回自动模式
```

## 🛠️ **API接口**

### **初始化函数**
```c
void Laser_Init(void);                    // 激光系统初始化
```

### **控制函数**
```c
void Laser_SetMode(LaserMode_t mode);     // 设置控制模式
void Laser_ManualControl(uint8_t enable); // 手动控制开关
void Laser_AutoFire(void);               // 自动发射
void Laser_Stop(void);                   // 强制停止
```

### **回调函数**
```c
void Laser_OnTargetLocked(void);         // 目标锁定回调
void Laser_OnTargetLost(void);           // 目标丢失回调
```

### **状态查询**
```c
LaserState_t Laser_GetState(void);       // 获取激光状态
LaserMode_t Laser_GetMode(void);         // 获取控制模式
```

## 📋 **调试输出**

### **系统启动**
```
LASER: System initialized - Mode: AUTO, State: OFF
```

### **自动发射**
```
TARGET REACHED - Motors stopped at error_dist=5.8
LASER: Auto fire started - Duration: 1000ms
LASER: Auto fire completed - OFF
```

### **手动控制**
```
LASER: Switched to MANUAL mode - ON
LASER: Manual OFF
LASER: Manual ON
```

### **目标跟踪**
```
TARGET LOST - Resuming tracking at error_dist=13.2
```

## ⚡ **性能特点**

### **响应速度**
- **按键响应**: 20ms周期检测，50ms防抖
- **自动触发**: 目标锁定后立即触发
- **发射精度**: 1000ms ±20ms

### **安全特性**
- **防抖处理**: 避免按键误触发
- **状态保护**: 防止重复触发
- **模式隔离**: 手动模式优先级高
- **超时保护**: 自动发射1秒后强制关闭

## 🧪 **测试验证**

### **自动模式测试**
1. 启动系统，确认激光关闭
2. 摄像头追踪目标点
3. 目标锁定后观察激光是否自动开启
4. 1秒后确认激光自动关闭
5. 目标丢失后确认系统恢复追踪

### **手动模式测试**
1. 自动模式下按KEY1，确认切换到手动模式
2. 观察激光是否开启
3. 再次按KEY1，确认激光关闭
4. 重复按KEY1，确认激光开关切换正常

### **模式切换测试**
1. 验证自动→手动模式切换
2. 验证手动模式下的激光控制
3. 验证手动模式优先级

## 🔧 **配置参数**

### **时间参数**
```c
#define LASER_AUTO_FIRE_DURATION_MS  1000  // 自动发射持续时间
#define LASER_DEBOUNCE_TIME_MS       50    // 按键防抖时间
```

### **追踪参数**
```c
const float STOP_THRESHOLD = 6.0f;         // 锁定阈值
const uint32_t STABLE_TIME_MS = 300;       // 稳定时间
```

### **硬件宏定义**
```c
// 继电器模块低电平触发逻辑
#define LASER_ON()    HAL_GPIO_WritePin(Laser_GPIO_Port, Laser_Pin, GPIO_PIN_RESET)  // 低电平开启
#define LASER_OFF()   HAL_GPIO_WritePin(Laser_GPIO_Port, Laser_Pin, GPIO_PIN_SET)    // 高电平关闭
```

### **继电器模块说明**
- **触发方式**: 低电平触发
- **控制逻辑**: IN端接低电平时继电器吸合，COM-NO导通
- **安全设计**: 系统初始化时设置为高电平，确保激光关闭
- **电源要求**: DC 5V-12V供电，控制电流3.5mA

## 📈 **扩展功能**

### **可扩展特性**
1. **多段发射**: 支持多次短脉冲发射
2. **发射模式**: 连续/脉冲/自定义模式
3. **安全锁定**: 添加安全开关功能
4. **发射计数**: 记录发射次数和时间
5. **远程控制**: 通过串口命令控制

### **集成建议**
1. 与OLED显示集成，显示激光状态
2. 与数据记录集成，记录发射日志
3. 与安全系统集成，添加安全检查

## 🔧 **代码重构优化 (v2.0)**

### **修复的问题**
1. **✅ 初始化激光开启问题** - 强制GPIO高电平确保激光关闭
2. **✅ 按键处理重构** - 所有按键逻辑集中到key_bsp.c
3. **✅ 代码组织优化** - 职责分离，模块化设计

### **重构内容**
- **激光模块**: 专注激光控制逻辑，移除按键处理
- **按键模块**: 集中处理所有按键逻辑，包括激光控制
- **初始化优化**: 双重保险确保激光初始关闭状态

### **新的代码架构**
```
key_bsp.c          → 统一按键处理入口
├── key_proc()     → 按键扫描和分发
└── key_laser_control() → KEY1激光控制逻辑

laser_bsp.c        → 纯激光控制逻辑
├── Laser_Init()   → 强化初始化
├── Laser_Process() → 自动发射超时处理
└── Laser_*()      → 激光控制API
```

## ✅ **系统集成完成 (v2.0)**

激光发射控制系统已完全集成并优化，提供了：

- **✅ 安全初始化** - 双重保险确保上电激光关闭
- **✅ 自动发射功能** - 目标锁定时自动发射1秒
- **✅ 手动控制功能** - KEY1按键手动控制
- **✅ 双模式支持** - 自动/手动模式切换
- **✅ 代码重构优化** - 按键处理集中化
- **✅ 安全保护机制** - 防抖、超时、状态保护
- **✅ 完整调试输出** - 便于监控和调试

**系统现在具备完整的目标追踪和激光发射能力，代码结构更加清晰！**
