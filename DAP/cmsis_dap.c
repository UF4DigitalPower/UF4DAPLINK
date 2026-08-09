#include "cmsis_dap.h"
#include "DAP.h"
#include "target.h"
#include "../Config/memory_config.h"
#include <string.h>

#define UF4_ID_DAP_QUEUE_COMMANDS       0x7Eu
#define UF4_ID_DAP_EXECUTE_COMMANDS     0x7Fu

static uint8_t s_queue_packets[DAP_PACKET_COUNT][DAP_PACKET_SIZE];
static uint8_t s_queue_count;

static uint8_t s_pending_packets[DAP_PACKET_COUNT + 1u][DAP_PACKET_SIZE];
static uint16_t s_pending_len[DAP_PACKET_COUNT + 1u];
static uint8_t s_pending_head;
static uint8_t s_pending_tail;
static uint8_t s_pending_count;

static uint16_t CMSIS_DAP_RunOfficial(const uint8_t *request, uint8_t *response)
{
    uint32_t result = DAP_ExecuteCommand(request, response);
    return (uint16_t)(result & 0xFFFFu);
}

static bool CMSIS_DAP_QueuePending(const uint8_t *response, uint16_t response_len)
{
    if (s_pending_count >= (DAP_PACKET_COUNT + 1u)) {
        return false;
    }

    if (response_len > DAP_PACKET_SIZE) {
        response_len = DAP_PACKET_SIZE;
    }

    memcpy(s_pending_packets[s_pending_tail], response, response_len);
    s_pending_len[s_pending_tail] = response_len;
    s_pending_tail = (uint8_t)((s_pending_tail + 1u) % (DAP_PACKET_COUNT + 1u));
    s_pending_count++;
    return true;
}

static bool CMSIS_DAP_PopPending(uint8_t *response, uint16_t *response_len, uint16_t response_max)
{
    uint16_t len;

    if ((s_pending_count == 0u) || (response == 0) || (response_len == 0)) {
        return false;
    }

    len = s_pending_len[s_pending_head];
    if (len > response_max) {
        len = response_max;
    }

    memcpy(response, s_pending_packets[s_pending_head], len);
    *response_len = len;
    s_pending_head = (uint8_t)((s_pending_head + 1u) % (DAP_PACKET_COUNT + 1u));
    s_pending_count--;
    return true;
}

static void CMSIS_DAP_FlushQueuedCommands(const uint8_t *final_request)
{
    uint8_t exec_packet[DAP_PACKET_SIZE];
    uint8_t response[DAP_PACKET_SIZE];
    uint16_t response_len;

    for (uint8_t i = 0u; i < s_queue_count; i++) {
        memcpy(exec_packet, s_queue_packets[i], DAP_PACKET_SIZE);
        exec_packet[0] = UF4_ID_DAP_EXECUTE_COMMANDS;
        response_len = CMSIS_DAP_RunOfficial(exec_packet, response);
        (void)CMSIS_DAP_QueuePending(response, response_len);
    }

    s_queue_count = 0u;

    response_len = CMSIS_DAP_RunOfficial(final_request, response);
    (void)CMSIS_DAP_QueuePending(response, response_len);
}

void DAP_Init(void)
{
    Target_Init();
    DAP_Setup();
    s_queue_count = 0u;
    s_pending_head = 0u;
    s_pending_tail = 0u;
    s_pending_count = 0u;
}

void DAP_Task(void)
{
}

uint16_t CMSIS_DAP_ExecuteCommand(const uint8_t *request,
                                  uint16_t request_len,
                                  uint8_t *response,
                                  uint16_t response_max)
{
    uint16_t response_len;

    (void)request_len;

    if ((request == 0) || (response == 0) || (response_max < DAP_PACKET_SIZE)) {
        return 0u;
    }

    if (request[0] == UF4_ID_DAP_QUEUE_COMMANDS) {
        if (s_queue_count < DAP_PACKET_COUNT) {
            uint16_t copy_len = (request_len > DAP_PACKET_SIZE) ? DAP_PACKET_SIZE : request_len;
            memset(s_queue_packets[s_queue_count], 0, DAP_PACKET_SIZE);
            memcpy(s_queue_packets[s_queue_count], request, copy_len);
            s_queue_count++;
        }
        return 0u;
    }

    if (s_queue_count != 0u) {
        CMSIS_DAP_FlushQueuedCommands(request);
        if (!CMSIS_DAP_PopPending(response, &response_len, response_max)) {
            return 0u;
        }
        return response_len;
    }

    response_len = CMSIS_DAP_RunOfficial(request, response);

    return (response_len > response_max) ? response_max : response_len;
}

bool CMSIS_DAP_GetPendingResponse(uint8_t *response, uint16_t *response_len, uint16_t response_max)
{
    return CMSIS_DAP_PopPending(response, response_len, response_max);
}
