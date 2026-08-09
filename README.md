# UF4 STM32F401 DAPLink 固件

## 中文

这个固件跑在 UF4 上位控制板的 STM32F401RCT6 上。它夹在 PC 和板上的 G474、F429、ESP32 中间，负责调试、串口和一些板级控制。

PC 这边看到的是一个 USB 复合设备：一个 CMSIS-DAP HID，加一个 CDC 串口。调试软件通过 CMSIS-DAP 走 SWD；上位机如果要换目标，就发 UF4 自定义 vendor command，固件再去切 TMUX1574，把 SWD 接到 G474 或 F429。

### 它现在做什么

- USB 上枚举 CMSIS-DAP HID，给 OpenOCD、pyOCD、Keil、STM32CubeProgrammer 这类工具用。
- 通过 TMUX1574 切 SWD 目标，目前是 G474 和 F429 两路。
- USB CDC 默认只开 1 路，接 G474 UART。
- 通过 CMSIS-DAP vendor command 控制目标选择、ESP32 BOOT/EN、LED、版本读取等。
- 管 STM32F401 自己板上的几根控制脚：LED、ESP32 BOOT/EN、目标 NRST、TMUX SEL/EN#。

### 引脚

| 功能 | STM32F401 引脚 | 说明 |
| --- | --- | --- |
| USB FS DM/DP | PA11 / PA12 | TinyUSB device |
| F401 自身 SWD | PA13 / PA14 | 烧录和调试这个 DAPLink 固件 |
| 目标 NRST/SWCLK/SWDIO/SWO | PC10 / PC11 / PC12 / PC13 | 接到 TMUX1574 公共 D 侧 |
| TMUX SEL / EN# | PC8 / PC9 | EN# 低有效 |
| LED | PC0 | 状态灯 |
| ESP32 BOOT / EN | PA0 / PA1 | ESP32 下载和复位控制 |
| ESP32 UART | PA2 / PA3 | USART2 |
| G474 UART | PA9 / PA10 | USART1 |
| F429 UART | PC6 / PC7 | USART6 |

TMUX1574 的选择关系：

```text
SEL = 0, EN# = 0 -> 连接 G474
SEL = 1, EN# = 0 -> 连接 F429
EN# = 1          -> 断开目标
```

### USB 枚举

电脑上看到的默认设备信息：

```text
VID/PID      : CAFE:401B
Manufacturer : UF4
Product      : UF4 F401 CMSIS-DAP CDC
Serial       : UF4-F401-PLACEHOLDER
Interfaces   : CMSIS-DAP HID + 1 CDC ACM
```

这里没有默认做成 `HID + 3 CDC`。原因很简单：STM32F401 的 USB OTG FS IN endpoint 不宽裕，三路 CDC 再加 HID 很容易把端点用紧，枚举和兼容性都不舒服。现在先稳一点，只暴露 1 路 CDC。以后真要三路串口同时给 PC，可以改成 vendor bulk 复用协议，或者换端点更多的 MCU。

### 上位机怎么切目标

PC 端不需要看到多个 DAP。它只看到一个 CMSIS-DAP 设备，目标选择走 UF4 自定义命令：

| 命令 | ID | 请求 | 响应 | 说明 |
| --- | --- | --- | --- | --- |
| SET_TARGET | 0x80 | `[0x80, target]` | `[0x80, status]` | 切换目标 |
| GET_TARGET | 0x81 | `[0x81]` | `[0x81, status, target]` | 读取当前目标 |
| ESP32_BOOT | 0x82 | `[0x82]` | `[0x82, status]` | ESP32 进入下载模式 |
| ESP32_RESET | 0x83 | `[0x83]` | `[0x83, status]` | 复位 ESP32 |
| GET_VTREF | 0x84 | `[0x84]` | `[0x84, status, lo, hi]` | 读取目标电压，占位实现 |
| SET_LED | 0x85 | `[0x85, state]` | `[0x85, status]` | 设置 LED |
| GET_VERSION | 0x86 | `[0x86]` | `[0x86, status, major, minor, patch]` | 读取固件版本 |
| GET_BOARD_INFO | 0x87 | `[0x87]` | `[0x87, status, ...name]` | 读取板卡名称 |

目标 ID：

```text
0 = none
1 = G474
2 = F429
```

命令定义在 `DAP/vendor_cmd.h`。上位机切到 `G474` 后，SWD 和默认 CDC 都指向 G474；切到 `F429` 后，SWD 会切到 F429，CDC 仍按当前固件配置走默认串口。

### 常改的配置

- `BSP/board_config.h`：引脚、电平、板卡名称、版本号。
- `Config/memory_config.h`：DAP 包大小、CDC 数量、USB/UART 缓冲区大小。
- `USB/tusb_config.h`：TinyUSB 类数量、EP0/CDC/HID 缓冲。
- `USB/usb_descriptors.c`：USB VID/PID、字符串描述符、配置描述符。
- `DAP/DAP_config.h`：CMSIS-DAP 适配层，里面有 SWD GPIO 操作和 DAP 信息字符串。
- `UART/uart_manager.c`：UART RX DMA、TX 中断发送、环形缓冲。

### 构建

工程底子来自 STM32CubeMX，CMake 也是在 CubeMX 生成的结构上增量加 TinyUSB、CMSIS-DAP 和应用代码。

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

烧录 `.bin` 时起始地址用 `0x08000000`。

## English

This firmware runs on the STM32F401RCT6 on the UF4 upper controller board. It sits between the host PC and the on-board G474, F429, and ESP32, and handles debug, serial, and a few board-control signals.

On the PC side it enumerates as one USB composite device: CMSIS-DAP HID plus one CDC serial port. Debug tools talk SWD through CMSIS-DAP. When the host wants a different target, it sends a UF4 vendor command; the firmware then switches the TMUX1574 so SWD goes to either G474 or F429.

### What It Does

- Enumerates a CMSIS-DAP HID interface for OpenOCD, pyOCD, Keil, STM32CubeProgrammer, and similar tools.
- Switches the target SWD bus through TMUX1574. The current targets are G474 and F429.
- Exposes one USB CDC port by default, connected to the G474 UART.
- Uses CMSIS-DAP vendor commands for target selection, ESP32 BOOT/EN control, LED control, version query, and board info.
- Drives board-level signals such as LED, ESP32 BOOT/EN, target NRST, and TMUX SEL/EN#.

### Pins

| Function | STM32F401 pins | Notes |
| --- | --- | --- |
| USB FS DM/DP | PA11 / PA12 | TinyUSB device |
| F401 self SWD | PA13 / PA14 | Programming/debugging this DAPLink firmware |
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

Default USB identity:

```text
VID/PID      : CAFE:401B
Manufacturer : UF4
Product      : UF4 F401 CMSIS-DAP CDC
Serial       : UF4-F401-PLACEHOLDER
Interfaces   : CMSIS-DAP HID + 1 CDC ACM
```

The default build does not expose `HID + 3 CDC`. The STM32F401 USB OTG FS peripheral has a tight IN endpoint budget, and adding three CDC ports next to HID makes enumeration and compatibility harder to trust. For three simultaneous UART channels, use a vendor bulk multiplexing protocol or move to an MCU with more USB endpoints.

### Host Target Selection

The host does not see multiple DAP devices. It sees one CMSIS-DAP device, and target selection goes through UF4 vendor commands:

| Command | ID | Request | Response | Description |
| --- | --- | --- | --- | --- |
| SET_TARGET | 0x80 | `[0x80, target]` | `[0x80, status]` | Select target |
| GET_TARGET | 0x81 | `[0x81]` | `[0x81, status, target]` | Read selected target |
| ESP32_BOOT | 0x82 | `[0x82]` | `[0x82, status]` | Put ESP32 into download mode |
| ESP32_RESET | 0x83 | `[0x83]` | `[0x83, status]` | Reset ESP32 |
| GET_VTREF | 0x84 | `[0x84]` | `[0x84, status, lo, hi]` | Read target voltage, placeholder implementation |
| SET_LED | 0x85 | `[0x85, state]` | `[0x85, status]` | Set LED |
| GET_VERSION | 0x86 | `[0x86]` | `[0x86, status, major, minor, patch]` | Read firmware version |
| GET_BOARD_INFO | 0x87 | `[0x87]` | `[0x87, status, ...name]` | Read board name |

Target IDs:

```text
0 = none
1 = G474
2 = F429
```

Command IDs are defined in `DAP/vendor_cmd.h`. After selecting `G474`, SWD and the default CDC path point to G474. After selecting `F429`, SWD switches to F429; CDC still follows the current firmware's default UART configuration.

### Common Config Files

- `BSP/board_config.h`: pin map, signal levels, board name, firmware version.
- `Config/memory_config.h`: DAP packet size, CDC count, USB/UART buffer sizes.
- `USB/tusb_config.h`: TinyUSB class count and EP0/CDC/HID buffers.
- `USB/usb_descriptors.c`: USB VID/PID, string descriptors, configuration descriptor.
- `DAP/DAP_config.h`: CMSIS-DAP hardware adaptation, SWD GPIO operations, DAP info strings.
- `UART/uart_manager.c`: UART RX DMA, interrupt-driven TX, and ring buffers.

### Build

The project keeps the STM32CubeMX-generated skeleton. TinyUSB, CMSIS-DAP, and the application modules are added incrementally on top of that CMake project.

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
