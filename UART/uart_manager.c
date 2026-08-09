#include "uart_manager.h"
#include "ring_buffer.h"
#include "../BSP/board.h"
#include "../Config/memory_config.h"
#include <stdbool.h>

typedef struct
{
    UART_HandleTypeDef *huart;
    ring_buffer_t rx_ring;
    ring_buffer_t tx_ring;
    uint8_t rx_storage[UART_RX_RING_BUFFER_SIZE];
    uint8_t tx_storage[UART_TX_RING_BUFFER_SIZE];
    uint8_t rx_dma[UART_RX_DMA_BUFFER_SIZE];
    uint8_t tx_dma[UART_TX_DMA_BUFFER_SIZE];
    volatile uint16_t last_rx_size;
    volatile bool tx_busy;
    volatile bool rx_running;
} uart_bridge_t;

static uart_bridge_t s_uart[UART_BRIDGE_COUNT];

static uart_bridge_id_t UART_IdFromHandle(UART_HandleTypeDef *huart, bool *ok)
{
    for (uint8_t i = 0u; i < UART_BRIDGE_COUNT; i++) {
        if (s_uart[i].huart == huart) {
            *ok = true;
            return (uart_bridge_id_t)i;
        }
    }

    *ok = false;
    return UART_BRIDGE_G474;
}

static void UART_StartRx(uart_bridge_t *bridge)
{
    if (bridge->huart == 0) {
        return;
    }

    if (HAL_UARTEx_ReceiveToIdle_DMA(bridge->huart, bridge->rx_dma, UART_RX_DMA_BUFFER_SIZE) == HAL_OK) {
        __HAL_DMA_DISABLE_IT(bridge->huart->hdmarx, DMA_IT_HT);
        bridge->rx_running = true;
        bridge->last_rx_size = 0u;
    }
}

static void UART_PumpTx(uart_bridge_t *bridge)
{
    size_t len;

    if (bridge->tx_busy || (bridge->huart == 0)) {
        return;
    }

    len = RingBuffer_Read(&bridge->tx_ring, bridge->tx_dma, UART_TX_DMA_BUFFER_SIZE);
    if (len == 0u) {
        return;
    }

    bridge->tx_busy = true;
    if (HAL_UART_Transmit_IT(bridge->huart, bridge->tx_dma, (uint16_t)len) != HAL_OK) {
        bridge->tx_busy = false;
    }
}

void UART_Manager_Init(void)
{
    s_uart[UART_BRIDGE_G474].huart = &huart_g474;
    s_uart[UART_BRIDGE_F429].huart = &huart_f429;
    s_uart[UART_BRIDGE_ESP32].huart = &huart_esp32;

    for (uint8_t i = 0u; i < UART_BRIDGE_COUNT; i++) {
        RingBuffer_Init(&s_uart[i].rx_ring, s_uart[i].rx_storage, UART_RX_RING_BUFFER_SIZE);
        RingBuffer_Init(&s_uart[i].tx_ring, s_uart[i].tx_storage, UART_TX_RING_BUFFER_SIZE);
        s_uart[i].tx_busy = false;
        s_uart[i].rx_running = false;
        UART_StartRx(&s_uart[i]);
    }
}

void UART_Manager_Task(void)
{
    for (uint8_t i = 0u; i < UART_BRIDGE_COUNT; i++) {
        if (!s_uart[i].rx_running) {
            UART_StartRx(&s_uart[i]);
        }
        UART_PumpTx(&s_uart[i]);
    }
}

size_t UART_Manager_WriteFromUsb(uart_bridge_id_t id, const uint8_t *data, size_t len)
{
    if (id >= UART_BRIDGE_COUNT) {
        return 0u;
    }

    return RingBuffer_Write(&s_uart[id].tx_ring, data, len);
}

size_t UART_Manager_ReadForUsb(uart_bridge_id_t id, uint8_t *data, size_t len)
{
    if (id >= UART_BRIDGE_COUNT) {
        return 0u;
    }

    return RingBuffer_Read(&s_uart[id].rx_ring, data, len);
}

void UART_Manager_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    bool ok;
    uart_bridge_id_t id = UART_IdFromHandle(huart, &ok);
    uart_bridge_t *bridge;

    if (!ok || (size > UART_RX_DMA_BUFFER_SIZE)) {
        return;
    }

    bridge = &s_uart[id];
    if (size >= bridge->last_rx_size) {
        (void)RingBuffer_Write(&bridge->rx_ring,
                               &bridge->rx_dma[bridge->last_rx_size],
                               (size_t)(size - bridge->last_rx_size));
    } else {
        (void)RingBuffer_Write(&bridge->rx_ring,
                               &bridge->rx_dma[bridge->last_rx_size],
                               (size_t)(UART_RX_DMA_BUFFER_SIZE - bridge->last_rx_size));
        if (size > 0u) {
            (void)RingBuffer_Write(&bridge->rx_ring, bridge->rx_dma, size);
        }
    }

    bridge->last_rx_size = size;
}

void UART_Manager_TxCpltCallback(UART_HandleTypeDef *huart)
{
    bool ok;
    uart_bridge_id_t id = UART_IdFromHandle(huart, &ok);

    if (ok) {
        s_uart[id].tx_busy = false;
    }
}
