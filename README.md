# UF4 DAPLINK

UF4 DAPLINK 是面向 CH32V307（D8C）的 USB 高速复合设备固件。它把 UF4 调试/下载接口、三个 USB CDC 串口和 CMSIS-DAP SWD 调试能力集成在同一块硬件上，并提供 ESP32-C3 启动控制和串口下载支持。

## 功能

- CMSIS-DAP v1 HID 和 CMSIS-DAP v2 WinUSB bulk 两种 SWD 传输方式
- SWD 时钟、复位、线路自检、诊断和 posted-read 处理
- 三个 CDC ACM 串口：ESP32 UART、ESP32 BOOT 和 G474 UART
- 通过 CMSIS-DAP vendor command 控制 ESP32-C3 的 IO9/EN 启动时序
- USB 全速/高速描述符、WinUSB MS OS 2.0 描述符和 WebUSB 相关支持
- 构建后自动生成 `ELF`、`HEX`、`BIN` 和 `MAP` 文件

设备默认 USB 标识为 `VID:PID = 1A86:FE0C`。USB 接口分配如下：

| 接口  | 类型            | 用途           |
|-----|---------------|--------------|
| 0/1 | CDC ACM       | ESP32 UART   |
| 2/3 | CDC ACM       | ESP32 BOOT   |
| 4/5 | CDC ACM       | G474 UART    |
| 6   | WinUSB bulk   | CMSIS-DAP v2 |
| 7   | HID interrupt | CMSIS-DAP v1 |

SWD 仅支持 SWD 协议，不提供 JTAG。硬件引脚和 OpenOCD/ESP32-C3 操作说明见 [`Tools/uf4-dap/README.md`](Tools/uf4-dap/README.md)。

## 目录

| 路径                 | 内容                                    |
|--------------------|---------------------------------------|
| `User/`            | 应用入口、USB 复合设备、CMSIS-DAP、SWD 和 UART 代码 |
| `Peripheral/`      | CH32V30x 外设驱动和头文件                     |
| `Core/`、`Startup/` | RISC-V 内核支持和启动文件                      |
| `Debug/`           | 调试相关公共代码                              |
| `Tools/uf4-dap/`   | Windows 下的 WinUSB/HID/SWD/ESP32 辅助工具  |
| `Tests/`           | 主机侧协议、描述符和 SWD 线路测试                   |
| `Docs/`            | 原理图、框图等硬件资料                           |
| `示例代码库/`           | TinyUSB、CMSIS-DAP 和其他参考工程             |

## 构建固件

### 环境

- Windows 10/11
- CMake 3.28 或更高版本
- Ninja（推荐）或其他 CMake 生成器
- WCH RISC-V Embedded GCC，工具链前缀为 `riscv-none-embed-`

`CMakeLists.txt` 默认从 MounRiver Studio 安装目录查找 WCH 工具链。如果工具链安装在其他位置，配置时传入 `TOOLCHAIN_FOLDER`：

```powershell
cmake -S . -B cmake-build-debug -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DTOOLCHAIN_FOLDER="C:/path/to/RISC-V Embedded GCC"
cmake --build cmake-build-debug --parallel
```

构建完成后，文件位于 `cmake-build-debug/`：

- `DAPLINK.elf`：调试信息完整的 ELF 文件
- `DAPLINK.hex`：使用 WCH-Link、MounRiver 或兼容工具烧录的 Intel HEX 文件
- `DAPLINK.bin`：原始二进制固件
- `DAPLINK.map`：链接映射文件

将固件写入 CH32V307 后，重新插拔 USB，使新的 USB 描述符和 CMSIS-DAP 功能重新枚举。构建生成文件不会自动更新已经连接的探针。

## 运行测试

主机侧测试不需要连接硬件，但需要 Python 3 和可执行的 `gcc`（MinGW-w64）：

```powershell
python Tests/test_usb_descriptors.py
python Tests/test_dap_transfers.py
python Tests/test_swd_wire.py
```

这些测试分别检查 USB FS/HS 描述符、CMSIS-DAP 命令层、SWD SPI/GPIO 时序和控制引脚逻辑。它们不会向目标芯片写入固件，也不能替代真实硬件上的 USB 枚举和 SWD 联调。

## 辅助工具

`Tools/uf4-dap/README.md` 包含以下内容：

- `uf4-dap` 的编译、设备枚举、CMSIS-DAP/SWD 命令和线路诊断
- OpenOCD HID/bulk 配置示例
- ESP32-C3 进入下载模式、探测和烧录流程
- G474 目标的读取、编程和校验工具

Windows 下编译原生辅助工具示例：

```powershell
gcc -O2 -Wall -Wextra -o Tools/uf4-dap/uf4-dap.exe `
  Tools/uf4-dap/uf4-dap.c -lsetupapi -lwinusb
```

ESP32-C3 下载脚本需要 esptool 5.x：

```powershell
python -m pip install "esptool>=5,<6"
python Tools/uf4-dap/uf4-esp32.py --port COMx probe
```

串口号会随 Windows 设备枚举变化，请以设备管理器显示的 CDC 端口为准。

## 相关资料

- [`Tools/uf4-dap/README.md`](Tools/uf4-dap/README.md)：详细工具和硬件操作手册
- [`Docs/LINK原理图.png`](Docs/LINK原理图.png)：LINK 硬件原理图
- [`Docs/V2_UF4FOC框图 .png`](Docs/V2_UF4FOC框图%20.png)：系统框图

## 许可

仓库包含来自不同上游项目的参考代码；使用和再发布时请同时遵守各子目录中的许可证和版权声明。
