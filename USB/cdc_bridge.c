#include "cdc_bridge.h"
#include "tusb.h"
#include "../Config/memory_config.h"
#include "../DAP/target.h"
#include "../UART/uart_manager.h"
#include <stdbool.h>

static uint8_t s_usb_rx[CDC_BRIDGE_COUNT][CDC_USB_RX_CHUNK_SIZE];
static uint8_t s_usb_tx[CDC_BRIDGE_COUNT][CDC_USB_TX_CHUNK_SIZE];

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

void CDC_Task(void)
{
    for (uint8_t itf = 0u; itf < CDC_BRIDGE_COUNT; itf++) {
        uint32_t available = tud_cdc_n_available(itf);
        uart_bridge_id_t uart_id;
        bool uart_valid = CDC_Bridge_GetUartId(itf, &uart_id);

        while (available > 0u) {
            uint32_t chunk = available;

            if (chunk > CDC_USB_RX_CHUNK_SIZE) {
                chunk = CDC_USB_RX_CHUNK_SIZE;
            }

            chunk = tud_cdc_n_read(itf, s_usb_rx[itf], chunk);
            if (chunk == 0u) {
                break;
            }

            if (uart_valid) {
                (void)UART_Manager_WriteFromUsb(uart_id, s_usb_rx[itf], chunk);
            }
            available = tud_cdc_n_available(itf);
        }

        if (!uart_valid) {
            continue;
        }

        while (tud_cdc_n_write_available(itf) > 0u) {
            uint32_t writable = tud_cdc_n_write_available(itf);
            size_t len;

            if (writable > CDC_USB_TX_CHUNK_SIZE) {
                writable = CDC_USB_TX_CHUNK_SIZE;
            }

            len = UART_Manager_ReadForUsb(uart_id, s_usb_tx[itf], writable);
            if (len == 0u) {
                break;
            }

            (void)tud_cdc_n_write(itf, s_usb_tx[itf], (uint32_t)len);
            (void)tud_cdc_n_write_flush(itf);
        }
    }
}
