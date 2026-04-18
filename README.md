中文版本在后面

# STM32_ESC_Telemetry_Test

> Single‑motor bench ESC controller firmware for `STM32F103C8T6 (Blue Pill)`.

This project is built on **STM32CubeMX / HAL** and reuses stable low‑level modules from the E1 project, then adds **ELRS / CRSF input**, **arm / disarm logic**, **failsafe**, **DShot output**, **OLED display**, **HC‑05 telemetry**, and **bench safety protections**.

---

## 🇬🇧 English

### 1. Project Status

| Item               | Detail                               |
|--------------------|--------------------------------------|
| Release status     | `V1.0` (first usable version)        |
| Target use case    | FYP bench demo (single ESC + single motor) |
| Control source     | ELRS receiver via `CRSF` on `USART2` |
| Motor output       | DShot on `PB8` (`TIM4_CH3 + DMA`)    |

---

### 2. Key Features

- ELRS / CRSF RC input (`420000 baud`, `USART2`)
- RC throttle + arm switch control
- Arm / disarm state machine with safety gates
- Receiver timeout failsafe
- DShot motor command output (single channel)
- OLED runtime status page (`I2C1`, SSD1306 128×64)
- HC‑05 debug / status link (`USART1`, `9600 baud`)
- Local key emergency stop latch / release
- ADC + DMA bus voltage / current monitoring
- ESC‑side software protections (overcurrent, overvoltage, regen related)

---

### 3. Hardware Mapping

**Board:** `STM32F103C8T6 Blue Pill`

| Pin   | Function                     |
|-------|------------------------------|
| `PA0` | ADC current input            |
| `PA1` | ADC voltage input            |
| `PA2` | USART2 TX (ELRS / CRSF)      |
| `PA3` | USART2 RX (ELRS / CRSF)      |
| `PA9` | USART1 TX (HC‑05)            |
| `PA10`| USART1 RX (HC‑05)            |
| `PB6` | I2C1 SCL (OLED)              |
| `PB7` | I2C1 SDA (OLED)              |
| `PB8` | ESC DShot output (`TIM4_CH3`)|
| `PB12`| HC‑05 STATE                  |
| `PB13`| USER KEY                     |
| `PC13`| USER LED                     |

---

### 4. Software Architecture

Code is organized by responsibility:

- **`app/`**
  - High‑level state machine and behaviour
  - CRSF parsing and RC data handling
  - Arm / disarm / failsafe logic
  - Motor command mapping and protections
  - Display model
- **`bsp/`**
  - HC‑05 transport and command parsing
  - Key debounce and event handling
  - ADC monitor and derived values
  - DShot low‑level driver
  - OLED low‑level driver
- **`Core/`**
  - CubeMX‑generated startup and HAL integration
- **`Drivers/`**
  - STM32 HAL + CMSIS

---

### 5. Main Runtime States

- `BOOT`
- `WAIT_RX`
- `READY`
- `ARMED`
- `FAILSAFE`
- `ESTOP`

**High‑level behaviour:**

- Boot → always output zero throttle
- No valid RC frames → `WAIT_RX`
- Valid and stable link → `READY`
- Arm conditions met → `ARMED`
- RC loss timeout after link established → `FAILSAFE`
- Local emergency stop → `ESTOP`

---

### 6. RC Mapping and Defaults

Defined in `app/inc/app_config.h`:

| Parameter                | Value               |
|--------------------------|---------------------|
| Throttle channel         | `CH3`               |
| Arm switch channel       | `CH5`               |
| RC endpoint model        | `988 µs ~ 2012 µs`  |
| Low‑throttle threshold   | `1050 µs`           |
| Arm switch OFF threshold | ≤ `1300 µs`         |
| Arm switch ON threshold  | ≥ `1700 µs`         |

---

### 7. Motor Output Policy

Current default policy is **bench‑friendly**:

- Disarmed → `DShot = 0`
- Armed + low throttle → `DShot = 0`
- Throttle above low threshold → mapped output
- Demo max command default: `DShot 1800`
- Output slew limiting enabled (up / down rate limits)

---

### 8. Protection Logic (Software‑side)

Motor control includes conservative protection checks:

- Soft current clamp behaviour
- Hard overcurrent trip
- Overvoltage trip
- Regen‑like negative current trip
- Latch and release conditions

> **Note:**  
> These protections help reduce risk but do **not** replace ESC hardware protections.  
> Bench tests should **always** be performed with proper safety precautions.

---

### 9. Receiver Path and Reliability Notes

Current RX implementation:

- `USART2` RX DMA in **circular mode**
- CRSF byte stream parsed continuously
- Avoids restart windows from stop / restart receive patterns

HC‑05 status transmission is **asynchronous (non‑blocking queue)** to avoid long loop stalls at `9600 baud`.

---

### 10. Telemetry and Debug

HC‑05 periodic status line includes fields such as:

- `state`, `rx`, `link`, `arm`, `arm_sw`
- `tl` (throttle low gate), `seen` (switch‑off seen flag)
- `drop` (arm drop reason)
- `thr_us`, `arm_us`, `dshot`
- `prot`, `reason`, `trip`
- `vf`, `ce`, `se`, `ue` (CRSF diagnostics)

**Supported HC‑05 commands:**

- `STATUS`
- `STOP`

---

### 11. Build

```bash
cmake --preset Debug
cmake --build --preset Debug

第一版可用版本标签：

- `V1.0`
