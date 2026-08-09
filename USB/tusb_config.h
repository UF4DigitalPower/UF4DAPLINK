#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#include "stm32f4xx_hal.h"
#include "../Config/memory_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUSB_MCU                    OPT_MCU_STM32F4
#define CFG_TUSB_RHPORT0_MODE           (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUSB_OS                     OPT_OS_NONE

#define CFG_TUD_ENABLED                 1
#define CFG_TUD_ENDPOINT0_SIZE          64

#define CFG_TUD_HID                     1
#define CFG_TUD_CDC                     2
#define CFG_TUD_MSC                     0
#define CFG_TUD_VENDOR                  0

#define CFG_TUD_HID_EP_BUFSIZE          USB_HID_EP_BUFSIZE
#define CFG_TUD_CDC_RX_BUFSIZE          UART_RX_RING_BUFFER_SIZE
#define CFG_TUD_CDC_TX_BUFSIZE          UART_TX_RING_BUFFER_SIZE
#define CFG_TUD_CDC_EP_BUFSIZE          USB_CDC_EP_BUFSIZE

#ifdef __cplusplus
}
#endif

#endif /* TUSB_CONFIG_H */
