#include "usb_composite.h"
#include "usb_desc.h"
#include <string.h>

#define RING_SIZE 1024u
#define CDC1_OUT_EP 2u
#define CDC2_OUT_EP 4u
#define CDC3_OUT_EP 6u

typedef struct {
    USART_TypeDef *usart;
    uint8_t *rx_usb;
    uint8_t *tx_usb;
    uint8_t rx_ring[RING_SIZE];
    uint8_t tx_ring[RING_SIZE];
    volatile uint16_t rx_head, rx_tail;
    volatile uint16_t tx_head, tx_tail;
    uint8_t line[7];
    uint8_t line_dirty;
} CDC_PORT;

__attribute__((aligned(4))) static uint8_t cdc_rx[USB_CDC_COUNT][DEF_USBD_HS_PACK_SIZE];
__attribute__((aligned(4))) static uint8_t cdc_tx[USB_CDC_COUNT][DEF_USBD_HS_PACK_SIZE];
__attribute__((aligned(4))) static uint8_t dap_tx[DEF_USBD_HS_PACK_SIZE];
__attribute__((aligned(4))) static uint8_t dap_rx[DEF_USBD_HS_PACK_SIZE];

static CDC_PORT cdc[USB_CDC_COUNT] = {
    {USART1, cdc_rx[0], cdc_tx[0], {0}, {0}, 0,0,0,0, {0x00,0xC2,0x01,0x00,0,0,8}, 0},
    {USART3, cdc_rx[1], cdc_tx[1], {0}, {0}, 0,0,0,0, {0x00,0xC2,0x01,0x00,0,0,8}, 0},
    {UART8,  cdc_rx[2], cdc_tx[2], {0}, {0}, 0,0,0,0, {0x00,0xC2,0x01,0x00,0,0,8}, 0}
};

static uint8_t ep_to_cdc(uint8_t ep) {
    if (ep == CDC1_OUT_EP) return 0;
    if (ep == CDC2_OUT_EP) return 1;
    if (ep == CDC3_OUT_EP) return 2;
    return 0xFF;
}

static uint8_t cdc_in_ep(uint8_t n)  { return (uint8_t)(CDC1_OUT_EP + n * 2u); }

static uint16_t ring_count(volatile uint16_t head, volatile uint16_t tail) {
    return (uint16_t)((head - tail) & (RING_SIZE - 1u));
}
static void ring_put(CDC_PORT *p, const uint8_t *data, uint16_t len) {
    while (len--) {
        uint16_t next = (uint16_t)((p->tx_head + 1u) & (RING_SIZE - 1u));
        if (next == p->tx_tail) break;
        p->tx_ring[p->tx_head] = *data++;
        p->tx_head = next;
    }
}

static void uart_apply_line(CDC_PORT *p) {
    USART_InitTypeDef init = {0};
    uint32_t baud = (uint32_t)p->line[0] | ((uint32_t)p->line[1] << 8) |
                    ((uint32_t)p->line[2] << 16) | ((uint32_t)p->line[3] << 24);
    if (baud == 0) baud = 115200;
    init.USART_BaudRate = baud;
    init.USART_WordLength = (p->line[6] == 5 || p->line[6] == 6 || p->line[6] == 7) ? USART_WordLength_8b : USART_WordLength_8b;
    init.USART_StopBits = (p->line[4] == 2) ? USART_StopBits_2 : USART_StopBits_1;
    init.USART_Parity = (p->line[5] == 1) ? USART_Parity_Odd : ((p->line[5] == 2) ? USART_Parity_Even : USART_Parity_No);
    init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(p->usart, &init);
    USART_Cmd(p->usart, ENABLE);
    p->line_dirty = 0;
}

static void configure_uart_pins(void) {
    GPIO_InitTypeDef g = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3 | RCC_APB1Periph_UART8, ENABLE);

    g.GPIO_Speed = GPIO_Speed_50MHz;
    g.GPIO_Mode = GPIO_Mode_AF_PP;
    g.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOA, &g);
    g.GPIO_Pin = GPIO_Pin_10; g.GPIO_Mode = GPIO_Mode_IPU; GPIO_Init(GPIOA, &g);
    g.GPIO_Mode = GPIO_Mode_AF_PP; g.GPIO_Pin = GPIO_Pin_10; GPIO_Init(GPIOB, &g);
    g.GPIO_Pin = GPIO_Pin_11; g.GPIO_Mode = GPIO_Mode_IPU; GPIO_Init(GPIOB, &g);
    g.GPIO_Mode = GPIO_Mode_AF_PP; g.GPIO_Pin = GPIO_Pin_4; GPIO_Init(GPIOC, &g);
    g.GPIO_Pin = GPIO_Pin_5; g.GPIO_Mode = GPIO_Mode_IPU; GPIO_Init(GPIOC, &g);
}

void USBComposite_Init(void) {
    configure_uart_pins();
    for (uint8_t i = 0; i < USB_CDC_COUNT; ++i) uart_apply_line(&cdc[i]);
}

void USBComposite_EndpointInit(void) {
    USBHSD->UEP1_TX_DMA = (uint32_t)cdc_tx[0];
    USBHSD->UEP2_RX_DMA = (uint32_t)cdc_rx[0]; USBHSD->UEP2_TX_DMA = (uint32_t)cdc_tx[0];
    USBHSD->UEP3_TX_DMA = (uint32_t)cdc_tx[1];
    USBHSD->UEP4_RX_DMA = (uint32_t)cdc_rx[1]; USBHSD->UEP4_TX_DMA = (uint32_t)cdc_tx[1];
    USBHSD->UEP5_TX_DMA = (uint32_t)cdc_tx[2];
    USBHSD->UEP6_RX_DMA = (uint32_t)cdc_rx[2]; USBHSD->UEP6_TX_DMA = (uint32_t)cdc_tx[2];
    USBHSD->UEP7_RX_DMA = (uint32_t)dap_rx; USBHSD->UEP7_TX_DMA = (uint32_t)dap_tx;
}

void USBComposite_OnOut(uint8_t ep, uint16_t len) {
    uint8_t n = ep_to_cdc(ep);
    if (n != 0xFF) {
        CDC_PORT *p = &cdc[n];
        ring_put(p, USBHSD_UEP_RXBUF(ep), len);
        USBHSD_UEP_RXDMA(ep) = (uint32_t)p->rx_usb;
        USBHSD_UEP_RXCTRL(ep) ^= USBHS_UEP_R_TOG_DATA1;
        USBHSD_UEP_RXCTRL(ep) = (USBHSD_UEP_RXCTRL(ep) & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_ACK;
        return;
    }
    if (ep == USB_DAP_EP) {
        const uint8_t *in = USBHSD_UEP_RXBUF(ep);
        uint16_t out_len = 0;
        if (len) {
            uint8_t cmd = in[0];
            dap_tx[0] = cmd;
            if (cmd == 0x00 && len >= 2) {
                static const char vendor[] = "UF4 Research";
                static const char product[] = "UF4 DAPLink";
                static const char serial[] = "0123456789";
                uint8_t id = in[1]; const char *s = 0;
                if (id == 1) s = vendor; else if (id == 2) s = product; else if (id == 3) s = serial;
                if (s) { uint8_t l = (uint8_t)strlen(s); dap_tx[1] = l; memcpy(&dap_tx[2], s, l); out_len = (uint16_t)(2 + l); }
                else if (id == 4) { uint16_t packet = USBHS_DevMaxPackLen ? USBHS_DevMaxPackLen : DEF_USBD_HS_PACK_SIZE; dap_tx[1] = 2; dap_tx[2] = (uint8_t)packet; dap_tx[3] = (uint8_t)(packet >> 8); out_len = 4; }
                else if (id == 5) { dap_tx[1] = 3; memcpy(&dap_tx[2], "2.1", 3); out_len = 5; }
                else { dap_tx[1] = 0; out_len = 2; }
            } else if (cmd == 0x02) { dap_tx[1] = 1; out_len = 2; }
            else if (cmd == 0x03 || cmd == 0x04 || cmd == 0x1A) { dap_tx[1] = 0; out_len = 2; }
            else if (cmd == 0x05) { dap_tx[1] = 0; out_len = 2; }
            else { dap_tx[1] = 0; out_len = 2; }
        }
        if (out_len) USBHS_Endp_DataUp(USB_DAP_EP, dap_tx, out_len, DEF_UEP_DMA_LOAD);
        USBHSD_UEP_RXDMA(ep) = (uint32_t)dap_rx;
        USBHSD_UEP_RXCTRL(ep) ^= USBHS_UEP_R_TOG_DATA1;
        USBHSD_UEP_RXCTRL(ep) = (USBHSD_UEP_RXCTRL(ep) & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_ACK;
    }
}

void USBComposite_OnIn(uint8_t ep) {
    for (uint8_t i = 0; i < USB_CDC_COUNT; ++i) if (cdc_in_ep(i) == ep) {
        USBHS_Endp_Busy[ep] &= (uint8_t)~DEF_UEP_BUSY;
        USBHSD_UEP_TXCTRL(ep) = (USBHSD_UEP_TXCTRL(ep) & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_NAK;
        USBHSD_UEP_TXCTRL(ep) ^= USBHS_UEP_T_TOG_DATA1;
    }
    if (ep == USB_DAP_EP) {
        USBHS_Endp_Busy[ep] &= (uint8_t)~DEF_UEP_BUSY;
        USBHSD_UEP_TXCTRL(ep) = (USBHSD_UEP_TXCTRL(ep) & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_NAK;
        USBHSD_UEP_TXCTRL(ep) ^= USBHS_UEP_T_TOG_DATA1;
    }
}

void USBComposite_Poll(void) {
    for (uint8_t i = 0; i < USB_CDC_COUNT; ++i) {
        CDC_PORT *p = &cdc[i];
        if (p->line_dirty) uart_apply_line(p);
        while (USART_GetFlagStatus(p->usart, USART_FLAG_RXNE) != RESET) {
            uint16_t next = (uint16_t)((p->rx_head + 1u) & (RING_SIZE - 1u));
            uint8_t b = (uint8_t)USART_ReceiveData(p->usart);
            if (next != p->rx_tail) { p->rx_ring[p->rx_head] = b; p->rx_head = next; }
        }
        while (p->tx_tail != p->tx_head && USART_GetFlagStatus(p->usart, USART_FLAG_TXE) != RESET) {
            USART_SendData(p->usart, p->tx_ring[p->tx_tail]);
            p->tx_tail = (uint16_t)((p->tx_tail + 1u) & (RING_SIZE - 1u));
        }
        uint8_t ep = cdc_in_ep(i);
        if (USBHS_Endp_Busy[ep] == 0 && p->rx_tail != p->rx_head) {
            uint16_t n = ring_count(p->rx_head, p->rx_tail); if (n > DEF_USBD_HS_PACK_SIZE) n = DEF_USBD_HS_PACK_SIZE;
            for (uint16_t j = 0; j < n; ++j) { cdc[i].tx_usb[j] = p->rx_ring[p->rx_tail]; p->rx_tail = (uint16_t)((p->rx_tail + 1u) & (RING_SIZE - 1u)); }
            USBHS_Endp_DataUp(ep, cdc[i].tx_usb, n, DEF_UEP_DMA_LOAD);
        }
    }
}

uint8_t USBComposite_ClassRequest(uint8_t request_type, uint8_t request, uint16_t value,
                                  uint16_t index, uint16_t length, const uint8_t **data, uint16_t *data_len) {
    uint8_t n;
    if ((request_type & USB_REQ_TYP_MASK) != USB_REQ_TYP_CLASS) return 0;
    if (index == 0 || index == 2 || index == 4) n = (uint8_t)(index / 2); else return 0;
    if (request == CDC_GET_LINE_CODING && (request_type & DEF_UEP_IN)) {
        *data = cdc[n].line; *data_len = (length < 7) ? length : 7; return 1;
    }
    if (request == CDC_SET_LINE_CODING && !(request_type & DEF_UEP_IN) && length == 7) { return 1; }
    if (request == CDC_SET_LINE_CTLSTE || request == CDC_SEND_BREAK) return 1;
    (void)value; return 0;
}

void USBComposite_ClassOut(uint8_t request, uint16_t index, const uint8_t *data, uint16_t len) {
    if (request == CDC_SET_LINE_CODING && len >= 7 && (index == 0 || index == 2 || index == 4)) {
        uint8_t n = (uint8_t)(index / 2); memcpy(cdc[n].line, data, 7); cdc[n].line_dirty = 1;
    }
}

uint8_t USBComposite_VendorRequest(uint8_t request_type, uint8_t request, uint16_t value,
                                   uint16_t index, uint16_t length, const uint8_t **data, uint16_t *data_len) {
    if ((request_type & USB_REQ_TYP_MASK) != USB_REQ_TYP_VENDOR || !(request_type & DEF_UEP_IN)) return 0;
    if (request == 0x22 && index == 0x0002 && value == 1) { *data = USB_WebUSB_URL; *data_len = (length < USB_WebUSB_URL[0]) ? length : USB_WebUSB_URL[0]; return 1; }
    if (request == 0x21 && index == 0x0007) { *data = USB_MS_OS_20_Desc; *data_len = (length < 46) ? length : 46; return 1; }
    return 0;
}
