/* USBHS DMA address registers cover only the first 64 KiB of SRAM. */
ASSERT(USBHS_EP0_Buf >= 0x20000000 && USBHS_EP0_Buf + 64 <= 0x20010000,
       "USBHS EP0 buffer exceeds DMA SRAM address range")
