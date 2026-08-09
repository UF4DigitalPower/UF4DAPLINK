#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    ITF_NUM_HID = 0,
    ITF_NUM_CDC0,
    ITF_NUM_CDC0_DATA,
    ITF_NUM_TOTAL
};

#define EPNUM_HID_OUT                   0x01u
#define EPNUM_HID_IN                    0x81u

#define EPNUM_CDC0_NOTIF                0x82u
#define EPNUM_CDC0_OUT                  0x02u
#define EPNUM_CDC0_IN                   0x83u

#ifdef __cplusplus
}
#endif

#endif /* USB_DESCRIPTORS_H */
