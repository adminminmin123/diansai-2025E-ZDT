# X轴抖动问题专项优化方案

## 🚨 **问题现象**
- Y轴电机运行正常，无抖动
- X轴电机在靠近目标点过程中发生抖动
- X轴和Y轴表现不一致

## 🔍 **问题分析**

### **可能原因**
1. **硬件差异**: X轴和Y轴电机可能有不同的机械特性
2. **信号处理**: X轴信号有负号处理 `Step_Motor_Set_Speed_my(-smooth_x, smooth_y)`
3. **控制敏感性**: X轴可能对控制参数更敏感
4. **机械负载**: X轴可能承受不同的机械负载

### **系统分析**
- 当前X轴和Y轴使用相同的PID参数
- 使用相同的平滑因子和变化率限制
- 摆动检测和抑制机制对两轴一视同仁

## ✅ **专项优化方案**

### **1. 非对称PID参数 (mypid.c)**
```c
// 修改前 - 对称参数
PID_struct_init(&pid_x, POSITION_PID, 8, 0.2, 0.005, 0.00002, 0.0002);
PID_struct_init(&pid_y, POSITION_PID, 8, 0.2, 0.005, 0.00002, 0.0002);
pid_x.input_deadband = 12;
pid_y.input_deadband = 12;

// 修改后 - X轴更保守
PID_struct_init(&pid_x, POSITION_PID, 6, 0.15, 0.003, 0.00001, 0.0001); // X轴更保守
PID_struct_init(&pid_y, POSITION_PID, 8, 0.2, 0.005, 0.00002, 0.0002);  // Y轴保持原参数
pid_x.input_deadband = 15; // X轴更大死区
pid_y.input_deadband = 12; // Y轴保持原死区
```

**X轴参数调整:**
- 最大输出: 8 → 6 RPM (降低25%)
- 积分限制: 0.2 → 0.15 (降低25%)
- 比例系数: 0.005 → 0.003 (降低40%)
- 积分系数: 0.00002 → 0.00001 (降低50%)
- 微分系数: 0.0002 → 0.0001 (降低50%)
- 死区: 12 → 15像素 (增加25%)

### **2. 独立平滑处理 (pi_bsp.c)**
```c
// X轴和Y轴使用不同的平滑因子
float smooth_factor_x = adaptive_smooth_factor * 0.6f; // X轴平滑因子减少40%
float smooth_factor_y = adaptive_smooth_factor;        // Y轴保持原平滑因子

smooth_x = smooth_x * (1.0f - smooth_factor_x) + pos_out_x * smooth_factor_x;
smooth_y = smooth_y * (1.0f - smooth_factor_y) + pos_out_y * smooth_factor_y;

// X轴更保守的初始化
smooth_x = pos_out_x * 0.05f; // X轴更保守的初始化
smooth_y = pos_out_y * 0.1f;  // Y轴正常初始化
```

### **3. 独立变化率限制**
```c
// X轴和Y轴独立的变化率限制
float max_change_x, max_change_y;

if (error_distance < 3.0f) {
    max_change_x = 0.005f; // X轴更严格
    max_change_y = 0.01f;  // Y轴正常
}
else if (error_distance < 6.0f) {
    max_change_x = 0.01f;  // X轴更严格
    max_change_y = 0.02f;  // Y轴正常
}
// ... 其他距离段
```

**X轴变化率限制比Y轴严格50%:**
- 3px内: X轴0.005 vs Y轴0.01
- 6px内: X轴0.01 vs Y轴0.02
- 10px内: X轴0.015 vs Y轴0.03
- 15px内: X轴0.025 vs Y轴0.05

### **4. 独立摆动检测**
```c
// X轴和Y轴独立的摆动检测阈值
float oscillation_threshold_x, oscillation_threshold_y;

if (error_distance < 5.0f) {
    oscillation_threshold_x = 0.3f; // X轴更敏感
    oscillation_threshold_y = 0.5f; // Y轴正常
}
else if (error_distance < 15.0f) {
    oscillation_threshold_x = 0.8f; // X轴更敏感
    oscillation_threshold_y = 1.0f; // Y轴正常
}
// ... 其他距离段
```

**X轴检测阈值比Y轴敏感40%:**
- 5px内: X轴0.3 vs Y轴0.5
- 15px内: X轴0.8 vs Y轴1.0
- 25px内: X轴1.2 vs Y轴1.5

### **5. 独立摆动抑制**
```c
// X轴独立的摆动抑制
if (direction_changes_x > 0) {
    if (direction_changes_x > 3) {
        pid_x.iout *= 0.001f; // X轴更激进的抑制
    }
    else if (direction_changes_x > 2) {
        pid_x.iout *= 0.01f;  // X轴强力抑制
    }
    // ... 其他级别
}

// Y轴独立的摆动抑制
if (direction_changes_y > 0) {
    if (direction_changes_y > 3) {
        pid_y.iout *= 0.005f; // Y轴正常抑制
    }
    else if (direction_changes_y > 2) {
        pid_y.iout *= 0.02f;  // Y轴正常抑制
    }
    // ... 其他级别
}
```

**X轴抑制比Y轴更激进:**
- 严重摆动: X轴0.001 vs Y轴0.005
- 中度摆动: X轴0.01 vs Y轴0.02
- 轻度摆动: X轴0.05 vs Y轴0.1

## 📊 **优化效果对比**

### **PID参数对比**

| 参数 | Y轴(正常) | X轴(优化前) | X轴(优化后) | 改进幅度 |
|------|-----------|-------------|-------------|----------|
| **最大输出** | 8 RPM | 8 RPM | 6 RPM | 降低25% |
| **比例系数** | 0.005 | 0.005 | 0.003 | 降低40% |
| **积分系数** | 0.00002 | 0.00002 | 0.00001 | 降低50% |
| **微分系数** | 0.0002 | 0.0002 | 0.0001 | 降低50% |
| **死区** | 12px | 12px | 15px | 增加25% |

### **控制策略对比**

| 控制方面 | Y轴策略 | X轴策略 | 差异 |
|----------|---------|---------|------|
| **平滑因子** | 正常 | 减少40% | 更平滑 |
| **变化率限制** | 正常 | 严格50% | 更稳定 |
| **摆动检测** | 正常阈值 | 敏感40% | 更早发现 |
| **摆动抑制** | 正常强度 | 激进5-10倍 | 更强抑制 |

## 🧪 **测试验证要点**

### **1. X轴抖动改善测试**
- 观察X轴在20-30px范围是否还有抖动
- 对比X轴和Y轴的运动平滑度
- 检查X轴是否能稳定锁定目标

### **2. 系统平衡性测试**
- 验证Y轴性能是否受影响
- 检查整体追踪精度是否保持
- 确认两轴协调性是否良好

### **3. 调试输出监控**
```
ANTI_OSC: dist=15.2 I_x=0.008 I_y=0.015 osc_x=0 osc_y=0 thresh_x=0.8 thresh_y=1.0
X-AXIS MILD OSCILLATION - Medium damping
```

**关键指标:**
- `osc_x` vs `osc_y`: X轴摆动计数应更低
- `thresh_x` vs `thresh_y`: X轴阈值应更敏感
- `I_x` vs `I_y`: X轴积分应更快衰减

## ⚙️ **进一步微调选项**

### **如果X轴仍有轻微抖动**
```c
// 进一步降低X轴参数
PID_struct_init(&pid_x, POSITION_PID, 4, 0.1, 0.002, 0.000005, 0.00005);
pid_x.input_deadband = 20; // 进一步增大死区

// 更严格的变化率限制
max_change_x = 0.003f; // 在3px内进一步降低
```

### **如果X轴响应过慢**
```c
// 适当提高X轴远距离参数
if (error_distance > 50.0f) {
    // X轴在远距离时可以稍微激进一些
    smooth_factor_x = adaptive_smooth_factor * 0.8f; // 从0.6提高到0.8
}
```

### **如果Y轴受到影响**
```c
// 确保Y轴参数不变
PID_struct_init(&pid_y, POSITION_PID, 10, 0.25, 0.006, 0.00003, 0.0003);
// 可以适当提高Y轴参数来补偿
```

## 📋 **优化总结**

### **核心改进**
1. **✅ 非对称PID参数** - X轴全面降低25-50%
2. **✅ 独立平滑处理** - X轴平滑因子减少40%
3. **✅ 独立变化率限制** - X轴限制严格50%
4. **✅ 独立摆动检测** - X轴检测敏感40%
5. **✅ 独立摆动抑制** - X轴抑制激进5-10倍

### **预期效果**
- **✅ X轴抖动消除** - 专门针对X轴的保守控制
- **✅ Y轴性能保持** - Y轴参数不变，保持原有性能
- **✅ 系统平衡** - 两轴独立优化，协调工作
- **✅ 精确追踪** - 整体追踪精度和稳定性提升

**现在X轴应该能够实现与Y轴一样的平滑稳定运行！**
