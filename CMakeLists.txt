cmake_minimum_required(VERSION 3.28)

# ---------- 交叉编译工具链 ----------
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NOT DEFINED TOOLCHAIN_FOLDER)
    set(TOOLCHAIN_FOLDER
            "E:/MounRiver/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC"
            CACHE PATH "WCH RISC-V GCC toolchain folder")
endif()

set(TOOLCHAIN_PREFIX "riscv-none-embed-")
set(TOOLCHAIN_BIN "${TOOLCHAIN_FOLDER}/bin")

if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}gcc.exe")
endif()
if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}g++.exe")
endif()
if(NOT CMAKE_ASM_COMPILER)
    set(CMAKE_ASM_COMPILER "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}gcc.exe")
endif()

set(CMAKE_OBJCOPY "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}objcopy.exe")
set(CMAKE_OBJDUMP "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}objdump.exe")
set(CMAKE_SIZE    "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}size.exe")
set(CMAKE_AR      "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}ar.exe")
set(CMAKE_RANLIB  "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}ranlib.exe")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ---------- 工程 ----------
project(DAPLINK LANGUAGES C CXX ASM)

# ---------- 语言标准 ----------
set(CMAKE_C_STANDARD          11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        ON)

# ---------- 编译选项 ----------
set(ARCH_FLAGS
        -march=rv32imacxw
        -mabi=ilp32
        -msmall-data-limit=8
        -msave-restore)

set(COMMON_FLAGS
        -fmax-errors=20
        -fmessage-length=0
        -fsigned-char
        -ffunction-sections
        -fdata-sections
        -fno-common
        -Wunused
        -Wuninitialized
        -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=..)

# 默认构建类型
if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Build type: Release  (-O2 -g)")
    set(OPT_FLAGS -O2 -g)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(STATUS "Build type: RelWithDebInfo  (-Ofast -g)")
    set(OPT_FLAGS -Ofast -g)
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    message(STATUS "Build type: MinSizeRel  (-Os)")
    set(OPT_FLAGS -Os)
else()
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}  (-O2 -g)")
    set(OPT_FLAGS -O2 -g)
endif()

# ---------- 源文件 ----------
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/User/*.c"
        "${CMAKE_SOURCE_DIR}/User/*.cpp"
        "${CMAKE_SOURCE_DIR}/Peripheral/*.c"
        "${CMAKE_SOURCE_DIR}/Debug/*.c"
        "${CMAKE_SOURCE_DIR}/Core/*.c")

set(STARTUP_ASM ${CMAKE_SOURCE_DIR}/Startup/startup_ch32v30x_D8C.S)
set(LINK_SCRIPT ${CMAKE_SOURCE_DIR}/CH32V30XRCT6.ld)

if(NOT EXISTS "${LINK_SCRIPT}")
    message(FATAL_ERROR "Linker script not found: ${LINK_SCRIPT}")
endif()

# ---------- 可执行目标 ----------
#option(USBHS_FORCE_FULL_SPEED "Use USBHS PHY at 12 Mbps for enumeration diagnostics" OFF)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${STARTUP_ASM})

# 让输出文件是 DAPLINK.elf（target 名保持 DAPLINK）
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/Debug"
        "${CMAKE_SOURCE_DIR}/Core"
        "${CMAKE_SOURCE_DIR}/User"
        "${CMAKE_SOURCE_DIR}/User/UART"
        "${CMAKE_SOURCE_DIR}/User/USB_Device"
        "${CMAKE_SOURCE_DIR}/Peripheral/inc"
        "${CMAKE_SOURCE_DIR}/Startup")

target_compile_definitions(${PROJECT_NAME} PRIVATE
        CH32V30x_D8C
        HSE_VALUE=8000000U
        $<$<BOOL:${USBHS_FORCE_FULL_SPEED}>:USBHS_FORCE_FULL_SPEED>
        $<$<BOOL:${USBHS_USE_HSI}>:USBHS_USE_HSI>)

target_compile_options(${PROJECT_NAME} PRIVATE ${ARCH_FLAGS} ${COMMON_FLAGS} ${OPT_FLAGS})

set_source_files_properties(${STARTUP_ASM} PROPERTIES LANGUAGE ASM COMPILE_OPTIONS "-x;assembler-with-cpp")

target_link_options(${PROJECT_NAME} PRIVATE
        ${ARCH_FLAGS}
        -nostartfiles
        -T ${LINK_SCRIPT}
        -T ${CMAKE_SOURCE_DIR}/usbhs_dma.ld
        --specs=nano.specs
        --specs=nosys.specs
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map
)

# ---------- 输出文件 ----------
set(HEX_FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex")
set(BIN_FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin")

set(UF4_BANNER_TXT    "${CMAKE_BINARY_DIR}/uf4_banner.txt")
set(UF4_BANNER_SCRIPT "${CMAKE_BINARY_DIR}/uf4_generate_banner.cmake")

file(WRITE "${UF4_BANNER_SCRIPT}" [=[
string(TIMESTAMP UF4_BUILD_TIME "%Y-%m-%d %H:%M:%S")
file(WRITE "${UF4_BANNER_TXT}" "
╔════════════════════════════════════════════════════════════════════════════════════════════════════════╗
║    ██╗   ██╗███████╗██╗  ██╗ ██████╗ ██╗   ██╗███████╗██████╗                                          ║
║    ██║   ██║██╔════╝██║  ██║██╔═══██╗██║   ██║██╔════╝██╔══██╗                                         ║
║    ██║   ██║█████╗  ███████║██║   ██║██║   ██║█████╗  ██████╔╝                                         ║
║    ██║   ██║██╔══╝  ╚════██║██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗                                         ║
║    ╚██████╔╝██║          ██║╚██████╔╝ ╚████╔╝ ███████╗██║  ██║                                         ║
║     ╚═════╝ ╚═╝          ╚═╝ ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝                                         ║
║                                                                                                        ║
║    ██████╗ ██╗   ██╗██╗██╗     ██████╗     ███████╗██╗   ██╗ ██████╗ ██████╗███████╗███████╗███████╗   ║
║    ██╔══██╗██║   ██║██║██║     ██╔══██╗    ██╔════╝██║   ██║██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝   ║
║    ██████╔╝██║   ██║██║██║     ██║  ██║    ███████╗██║   ██║██║     ██║     █████╗  ███████╗███████╗   ║
║    ██╔══██╗██║   ██║██║██║     ██║  ██║    ╚════██║██║   ██║██║     ██║     ██╔══╝  ╚════██║╚════██║   ║
║    ██████╔╝╚██████╔╝██║███████╗██████╔╝    ███████║╚██████╔╝╚██████╗╚██████╗███████╗███████║███████║   ║
║    ╚═════╝  ╚═════╝ ╚═╝╚══════╝╚═════╝     ╚══════╝ ╚═════╝  ╚═════╝ ╚═════╝╚══════╝╚══════╝╚══════╝   ║
╚════════════════════════════════════════════════════════════════════════════════════════════════════════╝
  UF4DigitalPower firmware generated successfully.
  Build time: ${UF4_BUILD_TIME}
")
]=])
# ---------- 构建后处理 ----------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex   $<TARGET_FILE:${PROJECT_NAME}> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${BIN_FILE}
        COMMAND ${CMAKE_COMMAND}
        -DUF4_BANNER_TXT=${UF4_BANNER_TXT}
        -DUF4_OUTPUT_ELF=$<TARGET_FILE:${PROJECT_NAME}>
        -P "${UF4_BANNER_SCRIPT}"
        COMMAND ${CMAKE_COMMAND} -E cat "${UF4_BANNER_TXT}"
        COMMENT "This version of the firmware is all manually maintained by UF4OVER"
        VERBATIM)
