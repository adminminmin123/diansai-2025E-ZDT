# 电赛 E 题 · 运动目标自动追踪系统（STM32F407 + MSPM0G3507 + K230）

> 全国大学生电子设计竞赛 E 题参赛作品，**省级一等奖**。双板系统：云台控制板（STM32F407VET6）负责目标追踪与激光打击，循迹小车板（MSPM0G3507）负责移动与联动，K230 视觉模块提供目标坐标。

## 系统架构

```
                K230 视觉模块
                     │ USART3 (目标坐标)
                     ▼
┌───────────────────────────────┐   USART2 ──► Emm_V5 闭环步进 (X 轴)
│        云台控制板               │   UART4  ──► Emm_V5 闭环步进 (Y 轴)
│     STM32F407VET6 @80MHz      │   USART1 ──► 调试串口
│  FreeRTOS + HAL + 双轴位置环    │   USART6 ◄─► 小车联动 (9600)
└───────────────────────────────┘
                     ▲
                     │ "TURN_START\r\n" 直角弯联动信号
┌───────────────────────────────┐
│          循迹小车板             │   5 路灰度循迹 + 双轮速度环 PID
│         MSPM0G3507             │   编码器计数 + 圈数控制
└───────────────────────────────┘
```

**联动逻辑**：小车巡线检测到直角弯时，通过 UART 发送 `TURN_START\r\n` 给云台，云台 X 轴立即转动 90°，完成协同动作。

## 云台控制板（本仓库根目录，Keil MDK 工程）

### FreeRTOS 多任务（2026-09 赛后重构：协作调度器 → RTOS）

| 任务 | 周期 | 优先级 | 职责 |
|------|------|--------|------|
| `uart_task` | 10 ms | 2 | 4 路串口协议解析与应答 |
| `pi_task` | 10 ms | 2 | K230 坐标 → 双轴 PID 解算 |
| `laser_task` | 10 ms | 2 | 激光三模式状态机 |
| `key_task` | 20 ms | 1 | 按键扫描、模式切换 |

- FreeRTOS V10.3.1，heap_4（16 KB），tick 1 kHz，`vTaskDelayUntil` 绝对周期调度
- 每个任务栈 512 words（`my_printf` 有 512 B 局部缓冲），开启栈溢出检测（mode 2）
- SysTick 与 HAL 共享：`HAL_IncTick()` + 调度器启动后 `xPortSysTickHandler()`
- 业务函数本体零改动，仅将原协作调度器的 4 个周期槽拆为 4 个任务

### 串口框架

4 路 UART 全部使用 **DMA 空闲中断接收**（`HAL_UARTEx_ReceiveToIdle_DMA`）+ 环形缓冲（rt-thread 风格 ringbuffer）解析，不丢帧：

| 外设 | 对端 | 作用 |
|------|------|------|
| USART1 | 电脑调试 | 日志输出 |
| USART2 | Emm_V5 闭环步进 (X 轴) | 角度指令 |
| USART3 | K230 视觉模块 | 目标坐标输入 |
| UART4 | Emm_V5 闭环步进 (Y 轴) | 角度指令 |
| USART6 | MSPM0 小车 | 联动协议 |

### 控制算法

- **双轴位置环 PID**：X/Y 轴独立闭环，积分限幅 `INT_LIMIT = 30` 防止 windup
- 配合 Emm_V5 闭环步进电机，20 ms 控制周期（key 任务）内完成角度解算
- **激光控制系统**：三模式（KEY2 切换）——手动、自动追踪发射、联动发射

### 迭代记录（开发日志）

1. **基础框架**：串口 DMA 空闲中断 + 环形缓冲，Emm_V5 闭环步进驱动
2. **追踪闭环**：第一版位置环 PID 出现过调/振荡问题 → 完成抗振荡专项系列修复（[Oscillation_Fix_Complete.md](Oscillation_Fix_Complete.md)、[X_Axis_Anti_Oscillation_Fix.md](X_Axis_Anti_Oscillation_Fix.md)、[Fine_Tuned_Anti_Oscillation.md](Fine_Tuned_Anti_Oscillation.md)）→ 收敛至稳定追踪
3. **系统增强**：激光三模式、K230 数据解析修复、小车直角弯联动
4. **赛后重构**：协作调度器迁移为 FreeRTOS 4 任务实时调度

## 小车板（`car_mspm0g3507/`，CCS Theia 工程）

- **MSPM0G3507**，5 路灰度传感器（L2/L1/M/R1/R2）巡线
- **双轮速度环 PID**：Kp = 1.8，Ki = 0.01，Kd = 0.08，编码器外部中断计数
- 按键控制：K1 设置圈数（1–5 圈），K2 启动；跑满设定圈数自动停车
- 检测到直角弯（L2 触发）→ 发送 `TURN_START\r\n` 联动云台

## 目录结构

```
├── Core/ Drivers/ MDK-ARM/ Middlewares/   # STM32 HAL 工程（Keil MDK-ARM V5）
├── app/          # FreeRTOS 任务、PID 应用层
├── bsp/          # 板级支持：串口、按键、电机、激光
├── ringbuffer/   # 环形缓冲（rt-thread 风格）
├── car_mspm0g3507/  # 小车板 MSPM0 工程（CCS Theia）
├── 2025template.ioc # CubeMX 配置（仅参考，见下方警告）
├── 矩形识别.py     # K230 侧矩形识别脚本
└── *.md           # 37 篇开发调参日志，按主题分类：
    ├── 抗振荡调参：Oscillation_Fix_Complete / Anti_Oscillation_Fix /
    │   Fine_Tuned_Anti_Oscillation / Adaptive_PID_Control_Guide / ...
    ├── 电机控制：Motor_Angle_Control_Guide / Y_Axis_Homing_Fix / ...
    ├── 激光系统：Laser_Control_System / Mode0_Laser_Misfire_Fix / ...
    └── 串口联调：Camera_Data_Fix_Guide / Car_Turn_Signal_Integration_Guide / ...
```

## 构建与烧录

### 云台板

- 工具链：Keil MDK-ARM V5（ARMCC V5.06），工程文件 `MDK-ARM/2025template.uvprojx`
- **警告**：不要用 CubeMX 重新生成本工程。`main.c` 手工配置为 HSI+PLL 80 MHz（`.ioc` 里是 HSE 25 MHz），FreeRTOS 为手动集成——重生成会破坏两者

### 小车板

- 工具链：TI CCS Theia，导入 `car_mspm0g3507/` 目录即可

## 奖项

全国大学生电子设计竞赛 E 题 **省级一等奖**
