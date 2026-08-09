#include "cdc_bridge.h"
#include "tusb.h"
#include "../Config/memory_config.h"
#include "../UART/uart_manager.h"

static uint8_t s_usb_rx[CDC_BRIDGE_COUNT][CDC_USB_RX_CHUNK_SIZE];
static uint8_t s_usb_tx[CDC_BRIDGE_COUNT][CDC_USB_TX_CHUNK_SIZE];

void CDC_Bridge_Init(void)
{
}

void CDC_Task(void)
{
    for (uint8_t itf = 0u; itf < CDC_BRIDGE_COUNT; itf++) {
        uint32_t available = tud_cdc_n_available(itf);

        while (available > 0u) {
            uint32_t chunk = available;

            if (chunk > CDC_USB_RX_CHUNK_SIZE) {
                chunk = CDC_USB_RX_CHUNK_SIZE;
            }

            chunk = tud_cdc_n_read(itf, s_usb_rx[itf], chunk);
            if (chunk == 0u) {
                break;
            }

            (void)UART_Manager_WriteFromUsb((uart_bridge_id_t)itf, s_usb_rx[itf], chunk);
            available = tud_cdc_n_available(itf);
        }

        while (tud_cdc_n_write_available(itf) > 0u) {
            uint32_t writable = tud_cdc_n_write_available(itf);
            size_t len;

            if (writable > CDC_USB_TX_CHUNK_SIZE) {
                writable = CDC_USB_TX_CHUNK_SIZE;
            }

            len = UART_Manager_ReadForUsb((uart_bridge_id_t)itf, s_usb_tx[itf], writable);
            if (len == 0u) {
                break;
            }

            (void)tud_cdc_n_write(itf, s_usb_tx[itf], (uint32_t)len);
            (void)tud_cdc_n_write_flush(itf);
        }
    }
}
