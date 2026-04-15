# STM32_ESC_Telemetry_Test

STM32F103C8T6 Blue Pill bench-demo firmware for single-motor ESC control using DShot output, ELRS/CRSF receiver input, HC-05 debug telemetry, OLED status display, local safety key, and ADC-based bus monitoring.

This project is built on a new STM32CubeMX / HAL project and reuses the stable low-level logic migrated from the previous E1 experiment firmware wherever practical.

---

## English

### Overview

This firmware is a practical FYP bench controller for a single ESC + motor test setup.

Main capabilities:

- receive ELRS receiver data over `USART2` using `CRSF`
- use one RC channel as throttle and one switch channel as `ARM / DISARM`
- apply receiver timeout failsafe
- output DShot throttle on `PA6`
- keep HC-05 support on `USART1` for debug and status output
- show runtime status on a `1.3" I2C OLED`
- support a local user key for emergency stop latch / release
- monitor bus current and bus voltage through `ADC1 + DMA`

Safety is the top priority:

- motor output defaults to zero on boot
- motor output is zero when receiver data is missing
- motor output is zero on failsafe
- motor output is zero when disarmed
- arming requires low throttle and a valid receiver link

### Hardware Target

Board:

- `STM32F103C8T6 Blue Pill`

Fixed peripheral assignment:

- `PA0` -> bus current ADC input
- `PA1` -> bus voltage ADC input
- `PA6` -> ESC throttle output (`TIM3_CH1`, DShot)
- `PA9 / PA10` -> HC-05 on `USART1`
- `PA2 / PA3` -> ELRS receiver on `USART2`
- `PB6 / PB7` -> OLED on `I2C1`
- `PB12` -> HC-05 `STATE`
- `PB13` -> user key
- `PC13` -> user LED

### Firmware Architecture

The project is split by responsibility:

- `app/`
  - top-level application flow and logic
  - CRSF receiver parsing
  - arm / disarm / failsafe state handling
  - throttle mapping and display model generation
- `bsp/`
  - board-support level reusable modules
  - HC-05, key debounce, ADC monitor
  - DShot and OLED low-level drivers reused from the E1 project
- `Core/`
  - STM32CubeMX-generated startup and HAL integration files
- `Drivers/`
  - STM32 HAL and CMSIS vendor code

### Current Application States

The high-level states are:

- `BOOT`
- `WAIT_RX`
- `READY`
- `ARMED`
- `FAILSAFE`
- `ESTOP`

Arming rules:

- valid CRSF frames must be present
- throttle must be low
- arm switch must be ON
- arm switch must have seen an `OFF -> ON` transition
- estop latch must be cleared

Disarm / stop conditions:

- arm switch OFF
- CRSF timeout
- failsafe condition
- local E-stop key
- HC-05 `STOP` command

### RC Mapping Defaults

Configured in `app/inc/app_config.h`:

- throttle channel: `CH3`
- arm switch channel: `CH5`
- low throttle threshold: `1050 us`
- arm switch ON threshold: `1600 us`
- max demo DShot command: `600`

### Local UI and Telemetry

OLED main page shows:

- receiver status
- arm status
- throttle input
- DShot command
- bus voltage and current

HC-05 debug link supports:

- periodic status output
- `STATUS` command
- `STOP` command

User key behavior:

- short press once -> latch `ESTOP`
- short press again -> clear `ESTOP` only when safe

### Build

Example build commands:

```bash
cmake --preset Debug
cmake --build --preset Debug
```

Expected output:

- `build/Debug/STM32_ESC_Telemetry_Test.elf`

### Notes

- This is a bench demo controller, not a full flight controller.
- The project assumes the ELRS receiver outputs standard `CRSF` serial frames at `420000 baud`.
- The clock tree is configured back to `72 MHz` to preserve timing margin for DShot and high-speed serial reception.

---

## 中文

### 项目概述

这是一个面向毕设台架实验的单电机 ESC 控制固件，运行在 `STM32F103C8T6 Blue Pill` 上。

主要功能包括：

- 通过 `USART2` 接收 ELRS 的 `CRSF` 串口数据
- 使用一个 RC 通道作为油门输入
- 使用一个开关通道作为 `ARM / DISARM`
- 在接收机失联时执行 failsafe 停机
- 在 `PA6` 输出 DShot 油门到 ESC
- 保留 `USART1` 上的 HC-05 调试与状态输出
- 在 `1.3 寸 I2C OLED` 上显示运行状态
- 使用本地按键实现紧急停机锁存 / 解锁
- 使用 `ADC1 + DMA` 采集母线电流和母线电压

本项目优先保证台架安全：

- 上电默认零油门
- 没有接收机数据时强制零油门
- failsafe 时强制零油门
- 未解锁时强制零油门
- 只有低油门且链路有效时才允许 ARM

### 硬件目标

开发板：

- `STM32F103C8T6 Blue Pill`

固定引脚分配如下：

- `PA0` -> 母线电流 ADC 输入
- `PA1` -> 母线电压 ADC 输入
- `PA6` -> ESC 油门输出（`TIM3_CH1`, DShot）
- `PA9 / PA10` -> HC-05（`USART1`）
- `PA2 / PA3` -> ELRS 接收机（`USART2`）
- `PB6 / PB7` -> OLED（`I2C1`）
- `PB12` -> HC-05 `STATE`
- `PB13` -> 用户按键
- `PC13` -> 用户 LED

### 软件结构

工程按职责分层：

- `app/`
  - 顶层应用流程与控制逻辑
  - CRSF 接收解析
  - ARM / DISARM / failsafe 状态管理
  - 油门映射与显示模型整理
- `bsp/`
  - 板级支持模块
  - HC-05、按键去抖、ADC 监测
  - 从 E1 项目复用过来的 DShot 与 OLED 底层
- `Core/`
  - STM32CubeMX 生成的启动与 HAL 集成代码
- `Drivers/`
  - STM32 HAL 与 CMSIS 官方代码

### 当前应用状态

高层状态包括：

- `BOOT`
- `WAIT_RX`
- `READY`
- `ARMED`
- `FAILSAFE`
- `ESTOP`

允许 ARM 的条件：

- 必须持续收到有效 CRSF 帧
- 油门必须处于低位
- ARM 开关必须为 ON
- ARM 开关必须经历一次 `OFF -> ON`
- 本地 `ESTOP` 必须已经解除

触发停机 / 退出 ARM 的条件：

- ARM 开关 OFF
- CRSF 超时
- failsafe
- 本地急停按键
- HC-05 的 `STOP` 命令

### 默认 RC 映射

配置位于 `app/inc/app_config.h`：

- 油门通道：`CH3`
- ARM 开关通道：`CH5`
- 低油门阈值：`1050 us`
- ARM 开关导通阈值：`1600 us`
- 演示用最大 DShot 命令：`600`

### 本地显示与调试

OLED 主页面显示：

- 接收机状态
- 解锁状态
- 油门输入
- DShot 命令
- 母线电压与电流

HC-05 调试链路支持：

- 周期性状态输出
- `STATUS` 命令
- `STOP` 命令

用户按键行为：

- 短按一次 -> 锁存 `ESTOP`
- 再短按一次 -> 只有在安全条件满足时才解除 `ESTOP`

### 编译

编译示例：

```bash
cmake --preset Debug
cmake --build --preset Debug
```

产物示例：

- `build/Debug/STM32_ESC_Telemetry_Test.elf`

### 说明

- 这是一个台架演示控制器，不是完整飞控。
- 项目默认 ELRS 接收机通过 `420000 baud` 输出标准 `CRSF` 串口帧。
- 系统时钟已经恢复为 `72 MHz`，以保证 DShot 与高速串口接收的时序裕量。

