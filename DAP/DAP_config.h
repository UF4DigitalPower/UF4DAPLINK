/*
 * STM32F401 UF4 CMSIS-DAP hardware port.
 *
 * This file adapts Arm's Apache-2.0 CMSIS-DAP reference implementation to the
 * local board abstraction. All concrete GPIO symbols come from board_config.h.
 */

#ifndef __DAP_CONFIG_H__
#define __DAP_CONFIG_H__

#include <string.h>
#include "stm32f4xx_hal.h"
#include "../BSP/board_config.h"
#include "../BSP/led.h"
#include "../DAP/target.h"
#include "../Config/memory_config.h"

#define CPU_CLOCK                       84000000U
#define IO_PORT_WRITE_CYCLES            2U

#define DAP_SWD                         1
#define DAP_JTAG                        0
#define DAP_JTAG_DEV_CNT                0U
#define DAP_DEFAULT_PORT                1U
#define DAP_DEFAULT_SWJ_CLOCK           1000000U

#define SWO_UART                        0
#define SWO_UART_DRIVER                 0
#define SWO_UART_MAX_BAUDRATE           0U
#define SWO_MANCHESTER                  0
#define SWO_BUFFER_SIZE                 0U
#define SWO_STREAM                      0

#define TIMESTAMP_CLOCK                 0U

#define DAP_UART                        0
#define DAP_UART_DRIVER                 0
#define DAP_UART_RX_BUFFER_SIZE         0U
#define DAP_UART_TX_BUFFER_SIZE         0U
#define DAP_UART_USB_COM_PORT           0

#define TARGET_FIXED                    0

__STATIC_INLINE uint8_t DAP_CopyString(char *str, const char *src)
{
    uint8_t len = (uint8_t)(strlen(src) + 1U);
    memcpy(str, src, len);
    return len;
}

__STATIC_INLINE uint8_t DAP_GetVendorString(char *str)
{
    return DAP_CopyString(str, "UF4");
}

__STATIC_INLINE uint8_t DAP_GetProductString(char *str)
{
    return DAP_CopyString(str, UF4_BOARD_NAME);
}

__STATIC_INLINE uint8_t DAP_GetSerNumString(char *str)
{
    return DAP_CopyString(str, "UF4F40100000001");
}

__STATIC_INLINE uint8_t DAP_GetTargetDeviceVendorString(char *str)
{
    (void)str;
    return 0U;
}

__STATIC_INLINE uint8_t DAP_GetTargetDeviceNameString(char *str)
{
    (void)str;
    return 0U;
}

__STATIC_INLINE uint8_t DAP_GetTargetBoardVendorString(char *str)
{
    (void)str;
    return 0U;
}

__STATIC_INLINE uint8_t DAP_GetTargetBoardNameString(char *str)
{
    (void)str;
    return 0U;
}

__STATIC_INLINE uint8_t DAP_GetProductFirmwareVersionString(char *str)
{
    return DAP_CopyString(str, "0.1.0");
}

__STATIC_FORCEINLINE uint32_t UF4_PinIndex(uint16_t pin)
{
    uint32_t index = 0U;

    while (((pin >> index) & 1U) == 0U) {
        index++;
    }

    return index;
}

__STATIC_FORCEINLINE void UF4_GPIO_Mode(GPIO_TypeDef *port,
                                        uint16_t pin,
                                        uint32_t mode,
                                        uint32_t otype,
                                        uint32_t pull,
                                        uint32_t speed)
{
    uint32_t pos;
    uint32_t mask;

    if ((port == GPIO_PORT_PLACEHOLDER) || (pin == GPIO_PIN_PLACEHOLDER)) {
        return;
    }

    pos = UF4_PinIndex(pin);
    mask = 3UL << (pos * 2U);

    port->MODER = (port->MODER & ~mask) | (mode << (pos * 2U));
    port->PUPDR = (port->PUPDR & ~mask) | (pull << (pos * 2U));
    port->OSPEEDR = (port->OSPEEDR & ~mask) | (speed << (pos * 2U));

    if (otype != 0U) {
        port->OTYPER |= pin;
    } else {
        port->OTYPER &= ~pin;
    }
}

__STATIC_FORCEINLINE void UF4_GPIO_Set(GPIO_TypeDef *port, uint16_t pin)
{
    if ((port != GPIO_PORT_PLACEHOLDER) && (pin != GPIO_PIN_PLACEHOLDER)) {
        port->BSRR = pin;
    }
}

__STATIC_FORCEINLINE void UF4_GPIO_Clr(GPIO_TypeDef *port, uint16_t pin)
{
    if ((port != GPIO_PORT_PLACEHOLDER) && (pin != GPIO_PIN_PLACEHOLDER)) {
        port->BSRR = ((uint32_t)pin << 16U);
    }
}

__STATIC_FORCEINLINE uint32_t UF4_GPIO_In(GPIO_TypeDef *port, uint16_t pin)
{
    if ((port == GPIO_PORT_PLACEHOLDER) || (pin == GPIO_PIN_PLACEHOLDER)) {
        return 0U;
    }

    return ((port->IDR & pin) != 0U) ? 1U : 0U;
}

__STATIC_FORCEINLINE void UF4_NRST_Release(void)
{
    UF4_GPIO_Set(TARGET_NRST_GPIO, TARGET_NRST_PIN);
    UF4_GPIO_Mode(TARGET_NRST_GPIO, TARGET_NRST_PIN, 1U, 1U, 1U, 2U);
}

__STATIC_FORCEINLINE void UF4_NRST_Assert(void)
{
    UF4_GPIO_Clr(TARGET_NRST_GPIO, TARGET_NRST_PIN);
    UF4_GPIO_Mode(TARGET_NRST_GPIO, TARGET_NRST_PIN, 1U, 1U, 0U, 2U);
}

__STATIC_INLINE void PORT_SWD_SETUP(void)
{
    UF4_GPIO_Set(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN);
    UF4_NRST_Release();
    UF4_GPIO_Clr(TARGET_SWCLK_GPIO, TARGET_SWCLK_PIN);

    UF4_GPIO_Mode(TARGET_SWCLK_GPIO, TARGET_SWCLK_PIN, 1U, 0U, 0U, 3U);
    UF4_GPIO_Mode(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN, 1U, 0U, 1U, 3U);
    UF4_GPIO_Mode(TARGET_SWO_GPIO, TARGET_SWO_PIN, 0U, 0U, 0U, 2U);
}

__STATIC_INLINE void PORT_JTAG_SETUP(void)
{
    PORT_SWD_SETUP();
}

__STATIC_INLINE void PORT_OFF(void)
{
    UF4_NRST_Release();
    UF4_GPIO_Set(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN);
    UF4_GPIO_Clr(TARGET_SWCLK_GPIO, TARGET_SWCLK_PIN);

    UF4_GPIO_Mode(TARGET_SWCLK_GPIO, TARGET_SWCLK_PIN, 0U, 0U, 0U, 0U);
    UF4_GPIO_Mode(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN, 0U, 0U, 1U, 0U);
    UF4_GPIO_Mode(TARGET_SWO_GPIO, TARGET_SWO_PIN, 0U, 0U, 0U, 0U);
}

__STATIC_FORCEINLINE uint32_t PIN_SWCLK_TCK_IN(void)
{
    return UF4_GPIO_In(TARGET_SWCLK_GPIO, TARGET_SWCLK_PIN);
}

__STATIC_FORCEINLINE void PIN_SWCLK_TCK_SET(void)
{
    UF4_GPIO_Set(TARGET_SWCLK_GPIO, TARGET_SWCLK_PIN);
}

__STATIC_FORCEINLINE void PIN_SWCLK_TCK_CLR(void)
{
    UF4_GPIO_Clr(TARGET_SWCLK_GPIO, TARGET_SWCLK_PIN);
}

__STATIC_FORCEINLINE uint32_t PIN_SWDIO_TMS_IN(void)
{
    return UF4_GPIO_In(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN);
}

__STATIC_FORCEINLINE void PIN_SWDIO_TMS_SET(void)
{
    UF4_GPIO_Set(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN);
}

__STATIC_FORCEINLINE void PIN_SWDIO_TMS_CLR(void)
{
    UF4_GPIO_Clr(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN);
}

__STATIC_FORCEINLINE uint32_t PIN_SWDIO_IN(void)
{
    return UF4_GPIO_In(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN);
}

__STATIC_FORCEINLINE void PIN_SWDIO_OUT(uint32_t bit)
{
    if ((bit & 1U) != 0U) {
        UF4_GPIO_Set(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN);
    } else {
        UF4_GPIO_Clr(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN);
    }
}

__STATIC_FORCEINLINE void PIN_SWDIO_OUT_ENABLE(void)
{
    UF4_GPIO_Mode(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN, 1U, 0U, 1U, 3U);
}

__STATIC_FORCEINLINE void PIN_SWDIO_OUT_DISABLE(void)
{
    UF4_GPIO_Mode(TARGET_SWDIO_GPIO, TARGET_SWDIO_PIN, 0U, 0U, 0U, 3U);
}

__STATIC_FORCEINLINE uint32_t PIN_TDI_IN(void)
{
    return 0U;
}

__STATIC_FORCEINLINE void PIN_TDI_OUT(uint32_t bit)
{
    (void)bit;
}

__STATIC_FORCEINLINE uint32_t PIN_TDO_IN(void)
{
    return 0U;
}

__STATIC_FORCEINLINE uint32_t PIN_nTRST_IN(void)
{
    return 1U;
}

__STATIC_FORCEINLINE void PIN_nTRST_OUT(uint32_t bit)
{
    (void)bit;
}

__STATIC_FORCEINLINE uint32_t PIN_nRESET_IN(void)
{
    return UF4_GPIO_In(TARGET_NRST_GPIO, TARGET_NRST_PIN);
}

__STATIC_FORCEINLINE void PIN_nRESET_OUT(uint32_t bit)
{
    if (bit != 0U) {
        UF4_NRST_Release();
    } else {
        UF4_NRST_Assert();
    }
}

__STATIC_INLINE void LED_CONNECTED_OUT(uint32_t bit)
{
    LED_SetState((bit != 0U) ? LED_DAP_CONNECTED : LED_USB_CONNECTED);
}

__STATIC_INLINE void LED_RUNNING_OUT(uint32_t bit)
{
    LED_SetState((bit != 0U) ? LED_PROGRAMMING : LED_DAP_CONNECTED);
}

__STATIC_INLINE uint32_t TIMESTAMP_GET(void)
{
    return 0U;
}

__STATIC_INLINE void DAP_SETUP(void)
{
    PORT_OFF();
    LED_SetState(LED_USB_CONNECTED);
}

__STATIC_INLINE uint8_t RESET_TARGET(void)
{
    Target_Reset();
    return 1U;
}

#endif /* __DAP_CONFIG_H__ */
