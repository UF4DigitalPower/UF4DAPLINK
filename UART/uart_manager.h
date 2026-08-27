#ifndef UART_MANAGER_H
#define UART_MANAGER_H

#include <stdbool.h>
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

typedef struct
{
    uint32_t usb_to_uart_bytes;
    uint32_t usb_to_uart_dropped;
    uint32_t uart_tx_bytes;
    uint32_t uart_tx_errors;
    uint32_t uart_rx_bytes;
    uint32_t uart_rx_dropped;
    uint32_t usb_from_uart_bytes;
    uint32_t uart_rx_start_errors;
} uart_bridge_stats_t;

#define UART_RX_SAMPLE_SIZE             256u

/** Initialize UART bridge ring buffers and start circular DMA reception. */
void UART_Manager_Init(void);

/** Non-blocking UART pump for TX and RX restart. */
void UART_Manager_Task(void);

/** Queue bytes received from USB CDC for UART TX DMA. */
size_t UART_Manager_WriteFromUsb(uart_bridge_id_t id, const uint8_t *data, size_t len);

/** Read and remove bytes received from UART for USB CDC TX. */
size_t UART_Manager_ReadForUsb(uart_bridge_id_t id, uint8_t *data, size_t len);

/** Return a contiguous UART-to-USB span without removing it from the queue. */
size_t UART_Manager_PeekForUsb(uart_bridge_id_t id, const uint8_t **data);

/** Remove bytes previously returned by UART_Manager_PeekForUsb(). */
void UART_Manager_ConsumeForUsb(uart_bridge_id_t id, size_t len);

/** Apply CDC line coding baud rate to the physical UART. */
bool UART_Manager_SetBaudRate(uart_bridge_id_t id, uint32_t baud_rate);

/** Abort active transfers, clear pending bridge bytes, and restart UART RX. */
bool UART_Manager_ResetBridge(uart_bridge_id_t id);

/** Return free bytes in the USB-to-UART transmit queue. */
size_t UART_Manager_GetTxFree(uart_bridge_id_t id);

/** Return free bytes in the UART-to-USB receive queue. */
size_t UART_Manager_GetRxFree(uart_bridge_id_t id);

/** Read debug counters for one UART bridge. */
bool UART_Manager_GetStats(uart_bridge_id_t id, uart_bridge_stats_t *stats);

/** Clear debug counters for one UART bridge. */
bool UART_Manager_ClearStats(uart_bridge_id_t id);

/** Read captured UART RX sample bytes since the last stats clear. */
size_t UART_Manager_GetRxSample(uart_bridge_id_t id, uint16_t offset, uint8_t *data, size_t len);

/** HAL callback hook for circular ReceiveToIdle DMA events. */
void UART_Manager_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size);

/** HAL callback hook for circular RX DMA completion. */
void UART_Manager_RxCpltCallback(UART_HandleTypeDef *huart);

/** HAL callback hook for interrupt-driven TX completion. */
void UART_Manager_TxCpltCallback(UART_HandleTypeDef *huart);

/** HAL callback hook for UART line or overrun errors. */
void UART_Manager_ErrorCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* UART_MANAGER_H */
