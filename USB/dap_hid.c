#include "dap_hid.h"
#include "tusb.h"
#include "../DAP/cmsis_dap.h"
#include "../Config/memory_config.h"
#include <string.h>

static uint8_t s_dap_response[DAP_PACKET_SIZE];

static void DAP_HID_PadResponse(uint16_t len)
{
    if (len < DAP_PACKET_SIZE) {
        memset(&s_dap_response[len], 0, DAP_PACKET_SIZE - len);
    }
}

static void DAP_HID_SendPending(void)
{
    uint16_t len;

    if (!tud_hid_ready()) {
        return;
    }

    if (CMSIS_DAP_GetPendingResponse(s_dap_response, &len, sizeof(s_dap_response))) {
        DAP_HID_PadResponse(len);
        (void)tud_hid_report(0u, s_dap_response, DAP_PACKET_SIZE);
    }
}

void DAP_HID_Init(void)
{
}

uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer,
                               uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0u;
}

void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize)
{
    uint16_t len;

    (void)instance;
    (void)report_id;
    (void)report_type;

    len = CMSIS_DAP_ExecuteCommand(buffer, bufsize, s_dap_response, sizeof(s_dap_response));
    if ((len > 0u) && tud_hid_ready()) {
        DAP_HID_PadResponse(len);
        (void)tud_hid_report(0u, s_dap_response, DAP_PACKET_SIZE);
    }
}

void tud_hid_report_received_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
    (void)instance;
    tud_hid_set_report_cb(instance, 0u, HID_REPORT_TYPE_OUTPUT, report, len);
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
    (void)instance;
    (void)report;
    (void)len;
    DAP_HID_SendPending();
}
