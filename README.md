# UF4 STM32F401 DAPLink 固件

## 中文说明

本工程是 UF4 上位控制板上的 STM32F401RCT6 DAPLink 固件。它的职责不是运行目标业务逻辑，而是作为 PC 和板上多个目标芯片之间的调试、控制和串口桥接入口。

### 主要职责

- 通过 USB 暴露 CMSIS-DAP HID 接口，让 PC 上的 OpenOCD、pyOCD、Keil、STM32CubeProgrammer 等工具可以访问目标 SWD。
- 通过 TMUX1574 在不同目标 SWD 总线之间切换，目前支持 G474 和 F429 两路目标。
- 通过 USB CDC 提供串口桥接入口，当前默认枚举 1 个 CDC，连接到 G474 UART。
- 提供 UF4 自定义 CMSIS-DAP vendor command，用于切换目标、控制 ESP32 进入下载模式、复位 ESP32、读取固件版本等。
- 控制板载状态 LED、ESP32 BOOT/EN、目标 NRST、TMUX SEL/EN# 等板级信号。

### 硬件连接摘要

| 功能 | STM32F401 引脚 | 说明 |
| --- | --- | --- |
| USB FS DM/DP | PA11 / PA12 | TinyUSB device |
| Probe SWD 调试口 | PA13 / PA14 | STM32F401 自身下载调试 |
| 目标 NRST/SWCLK/SWDIO/SWO | PC10 / PC11 / PC12 / PC13 | 接到 TMUX1574 公共 D 侧 |
| TMUX SEL / EN# | PC8 / PC9 | EN# 低有效 |
| LED | PC0 | 状态灯 |
| ESP32 BOOT / EN | PA0 / PA1 | ESP32 下载/复位控制 |
| ESP32 UART | PA2 / PA3 | USART2 |
| G474 UART | PA9 / PA10 | USART1 |
| F429 UART | PC6 / PC7 | USART6 |

TMUX1574 选择关系：

```text
SEL = 0, EN# = 0 -> 连接 G474
SEL = 1, EN# = 0 -> 连接 F429
EN# = 1          -> 断开目标
```

### USB 枚举

当前默认 USB 组合设备：

```text
VID/PID      : CAFE:401B
Manufacturer : UF4
Product      : UF4 F401 CMSIS-DAP CDC
Serial       : UF4-F401-PLACEHOLDER
Interfaces   : CMSIS-DAP HID + 1 CDC ACM
```

注意：STM32F401 的 USB OTG FS IN endpoint 数量有限，默认没有启用 `HID + 3 CDC`，否则可能无法稳定枚举。后续若需要三路串口同时暴露，可以改成 vendor bulk 复用协议，或换用端点更多的 MCU。

### 自定义命令

自定义命令走 CMSIS-DAP HID vendor command，命令定义在 `DAP/vendor_cmd.h`。

| 命令 | ID | 请求 | 响应 | 说明 |
| --- | --- | --- | --- | --- |
| SET_TARGET | 0x80 | `[0x80, target]` | `[0x80, status]` | 切换目标 |
| GET_TARGET | 0x81 | `[0x81]` | `[0x81, status, target]` | 读取当前目标 |
| ESP32_BOOT | 0x82 | `[0x82]` | `[0x82, status]` | ESP32 进入下载模式 |
| ESP32_RESET | 0x83 | `[0x83]` | `[0x83, status]` | 复位 ESP32 |
| GET_VTREF | 0x84 | `[0x84]` | `[0x84, status, lo, hi]` | 读取目标电压，占位实现 |
| SET_LED | 0x85 | `[0x85, state]` | `[0x85, status]` | 设置 LED 状态 |
| GET_VERSION | 0x86 | `[0x86]` | `[0x86, status, major, minor, patch]` | 读取固件版本 |
| GET_BOARD_INFO | 0x87 | `[0x87]` | `[0x87, status, ...name]` | 读取板卡名称 |

目标 ID：

```text
0 = none
1 = G474
2 = F429
```

### 关键配置文件

- `BSP/board_config.h`：硬件引脚、TMUX 选择电平、板卡名称、版本号。
- `Config/memory_config.h`：DAP 包大小、CDC 数量、USB/UART 缓冲区大小。
- `USB/tusb_config.h`：TinyUSB 设备类数量、EP0/CDC/HID 缓冲配置。
- `USB/usb_descriptors.c`：USB VID/PID、字符串描述符、配置描述符。
- `DAP/DAP_config.h`：CMSIS-DAP 适配层，包含 SWD GPIO 操作和 DAP 信息字符串。
- `UART/uart_manager.c`：UART RX DMA、TX 中断发送、环形缓冲管理。

### 构建

工程由 STM32CubeMX 生成骨架，并在其 CMake 工程上增量加入 TinyUSB、CMSIS-DAP 和应用层代码。

使用 STM32CubeCLT / Ninja 构建：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

输出文件：

```text
build/DAPLINK.elf
build/DAPLINK.hex
build/DAPLINK.bin
build/DAPLINK.map
```

烧录 `.bin` 时起始地址为 `0x08000000`。

## English

This project is the STM32F401RCT6 DAPLink firmware for the UF4 upper controller board. Its job is not to run target application logic. It acts as the debug, control, and UART bridge between the host PC and multiple on-board target devices.

### Responsibilities

- Expose a CMSIS-DAP HID interface over USB for tools such as OpenOCD, pyOCD, Keil, and STM32CubeProgrammer.
- Switch the target SWD bus through a TMUX1574 analog switch. The current targets are G474 and F429.
- Expose a USB CDC serial bridge. The default firmware enumerates one CDC ACM interface connected to the G474 UART.
- Provide UF4-specific CMSIS-DAP vendor commands for target selection, ESP32 boot/reset control, firmware version query, and board information query.
- Control board-level signals such as the status LED, ESP32 BOOT/EN, target NRST, and TMUX SEL/EN#.

### Hardware Summary

| Function | STM32F401 pins | Notes |
| --- | --- | --- |
| USB FS DM/DP | PA11 / PA12 | TinyUSB device |
| Probe debug SWD | PA13 / PA14 | STM32F401 self-program/debug |
| Target NRST/SWCLK/SWDIO/SWO | PC10 / PC11 / PC12 / PC13 | Routed to the TMUX1574 common D side |
| TMUX SEL / EN# | PC8 / PC9 | EN# is active low |
| LED | PC0 | Status LED |
| ESP32 BOOT / EN | PA0 / PA1 | ESP32 download/reset control |
| ESP32 UART | PA2 / PA3 | USART2 |
| G474 UART | PA9 / PA10 | USART1 |
| F429 UART | PC6 / PC7 | USART6 |

TMUX1574 selection:

```text
SEL = 0, EN# = 0 -> G474 connected
SEL = 1, EN# = 0 -> F429 connected
EN# = 1          -> target disconnected
```

### USB Enumeration

Default USB composite device:

```text
VID/PID      : CAFE:401B
Manufacturer : UF4
Product      : UF4 F401 CMSIS-DAP CDC
Serial       : UF4-F401-PLACEHOLDER
Interfaces   : CMSIS-DAP HID + 1 CDC ACM
```

The STM32F401 USB OTG FS peripheral has a limited number of IN endpoints, so the default firmware does not enable `HID + 3 CDC`. If three UART channels must be exposed at the same time, use a vendor bulk multiplexing protocol or a MCU with more USB endpoints.

### Vendor Commands

UF4-specific commands are transported as CMSIS-DAP HID vendor commands. Command IDs are defined in `DAP/vendor_cmd.h`.

| Command | ID | Request | Response | Description |
| --- | --- | --- | --- | --- |
| SET_TARGET | 0x80 | `[0x80, target]` | `[0x80, status]` | Select target |
| GET_TARGET | 0x81 | `[0x81]` | `[0x81, status, target]` | Read selected target |
| ESP32_BOOT | 0x82 | `[0x82]` | `[0x82, status]` | Put ESP32 into download mode |
| ESP32_RESET | 0x83 | `[0x83]` | `[0x83, status]` | Reset ESP32 |
| GET_VTREF | 0x84 | `[0x84]` | `[0x84, status, lo, hi]` | Read target voltage, placeholder implementation |
| SET_LED | 0x85 | `[0x85, state]` | `[0x85, status]` | Set LED state |
| GET_VERSION | 0x86 | `[0x86]` | `[0x86, status, major, minor, patch]` | Read firmware version |
| GET_BOARD_INFO | 0x87 | `[0x87]` | `[0x87, status, ...name]` | Read board name |

Target IDs:

```text
0 = none
1 = G474
2 = F429
```

### Important Files

- `BSP/board_config.h`: pin map, TMUX levels, board name, firmware version.
- `Config/memory_config.h`: DAP packet size, CDC count, USB/UART buffer sizes.
- `USB/tusb_config.h`: TinyUSB class counts and endpoint buffer settings.
- `USB/usb_descriptors.c`: USB VID/PID, strings, and configuration descriptor.
- `DAP/DAP_config.h`: CMSIS-DAP hardware adaptation and SWD GPIO operations.
- `UART/uart_manager.c`: UART RX DMA, interrupt-driven TX, and ring buffers.

### Build

The project keeps the STM32CubeMX-generated skeleton and adds TinyUSB, CMSIS-DAP, and application modules incrementally through CMake.

Build with STM32CubeCLT / Ninja:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Outputs:

```text
build/DAPLINK.elf
build/DAPLINK.hex
build/DAPLINK.bin
build/DAPLINK.map
```

When flashing the raw `.bin`, use base address `0x08000000`.
