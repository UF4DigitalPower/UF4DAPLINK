/* USBHS DMA address registers cover only the first 64 KiB of SRAM. */
ASSERT(USBHS_Echo_Buffer >= 0x20000000 && USBHS_Echo_Buffer + 32768 <= 0x20010000,
       "USBHS echo buffer exceeds DMA SRAM address range")
ASSERT(USBHS_EP0_Buf >= 0x20000000 && USBHS_EP0_Buf + 64 <= 0x20010000,
       "USBHS EP0 buffer exceeds DMA SRAM address range")
ASSERT(USBHS_EP3_Tx_Buf >= 0x20000000 && USBHS_EP3_Tx_Buf + 512 <= 0x20010000,
       "USBHS EP3 buffer exceeds DMA SRAM address range")
