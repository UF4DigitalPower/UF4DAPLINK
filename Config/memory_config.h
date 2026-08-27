#ifndef MEMORY_CONFIG_H
#define MEMORY_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define DAP_PACKET_SIZE                 64u
#define DAP_PACKET_COUNT                4u

#define CDC_BRIDGE_COUNT                2u
#define CDC_USB_RX_CHUNK_SIZE           64u
#define CDC_USB_TX_CHUNK_SIZE           256u
#define CDC_USB_RX_FIFO_SIZE            2048u
#define CDC_USB_TX_FIFO_SIZE            8192u

#define UART_RX_DMA_BUFFER_SIZE         1024u
#define UART_TARGET_RX_RING_BUFFER_SIZE 4096u
#define UART_TARGET_TX_RING_BUFFER_SIZE 512u
#define UART_ESP32_RX_RING_BUFFER_SIZE  16384u
#define UART_ESP32_TX_RING_BUFFER_SIZE  4096u
#define UART_TX_DMA_BUFFER_SIZE         256u

#define USB_CDC_EP_BUFSIZE              64u
#define USB_HID_EP_BUFSIZE              DAP_PACKET_SIZE

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_CONFIG_H */
