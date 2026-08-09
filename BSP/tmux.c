#include "tmux.h"
#include "board.h"
#include "board_config.h"

static bool TMUX_Write(GPIO_PinState sel0, GPIO_PinState sel1, GPIO_PinState enable_level)
{
    Board_WritePin(TMUX_SEL0_GPIO, TMUX_SEL0_PIN, sel0);
    Board_WritePin(TMUX_SEL1_GPIO, TMUX_SEL1_PIN, sel1);
    Board_WritePin(TMUX_EN_N_GPIO, TMUX_EN_N_PIN, enable_level);
    Board_DelayUs(TMUX_SETTLE_TIME_US);
    return true;
}

void TMUX_Init(void)
{
    (void)TMUX_SelectNone();
}

bool TMUX_SelectNone(void)
{
    return TMUX_Write(TMUX_NONE_SEL0_LEVEL, TMUX_NONE_SEL1_LEVEL, TMUX_ENABLE_INACTIVE_LEVEL);
}

bool TMUX_SelectG474(void)
{
    return TMUX_Write(TMUX_G474_SEL0_LEVEL, TMUX_G474_SEL1_LEVEL, TMUX_ENABLE_ACTIVE_LEVEL);
}

bool TMUX_SelectF429(void)
{
    return TMUX_Write(TMUX_F429_SEL0_LEVEL, TMUX_F429_SEL1_LEVEL, TMUX_ENABLE_ACTIVE_LEVEL);
}

bool TMUX_Select(target_t target)
{
    switch (target) {
    case TARGET_NONE:
        return TMUX_SelectNone();
    case TARGET_G474:
        return TMUX_SelectG474();
    case TARGET_F429:
        return TMUX_SelectF429();
    default:
        return false;
    }
}
