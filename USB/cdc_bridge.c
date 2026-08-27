#include "cdc_bridge.h"
#include "tusb.h"
#include "../Config/memory_config.h"
#include "../DAP/target.h"
#include "../ESP32/esp32.h"
#include "../UART/uart_manager.h"
#include <stdbool.h>

static uint8_t s_usb_rx[CDC_BRIDGE_COUNT][CDC_USB_RX_CHUNK_SIZE];
static uint8_t s_usb_rx_pending_offset[CDC_BRIDGE_COUNT];
static uint8_t s_usb_rx_pending_len[CDC_BRIDGE_COUNT];

static bool CDC_Bridge_GetUartId(uint8_t itf, uart_bridge_id_t *id)
{
    if (itf == 0u) {
        switch (Target_Get()) {
        case TARGET_G474:
            *id = UART_BRIDGE_G474;
            return true;
        case TARGET_F429:
            *id = UART_BRIDGE_F429;
            return true;
        default:
            return false;
        }
    }

    if (itf == 1u) {
        *id = UART_BRIDGE_ESP32;
        return true;
    }

    return false;
}

void CDC_Bridge_Init(void)
{
}

void tud_cdc_line_coding_cb(uint8_t itf, const cdc_line_coding_t *p_line_coding)
{
    uart_bridge_id_t uart_id;

    if ((p_line_coding == 0) || !CDC_Bridge_GetUartId(itf, &uart_id)) {
        return;
    }

    if ((p_line_coding->stop_bits != CDC_LINE_CODING_STOP_BITS_1) ||
        (p_line_coding->parity != CDC_LINE_CODING_PARITY_NONE) ||
        (p_line_coding->data_bits != 8u)) {
        return;
    }

    (void)UART_Manager_SetBaudRate(uart_id, p_line_coding->bit_rate);
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    if (itf == 1u) {
        ESP32_ApplySerialControl(dtr, rts);
    }
}

static void CDC_Bridge_PumpUartToUsb(uint8_t itf, uart_bridge_id_t uart_id)
{
    bool wrote_usb = false;

    (void)tud_cdc_n_write_flush(itf);

    while (tud_cdc_n_write_available(itf) > 0u) {
        uint32_t writable = tud_cdc_n_write_available(itf);
        const uint8_t *data;
        size_t len;
        uint32_t written;

        if (writable > CDC_USB_TX_CHUNK_SIZE) {
            writable = CDC_USB_TX_CHUNK_SIZE;
        }

        len = UART_Manager_PeekForUsb(uart_id, &data);
        if (len == 0u) {
            break;
        }

        if (len > writable) {
            len = writable;
        }

        written = tud_cdc_n_write(itf, data, (uint32_t)len);
        if (written == 0u) {
            break;
        }

        UART_Manager_ConsumeForUsb(uart_id, written);
        wrote_usb = true;
    }

    if (wrote_usb) {
        (void)tud_cdc_n_write_flush(itf);
    }
}

static void CDC_Bridge_PumpUsbToUart(uint8_t itf, uart_bridge_id_t uart_id)
{
    size_t written;

    while (true) {
        if (s_usb_rx_pending_offset[itf] < s_usb_rx_pending_len[itf]) {
            size_t remaining = s_usb_rx_pending_len[itf] - s_usb_rx_pending_offset[itf];

            written = UART_Manager_WriteFromUsb(uart_id,
                                                 &s_usb_rx[itf][s_usb_rx_pending_offset[itf]],
                                                 remaining);
            s_usb_rx_pending_offset[itf] += (uint8_t)written;
            if (s_usb_rx_pending_offset[itf] < s_usb_rx_pending_len[itf]) {
                return;
            }

            s_usb_rx_pending_offset[itf] = 0u;
            s_usb_rx_pending_len[itf] = 0u;
        }

        if (tud_cdc_n_available(itf) == 0u) {
            return;
        }

        written = UART_Manager_GetTxFree(uart_id);
        if (written == 0u) {
            return;
        }
        if (written > CDC_USB_RX_CHUNK_SIZE) {
            written = CDC_USB_RX_CHUNK_SIZE;
        }

        s_usb_rx_pending_len[itf] = (uint8_t)tud_cdc_n_read(itf, s_usb_rx[itf], (uint32_t)written);
        s_usb_rx_pending_offset[itf] = 0u;
        if (s_usb_rx_pending_len[itf] == 0u) {
            return;
        }
    }
}

void tud_cdc_tx_complete_cb(uint8_t itf)
{
    uart_bridge_id_t uart_id;

    if (CDC_Bridge_GetUartId(itf, &uart_id)) {
        CDC_Bridge_PumpUartToUsb(itf, uart_id);
    }
}

void CDC_Task(void)
{
    static const uint8_t itf_order[CDC_BRIDGE_COUNT] = { 1u, 0u };

    for (uint8_t order = 0u; order < CDC_BRIDGE_COUNT; order++) {
        uint8_t itf = itf_order[order];
        uart_bridge_id_t uart_id;
        bool uart_valid = CDC_Bridge_GetUartId(itf, &uart_id);

        if (uart_valid) {
            CDC_Bridge_PumpUartToUsb(itf, uart_id);
        }

        if (uart_valid) {
            CDC_Bridge_PumpUsbToUart(itf, uart_id);
        }
    }
}
