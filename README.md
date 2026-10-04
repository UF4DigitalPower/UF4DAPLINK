# UF4 STM32F401 DAPLink 固件

一句话：这是运行在 UF4 STM32F401 上的 CMSIS-DAP + 双 CDC 桥接固件，用一个 USB 口完成 G474/F429 目标烧录调试、目标串口随动切换和 ESP32 独立串口/下载控制。

项目详情：[博客文章](https://me.hepi.ng/blog/UF4DP2/)

## 中文

这个固件跑在 UF4 上位控制板的 STM32F401RCT6 上。它夹在 PC 和板上的 G474、F429、ESP32 中间，负责调试、串口和一些板级控制。

PC 这边看到的是一个 USB 复合设备：一个 CMSIS-DAP HID，加两个 CDC 串口。调试软件通过 CMSIS-DAP 走 SWD；上位机如果要换目标，就发 UF4 自定义 vendor command，固件再去切 TMUX1574，把 SWD 和 Target CDC 一起接到 G474 或 F429。另一个 CDC 固定给 ESP32。

### 它现在做什么

- USB 上枚举 CMSIS-DAP HID，给 OpenOCD、pyOCD、Keil、STM32CubeProgrammer 这类工具用。
- 通过 TMUX1574 切 SWD 目标，目前是 G474 和 F429 两路。
- USB CDC 开 2 路：CDC0 跟着当前目标在 G474/F429 UART 之间切换，CDC1 固定接 ESP32 UART。
- 通过 CMSIS-DAP vendor command 控制目标选择、ESP32 串口下载/复位、LED、版本读取等。
- 管 STM32F401 自己板上的几根控制脚：LED、ESP32 IO8/IO9/EN、目标 NRST、TMUX SEL/EN#。

### 引脚

| 功能 | STM32F401 引脚 | 说明 |
| --- | --- | --- |
| USB FS DM/DP | PA11 / PA12 | TinyUSB device |
| F401 自身 SWD | PA13 / PA14 | 烧录和调试这个 DAPLink 固件 |
| 目标 NRST/SWCLK/SWDIO/SWO | PC10 / PC11 / PC12 / PC13 | 接到 TMUX1574 公共 D 侧 |
| TMUX SEL / EN# | PC8 / PC9 | EN# 低有效 |
| LED | PC4 | 状态灯 |
| ESP32 IO8 / IO9 / EN | PC1 / PC2 / PC3 | ESP32-C6 串口下载和复位控制 |
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
Product      : UF4 F401 CMSIS-DAP 2CDC
Serial       : UF4-F401-PLACEHOLDER
Interfaces   : CMSIS-DAP HID + 2 CDC ACM
```

这两个 CDC 都省略了 CDC notification endpoint，只保留 bulk IN/OUT data endpoint。这样端点账刚好够：

```text
EP1 IN/OUT -> CMSIS-DAP HID
EP2 IN/OUT -> CDC0 Target UART
EP3 IN/OUT -> CDC1 ESP32 UART
```

这里没有做成 `HID + 3 CDC`。STM32F401 的 `USB_OTG_FS_MAX_IN_ENDPOINTS` 是 4，包含 EP0，所以非 0 号 endpoint 只有 EP1、EP2、EP3。即使省略 CDC notification endpoint，`HID + 3 CDC` 仍然需要 EP4，F401 放不下。

### 上位机怎么切目标

PC 端不需要看到多个 DAP。它只看到一个 CMSIS-DAP 设备，目标选择走 UF4 自定义命令：

仓库里带了一个 Windows 原生 C 版小工具：

```powershell
.\tools\uf4-target\uf4-target.exe set f429
.\tools\uf4-target\uf4-target.exe set g474
.\tools\uf4-target\uf4-target.exe esp32-boot
.\tools\uf4-target\uf4-target.exe esp32-reset
.\tools\uf4-target\uf4-target.exe reset
.\tools\uf4-target\uf4-target.exe vtref
.\tools\uf4-target\uf4-target.exe vref-adc
.\tools\uf4-target\uf4-target.exe probe-idcode --target f429 --clock-khz 100 --repeat 3
```

OpenOCD 烧录 F429 前先切目标：

```powershell
.\tools\uf4-target\uf4-target.exe set f429
C:\OpenOCD\bin\openocd.exe -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "adapter speed 1000; program path\to\firmware.hex verify reset exit"
```

工具源码在 `tools/uf4-target/uf4-target.c`，可用 MinGW-w64 编译：

```powershell
gcc -O2 -Wall -Wextra -o tools\uf4-target\uf4-target.exe tools\uf4-target\uf4-target.c -lsetupapi -lhid
```

| 命令             | ID   | 请求               | 响应                                    | 说明                           |
|----------------|------|------------------|---------------------------------------|------------------------------|
| SET_TARGET     | 0x80 | `[0x80, target]` | `[0x80, status]`                      | 切换目标                         |
| GET_TARGET     | 0x81 | `[0x81]`         | `[0x81, status, target]`              | 读取当前目标                       |
| ESP32_BOOT     | 0x82 | `[0x82]`         | `[0x82, status]`                      | ESP32 进入下载模式                 |
| ESP32_RESET    | 0x83 | `[0x83]`         | `[0x83, status]`                      | 复位 ESP32                     |
| GET_VTREF      | 0x84 | `[0x84]`         | `[0x84, status, lo, hi]`              | 读取目标电压，按 1/2 分压换算            |
| SET_LED        | 0x85 | `[0x85, state]`  | `[0x85, status]`                      | 设置 LED                       |
| GET_VERSION    | 0x86 | `[0x86]`         | `[0x86, status, major, minor, patch]` | 读取固件版本                       |
| GET_BOARD_INFO | 0x87 | `[0x87]`         | `[0x87, status, ...name]`             | 读取板卡名称                       |
| RESET_ALL      | 0x8B | `[0x8B]`         | `[0x8B, status]`                      | 依次拉低复位 G474 和 F429，最后选通 F429 |

目标 ID：

```text
0 = none
1 = G474
2 = F429
```

命令定义在 `DAP/vendor_cmd.h`。上位机切到 `G474` 后，SWD 和 CDC0 都指向 G474；切到 `F429` 后，SWD 和 CDC0 一起切到 F429。CDC1 不跟着目标切换，始终接 ESP32。

### 常改的配置

- `BSP/board_config.h`：引脚、电平、板卡名称、版本号。
- `Config/memory_config.h`：DAP 包大小、CDC 数量、USB/UART 缓冲区大小。
- `USB/tusb_config.h`：TinyUSB 类数量、EP0/CDC/HID 缓冲。
- `USB/usb_descriptors.c`：USB VID/PID、字符串描述符、配置描述符。
- `DAP/DAP_config.h`：CMSIS-DAP 适配层，里面有 SWD GPIO 操作和 DAP 信息字符串。
- `UART/uart_manager.c`：UART RX DMA、TX 中断发送、环形缓冲。

### 构建

工程底子来自 STM32CubeMX，CMake 也是在 CubeMX 生成的结构上增量加 TinyUSB、CMSIS-DAP 和应用代码。

推荐环境：

- Windows
- STM32CubeCLT 1.16.x 或更新版本
- Ninja

如果 `cmake` 已经在 PATH 里，在仓库根目录执行：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

如果使用 STM32CubeCLT 自带的 CMake，可以直接指定完整路径：

```powershell
& "D:\Program Files\STMicroelectronics\STM32CubeCLT_1.16.0\CMake\bin\cmake.exe" -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
& "D:\Program Files\STMicroelectronics\STM32CubeCLT_1.16.0\CMake\bin\cmake.exe" --build build --config Release
```

输出文件：

```text
build/DAPLINK.elf
build/DAPLINK.hex
build/DAPLINK.bin
build/DAPLINK.map
```

烧录 `.bin` 时起始地址用 `0x08000000`。

### 开源协议

本工程按 `GPL-3.0-only` 开源，详见 `LICENSE`。第三方组件仍遵循各自目录下的原始许可证。

## English

One sentence: this is a CMSIS-DAP + dual-CDC bridge firmware for the UF4 STM32F401 board, using one USB connection for G474/F429 target debugging, target UART switching, and an independent ESP32 UART/download-control path.

This firmware runs on the STM32F401RCT6 on the UF4 upper controller board. It sits between the host PC and the on-board G474, F429, and ESP32, and handles debug, serial, and a few board-control signals.

On the PC side it enumerates as one USB composite device: CMSIS-DAP HID plus two CDC serial ports. Debug tools talk SWD through CMSIS-DAP. When the host wants a different target, it sends a UF4 vendor command; the firmware then switches TMUX1574 so SWD and the Target CDC both go to either G474 or F429. The other CDC stays connected to ESP32.

### What It Does

- Enumerates a CMSIS-DAP HID interface for OpenOCD, pyOCD, Keil, STM32CubeProgrammer, and similar tools.
- Switches the target SWD bus through TMUX1574. The current targets are G474 and F429.
- Exposes two USB CDC ports: CDC0 follows the selected G474/F429 target UART, and CDC1 is fixed to the ESP32 UART.
- Uses CMSIS-DAP vendor commands for target selection, ESP32 serial download/reset control, LED control, version query, and board info.
- Drives board-level signals such as LED, ESP32 IO8/IO9/EN, target NRST, and TMUX SEL/EN#.

### Pins

| Function                    | STM32F401 pins            | Notes                                       |
|-----------------------------|---------------------------|---------------------------------------------|
| USB FS DM/DP                | PA11 / PA12               | TinyUSB device                              |
| F401 self SWD               | PA13 / PA14               | Programming/debugging this DAPLink firmware |
| Target NRST/SWCLK/SWDIO/SWO | PC10 / PC11 / PC12 / PC13 | Routed to the TMUX1574 common D side        |
| TMUX SEL / EN#              | PC8 / PC9                 | EN# is active low                           |
| LED                         | PC4                       | Status LED                                  |
| ESP32 IO8 / IO9 / EN        | PC1 / PC2 / PC3           | ESP32-C6 serial download/reset control      |
| ESP32 UART                  | PA2 / PA3                 | USART2                                      |
| G474 UART                   | PA9 / PA10                | USART1                                      |
| F429 UART                   | PC6 / PC7                 | USART6                                      |

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
Product      : UF4 F401 CMSIS-DAP 2CDC
Serial       : UF4-F401-PLACEHOLDER
Interfaces   : CMSIS-DAP HID + 2 CDC ACM
```

Both CDC interfaces omit the CDC notification endpoint and keep only bulk IN/OUT data endpoints. The endpoint layout is:

```text
EP1 IN/OUT -> CMSIS-DAP HID
EP2 IN/OUT -> CDC0 Target UART
EP3 IN/OUT -> CDC1 ESP32 UART
```

The build does not expose `HID + 3 CDC`. On STM32F401, `USB_OTG_FS_MAX_IN_ENDPOINTS` is 4 including EP0, so only EP1, EP2, and EP3 are available as non-control endpoint numbers. Even without CDC notification endpoints, `HID + 3 CDC` would still need EP4.

### Host Target Selection

The host does not see multiple DAP devices. It sees one CMSIS-DAP device, and target selection goes through UF4 vendor commands:

The repository includes a native C Windows helper:

```powershell
.\tools\uf4-target\uf4-target.exe set f429
.\tools\uf4-target\uf4-target.exe set g474
.\tools\uf4-target\uf4-target.exe esp32-boot
.\tools\uf4-target\uf4-target.exe esp32-reset
.\tools\uf4-target\uf4-target.exe reset
.\tools\uf4-target\uf4-target.exe vtref
.\tools\uf4-target\uf4-target.exe vref-adc
.\tools\uf4-target\uf4-target.exe probe-idcode --target f429 --clock-khz 100 --repeat 3
```

Before flashing F429 with OpenOCD, select the target first:

```powershell
.\tools\uf4-target\uf4-target.exe set f429
C:\OpenOCD\bin\openocd.exe -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "adapter speed 1000; program path\to\firmware.hex verify reset exit"
```

The source is `tools/uf4-target/uf4-target.c`. Build it with MinGW-w64:

```powershell
gcc -O2 -Wall -Wextra -o tools\uf4-target\uf4-target.exe tools\uf4-target\uf4-target.c -lsetupapi -lhid
```

| Command | ID | Request | Response | Description |
| --- | --- | --- | --- | --- |
| SET_TARGET | 0x80 | `[0x80, target]` | `[0x80, status]` | Select target |
| GET_TARGET | 0x81 | `[0x81]` | `[0x81, status, target]` | Read selected target |
| ESP32_BOOT | 0x82 | `[0x82]` | `[0x82, status]` | Put ESP32 into download mode |
| ESP32_RESET | 0x83 | `[0x83]` | `[0x83, status]` | Reset ESP32 |
| GET_VTREF | 0x84 | `[0x84]` | `[0x84, status, lo, hi]` | Read target voltage, with 1/2 divider compensation |
| SET_LED | 0x85 | `[0x85, state]` | `[0x85, status]` | Set LED |
| GET_VERSION | 0x86 | `[0x86]` | `[0x86, status, major, minor, patch]` | Read firmware version |
| GET_BOARD_INFO | 0x87 | `[0x87]` | `[0x87, status, ...name]` | Read board name |
| RESET_ALL | 0x8B | `[0x8B]` | `[0x8B, status]` | Pulse reset on G474 and F429, then select F429 |

Target IDs:

```text
0 = none
1 = G474
2 = F429
```

Command IDs are defined in `DAP/vendor_cmd.h`. After selecting `G474`, SWD and CDC0 point to G474. After selecting `F429`, SWD and CDC0 switch to F429 together. CDC1 does not follow target selection; it always stays on ESP32.

For ESP32-C6 flashing, use CDC1 as the serial port. `esptool` can toggle DTR/RTS on that port and the firmware maps them to `GPIO9` and `EN` automatically:

```powershell
esptool.py --chip esp32c6 --port COMx write-flash 0x0 firmware.bin
```

### Common Config Files

- `BSP/board_config.h`: pin map, signal levels, board name, firmware version.
- `Config/memory_config.h`: DAP packet size, CDC count, USB/UART buffer sizes.
- `USB/tusb_config.h`: TinyUSB class count and EP0/CDC/HID buffers.
- `USB/usb_descriptors.c`: USB VID/PID, string descriptors, configuration descriptor.
- `DAP/DAP_config.h`: CMSIS-DAP hardware adaptation, SWD GPIO operations, DAP info strings.
- `UART/uart_manager.c`: UART RX DMA, interrupt-driven TX, and ring buffers.

### Build

The project keeps the STM32CubeMX-generated skeleton. TinyUSB, CMSIS-DAP, and the application modules are added incrementally on top of that CMake project.

Recommended environment:

- Windows
- STM32CubeCLT 1.16.x or newer
- Ninja

If `cmake` is already in PATH, run these commands from the repository root:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

If you want to use the CMake bundled with STM32CubeCLT, call it with the full path:

```powershell
& "D:\Program Files\STMicroelectronics\STM32CubeCLT_1.16.0\CMake\bin\cmake.exe" -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
& "D:\Program Files\STMicroelectronics\STM32CubeCLT_1.16.0\CMake\bin\cmake.exe" --build build --config Release
```

Outputs:

```text
build/DAPLINK.elf
build/DAPLINK.hex
build/DAPLINK.bin
build/DAPLINK.map
```

When flashing the raw `.bin`, use base address `0x08000000`.

### License

This project is released under `GPL-3.0-only`. See `LICENSE`. Third-party components keep their original licenses in their own directories.
