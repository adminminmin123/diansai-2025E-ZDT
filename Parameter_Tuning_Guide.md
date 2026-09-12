# 目标追踪系统参数调整指导手册

## 📊 **当前系统参数总览**

### **PID参数 (mypid.c)**
```c
PID_struct_init(&pid_x, POSITION_PID, 1.5, 0.2, 0.005, 0.00002, 0.0002);
pid_x.input_deadband = 12; // 死区12像素
```

### **距离控制参数 (pi_bsp.c)**
```c
STOP_THRESHOLD = 8.0f;     // 8像素内停止
RESTART_THRESHOLD = 18.0f; // 18像素外重启
STABLE_TIME_MS = 800;      // 稳定800ms
```

### **距离分段控制**
- >80px: 3.0 RPM, 0.3 factor
- 50-80px: 2.0 RPM, 0.25 factor  
- 30-50px: 1.2 RPM, 0.20 factor
- 20-30px: 0.8 RPM, 0.12 factor
- 15-20px: 0.5 RPM, 0.08 factor
- 10-15px: 0.3 RPM, 0.05 factor
- 6-10px: 0.15 RPM, 0.03 factor
- <6px: 0.08 RPM, 0.02 factor

## 🔧 **根据现象调整参数**

### **现象1: 响应太慢，移动缓慢**

**症状:**
- 目标出现后很久才开始移动
- 移动速度很慢，像蜗牛爬行
- 远距离接近耗时很长

**调整方案:**
```c
// 1. 提高PID参数 (mypid.c)
PID_struct_init(&pid_x, POSITION_PID, 8, 2, 0.03, 0.0002, 0.01);

// 2. 提高远距离速度 (pi_bsp.c)
if (error_distance > 80.0f) {
    adaptive_smooth_factor = 0.6f; // 从0.3提高到0.6
    adaptive_max_speed = 6.0f;     // 从3.0提高到6.0
}
else if (error_distance > 50.0f) {
    adaptive_smooth_factor = 0.5f; // 从0.25提高到0.5
    adaptive_max_speed = 4.0f;     // 从2.0提高到4.0
}

// 3. 减小死区
pid_x.input_deadband = 6; // 从12减小到6
```

### **现象2: 在20-30像素范围摆动**

**症状:**
- 接近目标时左右摆动
- 在20-30像素范围无法稳定
- 摆动幅度较大

**调整方案:**
```c
// 1. 降低20-30px范围参数 (pi_bsp.c)
else if (error_distance > 20.0f) {
    adaptive_smooth_factor = 0.06f; // 从0.12降低到0.06
    adaptive_max_speed = 0.4f;      // 从0.8降低到0.4
}

// 2. 增强积分衰减
if (error_distance < 30.0f) {
    pid_x.iout *= 0.5f; // 新增30px内积分衰减
    pid_y.iout *= 0.5f;
}

// 3. 降低PID微分项
PID_struct_init(&pid_x, POSITION_PID, 1.5, 0.2, 0.005, 0.00002, 0.0001);
//                                                                    ^^^^^^
//                                                            从0.0002降到0.0001
```

### **现象3: 接近目标后仍有轻微摆动**

**症状:**
- 能接近目标但无法完全静止
- 在目标附近有小幅度摆动
- 无法牢牢锁定

**调整方案:**
```c
// 1. 增大死区 (mypid.c)
pid_x.input_deadband = 15; // 从12增加到15

// 2. 降低近距离参数 (pi_bsp.c)
else if (error_distance > 10.0f) {
    adaptive_smooth_factor = 0.03f; // 从0.05降低到0.03
    adaptive_max_speed = 0.2f;      // 从0.3降低到0.2
}
else if (error_distance > 6.0f) {
    adaptive_smooth_factor = 0.02f; // 从0.03降低到0.02
    adaptive_max_speed = 0.1f;      // 从0.15降低到0.1
}

// 3. 更激进的积分清除
if (error_distance < 15.0f) {
    pid_x.iout *= 0.1f; // 更激进的积分清除
    pid_y.iout *= 0.1f;
}
```

### **现象4: 无法到达目标，停在中途**

**症状:**
- 移动到某个距离就停止了
- 无法进入目标区域
- 死区过大导致无法精确定位

**调整方案:**
```c
// 1. 减小死区 (mypid.c)
pid_x.input_deadband = 4; // 从12减小到4

// 2. 减小停止阈值 (pi_bsp.c)
const float STOP_THRESHOLD = 4.0f; // 从8.0减小到4.0

// 3. 提高近距离参数
else if (error_distance > 6.0f) {
    adaptive_smooth_factor = 0.08f; // 从0.03提高到0.08
    adaptive_max_speed = 0.4f;      // 从0.15提高到0.4
}
```

### **现象5: 过冲，超过目标后摆动**

**症状:**
- 快速接近目标但冲过头
- 在目标两侧来回摆动
- 制动不及时

**调整方案:**
```c
// 1. 提前制动 (pi_bsp.c)
else if (error_distance > 25.0f) { // 从20.0提前到25.0
    adaptive_smooth_factor = 0.08f; // 从0.12降低到0.08
    adaptive_max_speed = 0.6f;      // 从0.8降低到0.6
}

// 2. 增强积分管理
if (error_distance < 35.0f) { // 从25.0扩大到35.0
    float integral_factor = error_distance / 35.0f;
    if (integral_factor < 0.05f) integral_factor = 0.05f;
    pid_x.iout *= integral_factor;
    pid_y.iout *= integral_factor;
}

// 3. 降低PID比例项
PID_struct_init(&pid_x, POSITION_PID, 1.5, 0.2, 0.003, 0.00002, 0.0002);
//                                                  ^^^^^ 从0.005降到0.003
```

### **现象6: 启动时突然跳动**

**症状:**
- 系统启动时电机突然快速移动
- 启动不平滑
- 有冲击感

**调整方案:**
```c
// 1. 延长启动时间 (pi_bsp.c)
if (time_since_start > 5000) { // 从3000延长到5000ms
    startup_phase = 0;
}

// 2. 降低启动参数
adaptive_smooth_factor = 0.05f + startup_factor * 0.1f; // 从0.15降到0.1
adaptive_max_speed = startup_factor * 0.8f;             // 从1.0降到0.8

// 3. 预热滤波器
if (!filter_initialized) {
    smooth_x = pos_out_x * 0.05f; // 从0.1降到0.05
    smooth_y = pos_out_y * 0.05f;
}
```

## 📈 **参数调整优先级**

### **优先级1: 基础响应性**
1. PID比例系数 (Kp) - 影响响应速度
2. 最大输出 (max_out) - 影响最大速度
3. 远距离速度参数 - 影响接近速度

### **优先级2: 稳定性**
1. 死区大小 (input_deadband) - 影响最终精度
2. 近距离速度参数 - 影响摆动
3. 积分管理 - 影响超调

### **优先级3: 精细调整**
1. 平滑因子 - 影响响应平滑度
2. 微分系数 (Kd) - 影响震荡
3. 停止阈值 - 影响停止精度

## 🧪 **调试方法**

### **1. 观察调试输出**
```
TRACKING: origin(x,y) error_dist=XX.X mode=XXX raw_x=X.XX out_x=X.XX
```
- error_dist: 当前距离
- mode: 当前控制模式
- raw_x/out_x: PID输出和最终输出

### **2. 关键指标监控**
- 响应时间: 从目标出现到开始移动的时间
- 接近时间: 从开始移动到接近目标的时间
- 稳定时间: 从接近目标到完全稳定的时间
- 摆动次数: 在目标附近的摆动次数

### **3. 分阶段测试**
1. **远距离测试**: 50px以上的响应速度
2. **中距离测试**: 20-50px的制动性能
3. **近距离测试**: 10px以内的精确性
4. **稳定性测试**: 长时间锁定的稳定性

## 💡 **调整技巧**

1. **一次只调一个参数** - 便于观察效果
2. **小幅度调整** - 避免系统不稳定
3. **记录调整过程** - 便于回退和对比
4. **分距离段测试** - 确保各距离段都正常
5. **长时间观察** - 确保稳定性

根据您观察到的具体现象，选择对应的调整方案进行参数优化！
