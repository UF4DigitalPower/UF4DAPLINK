#include "uart_manager.h"
#include "ring_buffer.h"
#include "../BSP/board.h"
#include "../Config/memory_config.h"
#include <stdbool.h>
#include <string.h>

typedef struct
{
    UART_HandleTypeDef *huart;
    ring_buffer_t rx_ring;
    ring_buffer_t tx_ring;
    uint8_t *rx_storage;
    size_t rx_storage_size;
    uint8_t *tx_storage;
    size_t tx_storage_size;
    uint8_t rx_dma[UART_RX_DMA_BUFFER_SIZE];
    size_t rx_dma_pos;
    uint8_t tx_dma[UART_TX_DMA_BUFFER_SIZE];
    volatile bool tx_busy;
    volatile bool rx_running;
    uart_bridge_stats_t stats;
    uint8_t rx_sample[UART_RX_SAMPLE_SIZE];
    size_t rx_sample_count;
} uart_bridge_t;

static uart_bridge_t s_uart[UART_BRIDGE_COUNT];
static uint8_t s_g474_rx_storage[UART_TARGET_RX_RING_BUFFER_SIZE];
static uint8_t s_g474_tx_storage[UART_TARGET_TX_RING_BUFFER_SIZE];
static uint8_t s_f429_rx_storage[UART_TARGET_RX_RING_BUFFER_SIZE];
static uint8_t s_f429_tx_storage[UART_TARGET_TX_RING_BUFFER_SIZE];
static uint8_t s_esp32_rx_storage[UART_ESP32_RX_RING_BUFFER_SIZE];
static uint8_t s_esp32_tx_storage[UART_ESP32_TX_RING_BUFFER_SIZE];

static uint32_t UART_EnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    return primask;
}

static void UART_ExitCritical(uint32_t primask)
{
    __set_PRIMASK(primask);
}

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

static void UART_RecordRxSample(uart_bridge_t *bridge, const uint8_t *data, size_t len)
{
    size_t room = UART_RX_SAMPLE_SIZE - bridge->rx_sample_count;

    if ((room == 0u) || (data == 0) || (len == 0u)) {
        return;
    }

    if (len > room) {
        len = room;
    }

    memcpy(&bridge->rx_sample[bridge->rx_sample_count], data, len);
    bridge->rx_sample_count += len;
}

static void UART_StoreRxDmaSpan(uart_bridge_t *bridge, size_t start, size_t end)
{
    size_t len;
    size_t written;

    if ((end <= start) || (end > UART_RX_DMA_BUFFER_SIZE)) {
        return;
    }

    len = end - start;
    UART_RecordRxSample(bridge, &bridge->rx_dma[start], len);
    written = RingBuffer_Write(&bridge->rx_ring, &bridge->rx_dma[start], len);
    bridge->stats.uart_rx_bytes += (uint32_t)written;
    bridge->stats.uart_rx_dropped += (uint32_t)(len - written);
}

/* Caller holds the UART manager critical section. */
static void UART_CaptureRxDmaUntilLocked(uart_bridge_t *bridge, size_t pos)
{
    if ((bridge->huart == 0) || !bridge->rx_running || (pos > UART_RX_DMA_BUFFER_SIZE)) {
        return;
    }

    if (pos == UART_RX_DMA_BUFFER_SIZE) {
        /* HT always precedes TC, so a zero cursor means this full span was reported already. */
        if (bridge->rx_dma_pos != 0u) {
            UART_StoreRxDmaSpan(bridge, bridge->rx_dma_pos, pos);
        }
        bridge->rx_dma_pos = 0u;
    } else if (pos > bridge->rx_dma_pos) {
        UART_StoreRxDmaSpan(bridge, bridge->rx_dma_pos, pos);
    } else if (pos < bridge->rx_dma_pos) {
        UART_StoreRxDmaSpan(bridge, bridge->rx_dma_pos, UART_RX_DMA_BUFFER_SIZE);
        UART_StoreRxDmaSpan(bridge, 0u, pos);
    }
    if (pos != UART_RX_DMA_BUFFER_SIZE) {
        bridge->rx_dma_pos = pos;
    }
}

static void UART_CaptureRxDmaUntil(uart_bridge_t *bridge, size_t pos)
{
    uint32_t primask = UART_EnterCritical();

    UART_CaptureRxDmaUntilLocked(bridge, pos);
    UART_ExitCritical(primask);
}

static void UART_CaptureRxDma(uart_bridge_t *bridge)
{
    size_t pos;
    uint32_t primask;

    primask = UART_EnterCritical();
    if ((bridge->huart == 0) || (bridge->huart->hdmarx == 0) || !bridge->rx_running) {
        UART_ExitCritical(primask);
        return;
    }

    pos = UART_RX_DMA_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(bridge->huart->hdmarx);
    if (pos >= UART_RX_DMA_BUFFER_SIZE) {
        pos = 0u;
    }

    UART_CaptureRxDmaUntilLocked(bridge, pos);
    UART_ExitCritical(primask);
}

static void UART_StartRx(uart_bridge_t *bridge)
{
    if (bridge->huart == 0) {
        return;
    }

    bridge->rx_dma_pos = 0u;
    if (HAL_UARTEx_ReceiveToIdle_DMA(bridge->huart, bridge->rx_dma, UART_RX_DMA_BUFFER_SIZE) == HAL_OK) {
        bridge->rx_running = true;
    } else {
        bridge->stats.uart_rx_start_errors++;
    }
}

static void UART_ClearQueues(uart_bridge_t *bridge)
{
    uint32_t primask = UART_EnterCritical();

    RingBuffer_Init(&bridge->rx_ring, bridge->rx_storage, bridge->rx_storage_size);
    RingBuffer_Init(&bridge->tx_ring, bridge->tx_storage, bridge->tx_storage_size);
    bridge->tx_busy = false;
    UART_ExitCritical(primask);
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
    if (HAL_UART_Transmit_DMA(bridge->huart, bridge->tx_dma, (uint16_t)len) != HAL_OK) {
        bridge->tx_busy = false;
        bridge->stats.uart_tx_errors++;
    } else {
        bridge->stats.uart_tx_bytes += (uint32_t)len;
    }
}

void UART_Manager_Init(void)
{
    s_uart[UART_BRIDGE_G474].huart = &huart_g474;
    s_uart[UART_BRIDGE_F429].huart = &huart_f429;
    s_uart[UART_BRIDGE_ESP32].huart = &huart_esp32;

    s_uart[UART_BRIDGE_G474].rx_storage = s_g474_rx_storage;
    s_uart[UART_BRIDGE_G474].rx_storage_size = UART_TARGET_RX_RING_BUFFER_SIZE;
    s_uart[UART_BRIDGE_G474].tx_storage = s_g474_tx_storage;
    s_uart[UART_BRIDGE_G474].tx_storage_size = UART_TARGET_TX_RING_BUFFER_SIZE;

    s_uart[UART_BRIDGE_F429].rx_storage = s_f429_rx_storage;
    s_uart[UART_BRIDGE_F429].rx_storage_size = UART_TARGET_RX_RING_BUFFER_SIZE;
    s_uart[UART_BRIDGE_F429].tx_storage = s_f429_tx_storage;
    s_uart[UART_BRIDGE_F429].tx_storage_size = UART_TARGET_TX_RING_BUFFER_SIZE;

    s_uart[UART_BRIDGE_ESP32].rx_storage = s_esp32_rx_storage;
    s_uart[UART_BRIDGE_ESP32].rx_storage_size = UART_ESP32_RX_RING_BUFFER_SIZE;
    s_uart[UART_BRIDGE_ESP32].tx_storage = s_esp32_tx_storage;
    s_uart[UART_BRIDGE_ESP32].tx_storage_size = UART_ESP32_TX_RING_BUFFER_SIZE;

    for (uint8_t i = 0u; i < UART_BRIDGE_COUNT; i++) {
        RingBuffer_Init(&s_uart[i].rx_ring, s_uart[i].rx_storage, s_uart[i].rx_storage_size);
        RingBuffer_Init(&s_uart[i].tx_ring, s_uart[i].tx_storage, s_uart[i].tx_storage_size);
        s_uart[i].tx_busy = false;
        s_uart[i].rx_running = false;
        memset(&s_uart[i].stats, 0, sizeof(s_uart[i].stats));
        s_uart[i].rx_sample_count = 0u;
        UART_StartRx(&s_uart[i]);
    }
}

void UART_Manager_Task(void)
{
    for (uint8_t i = 0u; i < UART_BRIDGE_COUNT; i++) {
        if (!s_uart[i].rx_running) {
            UART_StartRx(&s_uart[i]);
        }
        UART_CaptureRxDma(&s_uart[i]);
        UART_PumpTx(&s_uart[i]);
    }
}

size_t UART_Manager_WriteFromUsb(uart_bridge_id_t id, const uint8_t *data, size_t len)
{
    size_t written;
    uint32_t primask;

    if (id >= UART_BRIDGE_COUNT) {
        return 0u;
    }

    primask = UART_EnterCritical();
    written = RingBuffer_Write(&s_uart[id].tx_ring, data, len);
    s_uart[id].stats.usb_to_uart_bytes += (uint32_t)written;
    s_uart[id].stats.usb_to_uart_dropped += (uint32_t)(len - written);
    UART_ExitCritical(primask);
    return written;
}

size_t UART_Manager_ReadForUsb(uart_bridge_id_t id, uint8_t *data, size_t len)
{
    size_t read;
    uint32_t primask;

    if (id >= UART_BRIDGE_COUNT) {
        return 0u;
    }

    primask = UART_EnterCritical();
    read = RingBuffer_Read(&s_uart[id].rx_ring, data, len);
    s_uart[id].stats.usb_from_uart_bytes += (uint32_t)read;
    UART_ExitCritical(primask);
    return read;
}

size_t UART_Manager_PeekForUsb(uart_bridge_id_t id, const uint8_t **data)
{
    size_t len;
    uint32_t primask;

    if ((id >= UART_BRIDGE_COUNT) || (data == 0)) {
        return 0u;
    }

    primask = UART_EnterCritical();
    len = RingBuffer_PeekLinear(&s_uart[id].rx_ring, data);
    UART_ExitCritical(primask);
    return len;
}

void UART_Manager_ConsumeForUsb(uart_bridge_id_t id, size_t len)
{
    uint32_t primask;

    if (id >= UART_BRIDGE_COUNT) {
        return;
    }

    primask = UART_EnterCritical();
    RingBuffer_Drop(&s_uart[id].rx_ring, len);
    s_uart[id].stats.usb_from_uart_bytes += (uint32_t)len;
    UART_ExitCritical(primask);
}

bool UART_Manager_SetBaudRate(uart_bridge_id_t id, uint32_t baud_rate)
{
    uart_bridge_t *bridge;

    if ((id >= UART_BRIDGE_COUNT) || (baud_rate == 0u)) {
        return false;
    }

    bridge = &s_uart[id];
    if (bridge->huart == 0) {
        return false;
    }

    if (bridge->huart->Init.BaudRate == baud_rate) {
        return true;
    }

    (void)HAL_UART_Abort(bridge->huart);
    bridge->huart->Init.BaudRate = baud_rate;
    UART_ClearQueues(bridge);
    bridge->rx_running = false;
    bridge->rx_dma_pos = 0u;

    if (HAL_UART_Init(bridge->huart) != HAL_OK) {
        return false;
    }

    UART_StartRx(bridge);
    return bridge->rx_running;
}

bool UART_Manager_ResetBridge(uart_bridge_id_t id)
{
    uart_bridge_t *bridge;

    if (id >= UART_BRIDGE_COUNT) {
        return false;
    }

    bridge = &s_uart[id];
    if (bridge->huart == 0) {
        return false;
    }

    (void)HAL_UART_Abort(bridge->huart);
    UART_ClearQueues(bridge);
    bridge->rx_running = false;
    bridge->rx_dma_pos = 0u;
    UART_StartRx(bridge);

    return bridge->rx_running;
}

size_t UART_Manager_GetTxFree(uart_bridge_id_t id)
{
    if (id >= UART_BRIDGE_COUNT) {
        return 0u;
    }

    return RingBuffer_Free(&s_uart[id].tx_ring);
}

size_t UART_Manager_GetRxFree(uart_bridge_id_t id)
{
    if (id >= UART_BRIDGE_COUNT) {
        return 0u;
    }

    return RingBuffer_Free(&s_uart[id].rx_ring);
}

bool UART_Manager_GetStats(uart_bridge_id_t id, uart_bridge_stats_t *stats)
{
    if ((id >= UART_BRIDGE_COUNT) || (stats == 0)) {
        return false;
    }

    *stats = s_uart[id].stats;
    return true;
}

bool UART_Manager_ClearStats(uart_bridge_id_t id)
{
    if (id >= UART_BRIDGE_COUNT) {
        return false;
    }

    memset(&s_uart[id].stats, 0, sizeof(s_uart[id].stats));
    s_uart[id].rx_sample_count = 0u;
    return true;
}

size_t UART_Manager_GetRxSample(uart_bridge_id_t id, uint16_t offset, uint8_t *data, size_t len)
{
    size_t available;

    if ((id >= UART_BRIDGE_COUNT) || (data == 0) || (offset >= s_uart[id].rx_sample_count)) {
        return 0u;
    }

    available = s_uart[id].rx_sample_count - offset;
    if (len > available) {
        len = available;
    }

    memcpy(data, &s_uart[id].rx_sample[offset], len);
    return len;
}

void UART_Manager_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    bool ok;
    uart_bridge_id_t id = UART_IdFromHandle(huart, &ok);

    if (!ok) {
        return;
    }

    UART_CaptureRxDmaUntil(&s_uart[id], size);
}

void UART_Manager_RxCpltCallback(UART_HandleTypeDef *huart)
{
    bool ok;
    uart_bridge_id_t id = UART_IdFromHandle(huart, &ok);

    if (ok) {
        UART_CaptureRxDmaUntil(&s_uart[id], UART_RX_DMA_BUFFER_SIZE);
    }
}

void UART_Manager_TxCpltCallback(UART_HandleTypeDef *huart)
{
    bool ok;
    uart_bridge_id_t id = UART_IdFromHandle(huart, &ok);

    if (ok) {
        s_uart[id].tx_busy = false;
    }
}

void UART_Manager_ErrorCallback(UART_HandleTypeDef *huart)
{
    bool ok;
    uart_bridge_id_t id = UART_IdFromHandle(huart, &ok);
    uart_bridge_t *bridge;

    if (!ok) {
        return;
    }

    bridge = &s_uart[id];
    (void)HAL_UART_AbortReceive(huart);
    bridge->rx_running = false;
    bridge->stats.uart_rx_start_errors++;

    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    huart->ErrorCode = HAL_UART_ERROR_NONE;

    UART_StartRx(bridge);
}
