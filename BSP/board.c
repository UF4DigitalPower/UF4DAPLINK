#include "board.h"
#include "board_config.h"
#include "led.h"
#include "tmux.h"
#include "adc.h"
#include "../ESP32/esp32.h"
#include "../UART/uart_manager.h"

void Board_Init(void)
{
    LED_Init();
    TMUX_Init();
    ESP32_Init();
    UART_Manager_Init();
}

uint32_t Board_Millis(void)
{
    return HAL_GetTick();
}

void Board_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000u);

    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0u) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0u;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        start = DWT->CYCCNT;
    }

    while ((DWT->CYCCNT - start) < ticks) {
        __NOP();
    }
}

uint16_t Board_ReadVTREF_mV(void)
{
    return 0u;
}

void Board_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState level)
{
    if ((port != GPIO_PORT_PLACEHOLDER) && (pin != GPIO_PIN_PLACEHOLDER)) {
        HAL_GPIO_WritePin(port, pin, level);
    }
}

GPIO_PinState Board_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    if ((port == GPIO_PORT_PLACEHOLDER) || (pin == GPIO_PIN_PLACEHOLDER)) {
        return GPIO_PIN_RESET;
    }

    return HAL_GPIO_ReadPin(port, pin);
}

GPIO_PinState Board_ActiveLevel(bool active, GPIO_PinState active_level)
{
    if (active) {
        return active_level;
    }

    return (active_level == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
}
