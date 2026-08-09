#include "tusb.h"
#include "usb_descriptors.h"
#include "../BSP/board_config.h"
#include "../Config/memory_config.h"

#define USB_VID                         0xCafeu
#define USB_PID                         0x401Bu
#define USB_BCD                         0x0100u

#define CONFIG_TOTAL_LEN                (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN + TUD_CDC_DESC_LEN)

enum
{
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_CDC0,
    STRID_CDC1,
    STRID_CDC2,
    STRID_CMSIS_DAP
};

static tusb_desc_device_t const s_device_desc = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200u,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = USB_BCD,
    .iManufacturer      = STRID_MANUFACTURER,
    .iProduct           = STRID_PRODUCT,
    .iSerialNumber      = STRID_SERIAL,
    .bNumConfigurations = 1u,
};

uint8_t const s_hid_report_desc[] = {
    TUD_HID_REPORT_DESC_GENERIC_INOUT(DAP_PACKET_SIZE)
};

static uint8_t const s_config_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, STRID_CMSIS_DAP, HID_ITF_PROTOCOL_NONE,
                             sizeof(s_hid_report_desc), EPNUM_HID_OUT, EPNUM_HID_IN,
                             USB_HID_EP_BUFSIZE, 1),

    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC0, STRID_CDC0, EPNUM_CDC0_NOTIF, 8,
                       EPNUM_CDC0_OUT, EPNUM_CDC0_IN, USB_CDC_EP_BUFSIZE),
};

static char const *s_string_desc[] = {
    (const char[]){ 0x09, 0x04 },
    "UF4",
    "UF4 F401 CMSIS-DAP CDC",
    "UF4-F401-PLACEHOLDER",
    "G474 UART",
    "F429 UART",
    "ESP32 UART",
    "CMSIS-DAP",
};

static uint16_t s_desc_str[32];

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&s_device_desc;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return s_hid_report_desc;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return s_config_desc;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    uint8_t chr_count;

    (void)langid;

    if (index == STRID_LANGID) {
        s_desc_str[1] = 0x0409u;
        chr_count = 1u;
    } else {
        const char *str;

        if (index >= (sizeof(s_string_desc) / sizeof(s_string_desc[0]))) {
            return 0;
        }

        str = s_string_desc[index];
        chr_count = 0u;
        while ((str[chr_count] != '\0') && (chr_count < 31u)) {
            s_desc_str[1u + chr_count] = (uint8_t)str[chr_count];
            chr_count++;
        }
    }

    s_desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8u) | ((2u * chr_count) + 2u));
    return s_desc_str;
}
