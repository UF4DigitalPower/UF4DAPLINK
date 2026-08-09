#ifndef VENDOR_CMD_H
#define VENDOR_CMD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UF4_CMD_SET_TARGET              0x80u
#define UF4_CMD_GET_TARGET              0x81u
#define UF4_CMD_ESP32_BOOT              0x82u
#define UF4_CMD_ESP32_RESET             0x83u
#define UF4_CMD_GET_VTREF               0x84u
#define UF4_CMD_SET_LED                 0x85u
#define UF4_CMD_GET_VERSION             0x86u
#define UF4_CMD_GET_BOARD_INFO          0x87u

#define UF4_STATUS_OK                   0x00u
#define UF4_STATUS_ERROR                0xFFu

/** Return true when cmd is in the UF4 CMSIS-DAP vendor command range. */
bool VendorCmd_IsVendor(uint8_t cmd);

/** Process one UF4 vendor command packet. */
uint16_t VendorCmd_Process(const uint8_t *request, uint16_t request_len, uint8_t *response, uint16_t response_max);

#ifdef __cplusplus
}
#endif

#endif /* VENDOR_CMD_H */
