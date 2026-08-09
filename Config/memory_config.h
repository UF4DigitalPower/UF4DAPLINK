#ifndef MEMORY_CONFIG_H
#define MEMORY_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define DAP_PACKET_SIZE                 64u
#define DAP_PACKET_COUNT                4u

#define CDC_BRIDGE_COUNT                2u
#define CDC_USB_RX_CHUNK_SIZE           64u
#define CDC_USB_TX_CHUNK_SIZE           64u

#define UART_RX_DMA_BUFFER_SIZE         256u
#define UART_RX_RING_BUFFER_SIZE        1024u
#define UART_TX_RING_BUFFER_SIZE        1024u
#define UART_TX_DMA_BUFFER_SIZE         128u

#define USB_CDC_EP_BUFSIZE              64u
#define USB_HID_EP_BUFSIZE              DAP_PACKET_SIZE

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_CONFIG_H */
