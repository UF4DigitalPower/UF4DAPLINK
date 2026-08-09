#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "usart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define huart_g474 huart1
#define huart_esp32 huart2
#define huart_f429 huart6

/** Initialize clocks, HAL-owned peripherals, GPIO placeholders, and BSP modules. */
void Board_Init(void);

/** Configure the STM32 system clock and USB 48 MHz clock source. */
void Board_SystemClock_Config(void);

/** Return the monotonic HAL tick in milliseconds. */
uint32_t Board_Millis(void);

/** Busy-wait for short hardware settle windows only. */
void Board_DelayUs(uint32_t us);

/** Read the target reference voltage in millivolts. */
uint16_t Board_ReadVTREF_mV(void);

/** Write a board-configured GPIO if it has been assigned. */
void Board_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState level);

/** Read a board-configured GPIO; returns GPIO_PIN_RESET for placeholders. */
GPIO_PinState Board_ReadPin(GPIO_TypeDef *port, uint16_t pin);

/** Convert logical active/inactive state into the configured active level. */
GPIO_PinState Board_ActiveLevel(bool active, GPIO_PinState active_level);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
