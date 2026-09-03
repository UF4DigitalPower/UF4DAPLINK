#include "target.h"
#include "DAP.h"
#include "DAP_config.h"
#include "../BSP/board.h"
#include "../BSP/board_config.h"
#include "../BSP/tmux.h"

static target_t s_current_target = TARGET_F429;

static bool Target_IsValid(target_t target)
{
    return (target == TARGET_G474) || (target == TARGET_F429);
}

static void Target_SafeLines(void)
{
    PORT_OFF();
}

void Target_Init(void)
{
    s_current_target = TARGET_F429;
    Target_SafeLines();
    (void)TMUX_SelectF429();
}

bool Target_Select(target_t target)
{
    if (!Target_IsValid(target)) {
        return false;
    }

    if (target == s_current_target) {
        return true;
    }

    Target_SafeLines();
    (void)TMUX_SelectNone();

    if (!TMUX_Select(target)) {
        return false;
    }

    PORT_SWD_SETUP();
    Board_WritePin(TARGET_NRST_GPIO, TARGET_NRST_PIN, TARGET_NRST_INACTIVE_LEVEL);
    s_current_target = target;

    return true;
}

target_t Target_Get(void)
{
    return s_current_target;
}

void Target_Reset(void)
{
    PIN_nRESET_OUT(0U);
    Board_DelayUs(TARGET_RESET_PULSE_US);
    PIN_nRESET_OUT(1U);
}

void Target_ResetAllAtStartup(void)
{
    (void)Target_Select(TARGET_G474);
    Target_Reset();

    (void)Target_Select(TARGET_F429);
    Target_Reset();
}

bool Target_Detect(void)
{
    return Board_ReadVTREF_mV() > VTREF_PRESENT_THRESHOLD_MV;
}

uint32_t Target_ReadIDCODE(void)
{
    uint32_t idcode = 0u;

    if (SWD_Transfer(DAP_TRANSFER_RnW, &idcode) != DAP_TRANSFER_OK) {
        return 0u;
    }

    return idcode;
}

void Target_Task(void)
{
}
