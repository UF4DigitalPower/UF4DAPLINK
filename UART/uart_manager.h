#ifndef UART_MANAGER_H
#define UART_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    UART_BRIDGE_G474 = 0,
    UART_BRIDGE_F429,
    UART_BRIDGE_ESP32,
    UART_BRIDGE_COUNT
} uart_bridge_id_t;

/** Initialize UART bridge ring buffers and start RX DMA-to-idle. */
void UART_Manager_Init(void);

/** Non-blocking UART pump for TX DMA and RX DMA restart. */
void UART_Manager_Task(void);

/** Queue bytes received from USB CDC for UART TX DMA. */
size_t UART_Manager_WriteFromUsb(uart_bridge_id_t id, const uint8_t *data, size_t len);

/** Read bytes received from UART for USB CDC TX. */
size_t UART_Manager_ReadForUsb(uart_bridge_id_t id, uint8_t *data, size_t len);

/** HAL callback hook for ReceiveToIdle DMA events. */
void UART_Manager_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size);

/** HAL callback hook for TX DMA completion. */
void UART_Manager_TxCpltCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* UART_MANAGER_H */
