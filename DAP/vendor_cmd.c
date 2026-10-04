#include "vendor_cmd.h"
#include "target.h"
#include "../BSP/board.h"
#include "../BSP/board_config.h"
#include "../BSP/led.h"
#include "../ESP32/esp32.h"
#include "../UART/uart_manager.h"
#include "../Config/memory_config.h"

static void VendorCmd_WriteU32(uint8_t *response, uint16_t *offset, uint32_t value)
{
    response[(*offset)++] = (uint8_t)(value & 0xFFu);
    response[(*offset)++] = (uint8_t)((value >> 8u) & 0xFFu);
    response[(*offset)++] = (uint8_t)((value >> 16u) & 0xFFu);
    response[(*offset)++] = (uint8_t)((value >> 24u) & 0xFFu);
}

static uint16_t VendorCmd_RequestLength(uint8_t cmd, uint16_t packet_len)
{
    switch (cmd) {
    case UF4_CMD_SET_TARGET:
    case UF4_CMD_SET_LED:
    case UF4_CMD_GET_UART_STATS:
    case UF4_CMD_CLEAR_UART_STATS:
        return 2u;
    case UF4_CMD_GET_UART_RX_SAMPLE:
        return 4u;
    case UF4_CMD_GET_TARGET:
    case UF4_CMD_ESP32_BOOT:
    case UF4_CMD_ESP32_RESET:
    case UF4_CMD_GET_VTREF:
    case UF4_CMD_GET_VERSION:
    case UF4_CMD_GET_BOARD_INFO:
    case UF4_CMD_RESET_ALL:
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
        (void)UART_Manager_ResetBridge(UART_BRIDGE_ESP32);
        ESP32_EnterBootloader();
        return 2u;

    case UF4_CMD_ESP32_RESET:
        (void)UART_Manager_ResetBridge(UART_BRIDGE_ESP32);
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

    case UF4_CMD_RESET_ALL:
        Target_ResetAll();
        return 2u;

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

    case UF4_CMD_GET_UART_STATS: {
        uart_bridge_stats_t stats;
        uint16_t offset = 3u;

        if ((request_len < 2u) || (response_max < 35u) ||
            !UART_Manager_GetStats((uart_bridge_id_t)request[1], &stats)) {
            response[1] = UF4_STATUS_ERROR;
            return 2u;
        }

        response[2] = request[1];
        VendorCmd_WriteU32(response, &offset, stats.usb_to_uart_bytes);
        VendorCmd_WriteU32(response, &offset, stats.usb_to_uart_dropped);
        VendorCmd_WriteU32(response, &offset, stats.uart_tx_bytes);
        VendorCmd_WriteU32(response, &offset, stats.uart_tx_errors);
        VendorCmd_WriteU32(response, &offset, stats.uart_rx_bytes);
        VendorCmd_WriteU32(response, &offset, stats.uart_rx_dropped);
        VendorCmd_WriteU32(response, &offset, stats.usb_from_uart_bytes);
        VendorCmd_WriteU32(response, &offset, stats.uart_rx_start_errors);
        return offset;
    }

    case UF4_CMD_CLEAR_UART_STATS:
        if ((request_len < 2u) || !UART_Manager_ClearStats((uart_bridge_id_t)request[1])) {
            response[1] = UF4_STATUS_ERROR;
        }
        return 2u;

    case UF4_CMD_GET_UART_RX_SAMPLE: {
        uint16_t offset;
        size_t len;

        if ((request_len < 4u) || (response_max < 7u)) {
            response[1] = UF4_STATUS_ERROR;
            return 2u;
        }

        offset = (uint16_t)request[2] | ((uint16_t)request[3] << 8u);
        response[2] = request[1];
        response[3] = request[2];
        response[4] = request[3];
        len = UART_Manager_GetRxSample((uart_bridge_id_t)request[1],
                                       offset,
                                       &response[6],
                                       (size_t)(response_max - 6u));
        response[5] = (uint8_t)len;
        return (uint16_t)(6u + len);
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
