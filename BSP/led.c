#include "led.h"
#include "board.h"
#include "board_config.h"

static led_state_t s_led_state = LED_OFF;
static uint32_t s_last_toggle_ms;
static bool s_led_on;

void LED_Init(void)
{
    LED_Off();
    s_led_state = LED_OFF;
}

void LED_On(void)
{
    Board_WritePin(STATUS_LED_GPIO, STATUS_LED_PIN, LED_ACTIVE_LEVEL);
    s_led_on = true;
}

void LED_Off(void)
{
    Board_WritePin(STATUS_LED_GPIO, STATUS_LED_PIN, LED_INACTIVE_LEVEL);
    s_led_on = false;
}

void LED_Toggle(void)
{
    if (s_led_on) {
        LED_Off();
    } else {
        LED_On();
    }
}

void LED_SetState(led_state_t state)
{
    s_led_state = state;
    s_last_toggle_ms = Board_Millis();

    if (state == LED_OFF) {
        LED_Off();
    }
}

void LED_Task(void)
{
    uint32_t period_ms;

    switch (s_led_state) {
    case LED_USB_CONNECTED:
        period_ms = 1000u;
        break;
    case LED_DAP_CONNECTED:
        period_ms = 250u;
        break;
    case LED_PROGRAMMING:
        period_ms = 75u;
        break;
    case LED_ERROR:
        period_ms = 125u;
        break;
    case LED_OFF:
    default:
        LED_Off();
        return;
    }

    if ((Board_Millis() - s_last_toggle_ms) >= period_ms) {
        s_last_toggle_ms = Board_Millis();
        LED_Toggle();
    }
}
