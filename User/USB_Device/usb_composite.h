#ifndef USER_USB_COMPOSITE_H_
#define USER_USB_COMPOSITE_H_

#include "ch32v30x.h"
#include "ch32v30x_usbhs_device.h"

#define USB_CDC_COUNT 3
#define USB_DAP_EP 7

void USBComposite_Init(void);
void USBComposite_EndpointInit(void);
void USBComposite_Poll(void);
void USBComposite_OnOut(uint8_t ep, uint16_t len);
void USBComposite_OnIn(uint8_t ep);
uint8_t USBComposite_ClassRequest(uint8_t request_type, uint8_t request,
                                  uint16_t value, uint16_t index, uint16_t length,
                                  const uint8_t **data, uint16_t *data_len);
void USBComposite_ClassOut(uint8_t request, uint16_t index, const uint8_t *data, uint16_t len);
uint8_t USBComposite_VendorRequest(uint8_t request_type, uint8_t request,
                                   uint16_t value, uint16_t index, uint16_t length,
                                   const uint8_t **data, uint16_t *data_len);

#endif
