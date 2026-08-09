#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    ITF_NUM_HID = 0,
    ITF_NUM_TARGET_CDC,
    ITF_NUM_TARGET_CDC_DATA,
    ITF_NUM_ESP32_CDC,
    ITF_NUM_ESP32_CDC_DATA,
    ITF_NUM_TOTAL
};

#define EPNUM_HID_OUT                   0x01u
#define EPNUM_HID_IN                    0x81u

#define EPNUM_TARGET_CDC_OUT            0x02u
#define EPNUM_TARGET_CDC_IN             0x82u

#define EPNUM_ESP32_CDC_OUT             0x03u
#define EPNUM_ESP32_CDC_IN              0x83u

#ifdef __cplusplus
}
#endif

#endif /* USB_DESCRIPTORS_H */
