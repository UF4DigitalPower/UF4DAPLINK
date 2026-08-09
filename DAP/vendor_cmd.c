#include "vendor_cmd.h"
#include "target.h"
#include "../BSP/board.h"
#include "../BSP/board_config.h"
#include "../BSP/led.h"
#include "../ESP32/esp32.h"
#include "../Config/memory_config.h"

static uint16_t VendorCmd_RequestLength(uint8_t cmd, uint16_t packet_len)
{
    switch (cmd) {
    case UF4_CMD_SET_TARGET:
    case UF4_CMD_SET_LED:
        return 2u;
    case UF4_CMD_GET_TARGET:
    case UF4_CMD_ESP32_BOOT:
    case UF4_CMD_ESP32_RESET:
    case UF4_CMD_GET_VTREF:
    case UF4_CMD_GET_VERSION:
    case UF4_CMD_GET_BOARD_INFO:
        return 1u;
    default:
        return packet_len;
    }
}

bool VendorCmd_IsVendor(uint8_t cmd)
{
    return (cmd >= 0x80u) && (cmd <= 0x9Fu);
}

uint16_t VendorCmd_Process(const uint8_t *request, uint16_t request_len, uint8_t *response, uint16_t response_max)
{
    uint8_t cmd;

    if ((request_len == 0u) || (response_max < 2u)) {
        return 0u;
    }

    cmd = request[0];
    response[0] = cmd;
    response[1] = UF4_STATUS_OK;

    switch (cmd) {
    case UF4_CMD_SET_TARGET:
        if ((request_len < 2u) || !Target_Select((target_t)request[1])) {
            response[1] = UF4_STATUS_ERROR;
        }
        return 2u;

    case UF4_CMD_GET_TARGET:
        if (response_max < 3u) {
            return 0u;
        }
        response[2] = (uint8_t)Target_Get();
        return 3u;

    case UF4_CMD_ESP32_BOOT:
        ESP32_EnterBootloader();
        return 2u;

    case UF4_CMD_ESP32_RESET:
        ESP32_Reset();
        return 2u;

    case UF4_CMD_GET_VTREF: {
        uint16_t mv;
        if (response_max < 4u) {
            return 0u;
        }
        mv = Board_ReadVTREF_mV();
        response[2] = (uint8_t)(mv & 0xFFu);
        response[3] = (uint8_t)(mv >> 8u);
        return 4u;
    }

    case UF4_CMD_SET_LED:
        if (request_len < 2u) {
            response[1] = UF4_STATUS_ERROR;
            return 2u;
        }
        LED_SetState((led_state_t)request[1]);
        return 2u;

    case UF4_CMD_GET_VERSION:
        if (response_max < 5u) {
            return 0u;
        }
        response[2] = UF4_FW_VERSION_MAJOR;
        response[3] = UF4_FW_VERSION_MINOR;
        response[4] = UF4_FW_VERSION_PATCH;
        return 5u;

    case UF4_CMD_GET_BOARD_INFO: {
        const char *name = UF4_BOARD_NAME;
        uint16_t i = 2u;

        while ((*name != '\0') && (i < response_max)) {
            response[i++] = (uint8_t)*name++;
        }

        return i;
    }

    default:
        response[1] = UF4_STATUS_ERROR;
        return 2u;
    }
}

uint32_t DAP_ProcessVendorCommand(const uint8_t *request, uint8_t *response)
{
    uint16_t request_len;
    uint16_t response_len;

    request_len = VendorCmd_RequestLength(request[0], DAP_PACKET_SIZE);
    response_len = VendorCmd_Process(request, request_len, response, DAP_PACKET_SIZE);

    return ((uint32_t)request_len << 16u) | response_len;
}
