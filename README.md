# 🏓 Smart Ping Pong Launcher (多功能智能乒乓球发球机)

[![STM32F407](https://img.shields.io/badge/MCU-STM32F407-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f407.html)
[![Language](https://img.shields.io/badge/Language-C-green.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Status](https://img.shields.io/badge/Status-Completed-success.svg)]()

> **2026 Fudan University Electronic Design Contest (Group C13)**
> 
> **2026年复旦大学电子设计竞赛 C题 参赛开源项目**

**English** | [中文介绍](#中文介绍)

A highly stable, multi-functional table tennis ball launcher based on the **STM32F407** microcontroller. This project utilizes dual DC motors (380 series) for friction-wheel propulsion and high-torque digital servos (MG996R) for a 2-axis (Pitch & Yaw) aiming gimbal. 

To overcome the physical limitations of low-cost hardware (e.g., static friction dead-zones, high voltage drops), we implemented several robust purely-software algorithms, including a `Kick-start` sprint algorithm and a `Smooth Damping` servo controller.

---

## 中文介绍

本项目为2026年复旦大学电子设计竞赛C题（多功能乒乓球发球机）的完整软硬件开源方案。系统以 **STM32F407** 为核心主控，通过严格的时钟频率隔离（50Hz舵机与1kHz电机总线分离），实现了对底层硬件的极限压榨与精准控制。

### ✨ 核心技术特性 (Key Features)

* 🚀 **Kick-start 静摩擦冲刺算法：** 针对380电机+L298N在低PWM下的死区问题，在零档起步瞬间注入 50ms 满功率脉冲，随后主循环平滑接管，实现真正的“零卡顿”起步。
* 🌪️ **暴力极限侧旋控制：** 摒弃常规同向差速，采用“单侧推进+单侧半速反转切削”的非对称物理模型，将发球动能极限转化为剧烈侧旋。
* 🦾 **机械阻尼感云台控制：** 放弃生硬的角度直驱，引入“目标值解耦+浮点数步长跟随”算法。使得云台（俯仰/偏航）在执行“一跳/两跳”和“全台扫掠”动作时，极具丝滑的物理阻尼质感。
* 🛡️ **高并发防死机架构：** 舍弃不稳定的外设中断，重构为 168MHz HSI 内部核心时钟+高频状态机轮询，保证赛场上的绝对可靠性。

---

## 🛠️ 硬件清单 (Hardware Bill of Materials)

| 组件名称 (Component) | 型号/规格 | 数量 | 用途描述 |
| :--- | :--- | :--- | :--- |
| **MCU 核心板** | STM32F407VET6 | 1 | 系统运算与状态机控制核心 |
| **电机驱动** | L298N | 1 | 驱动双路直流发球电机 |
| **动力马达** | 380 直流有刷马达 | 2 | 结合高阻力摩擦轮，提供发球初速度与旋转 |
| **姿态云台** | MG996R 高扭矩数字舵机 | 2 | 控制发射轨道的俯仰角(Pitch)与偏航角(Yaw) |
| **降压稳压模块** | LM2596S / 6V Buck | 1 | 为舵机独立供电，避免单片机大电流瞬间假死 |
| **交互按键** | 独立轻触按键 | 5 | 触发系统状态机切换 |

---

## 🔌 核心引脚映射 (Pinout Mapping)

| 引脚 (Pin) | 功能 (Function) | 外设配置 (Configuration) | 备注描述 (Notes) |
| :--- | :--- | :--- | :--- |
| **PE0** | 加速 (Speed Up) | `GPIO_Input` (Pull-up) | 最高至5档 |
| **PE1** | 减速 (Speed Down)| `GPIO_Input` (Pull-up) | 降至0档平滑刹停 |
| **PE2** | 侧旋 (Spin Mode) | `GPIO_Input` (Pull-up) | 直线 / 左侧旋 / 右侧旋 循环 |
| **PE3** | 俯仰 (Pitch) | `GPIO_Input` (Pull-up) | 一跳(115°) / 回正(90°) / 两跳(70°) 钟摆循环 |
| **PE4** | 偏航 (Yaw) | `GPIO_Input` (Pull-up) | 居中(90°) / 左(120°) / 居中 / 右(60°) 循环 |
| **PD12**| 左轮 PWM | `TIM4_CH1` | 1kHz, 控制左轮速度 |
| **PD13**| 右轮 PWM | `TIM4_CH2` | 1kHz, 控制右轮速度 |
| **PD0~PD3**| 电机方向控制 | `GPIO_Output` | 控制推拉与反转逻辑 |
| **PA6** | 偏航舵机信号 | `TIM3_CH1` | 50Hz, 严格独立定时器 |
| **PA7** | 俯仰舵机信号 | `TIM3_CH2` | 50Hz, 严格独立定时器 |

---

## 💻 编译与烧录说明 (How to Build)

1.  **开发环境：** 推荐使用 `CLion` 配合 `STM32CubeMX`，或直接使用 `STM32CubeIDE`。
2.  **工程结构：** 代码主体逻辑位于 `Core/Src/main.c`。
3.  **时钟说明：** 核心代码已内置 168MHz 的 `SystemClock_Config()` 强校验，即使外部晶振(HSE)起振失败，也能使用内部高速时钟(HSI)完美运行 50Hz 舵机信号。
4.  **警告 ⚠️：** **必须严格实现共地 (Common Ground)！** L298N、降压模块的 GND 必须与 STM32 开发板的 GND 物理相连，否则数字舵机将无法解码 PWM 信号导致休眠假死。

---

## 👥 参赛团队 (Team C13)

* **Xiong Jian (熊简)** - [Student ID: 24300720236]
* **Duan Yunfang (段蕴芳)** - [Student ID: 24300720230]
* **Tian Yufei (田雨霏)** - [Student ID: 24300720218]

*Designed for the 2026 Fudan University Electronic Design Contest. All rights reserved.*
