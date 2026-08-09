#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Board hardware map.
 * Ambiguous/conflicting signals remain placeholders and are marked TODO.
 */
#define PIN_PLACEHOLDER                 0u
#define GPIO_PORT_PLACEHOLDER           ((GPIO_TypeDef *)0)
#define GPIO_PIN_PLACEHOLDER            ((uint16_t)0u)
#define GPIO_LEVEL_PLACEHOLDER          GPIO_PIN_RESET
#define ADC_CHANNEL_PLACEHOLDER         0u
#define PLACEHOLDER_DELAY_US            0u
#define PLACEHOLDER_DELAY_MS            0u
#define PLACEHOLDER_MV                  0u

#define PIN_PA0                         0u
#define PIN_PA1                         1u
#define PIN_PA2                         2u
#define PIN_PA3                         3u
#define PIN_PA9                         9u
#define PIN_PA10                        10u
#define PIN_PA11                        11u
#define PIN_PA12                        12u
#define PIN_PA13                        13u
#define PIN_PA14                        14u
#define PIN_PC0                         0u
#define PIN_PC6                         6u
#define PIN_PC7                         7u
#define PIN_PC8                         8u
#define PIN_PC9                         9u
#define PIN_PC10                        10u
#define PIN_PC11                        11u
#define PIN_PC12                        12u
#define PIN_PC13                        13u

#define BOARD_GPIO_CLOCK_ENABLE()       do {        \
                                            __HAL_RCC_GPIOA_CLK_ENABLE(); \
                                            __HAL_RCC_GPIOC_CLK_ENABLE(); \
                                        } while (0)

#define PIN_USB_DM                      PIN_PA11
#define PIN_USB_DP                      PIN_PA12
#define PIN_USB_VBUS                    PIN_PLACEHOLDER
#define USB_DM_GPIO                     GPIOA
#define USB_DM_PIN                      GPIO_PIN_11
#define USB_DP_GPIO                     GPIOA
#define USB_DP_PIN                      GPIO_PIN_12
#define USB_GPIO_AF                     GPIO_AF10_OTG_FS

#define PIN_PROBE_SWDIO                 PIN_PA13
#define PIN_PROBE_SWCLK                 PIN_PA14

#define PIN_TARGET_SWDIO                PIN_PC12
#define PIN_TARGET_SWCLK                PIN_PC11
#define PIN_TARGET_NRST                 PIN_PC10
#define PIN_TARGET_SWO                  PIN_PC13

#define TARGET_SWDIO_GPIO               GPIOC
#define TARGET_SWDIO_PIN                GPIO_PIN_12
#define TARGET_SWCLK_GPIO               GPIOC
#define TARGET_SWCLK_PIN                GPIO_PIN_11
#define TARGET_NRST_GPIO                GPIOC
#define TARGET_NRST_PIN                 GPIO_PIN_10
#define TARGET_SWO_GPIO                 GPIOC
#define TARGET_SWO_PIN                  GPIO_PIN_13

#define TARGET_NRST_ACTIVE_LEVEL        GPIO_PIN_RESET
#define TARGET_NRST_INACTIVE_LEVEL      GPIO_PIN_SET
#define TARGET_RESET_PULSE_US           1000u
#define DAP_SWJ_PINS_MAX_WAIT_US        1000u

#define PIN_TMUX_SEL0                   PIN_PC8
#define PIN_TMUX_SEL1                   PIN_PLACEHOLDER
#define PIN_TMUX_EN_N                   PIN_PC9
#define TMUX_SEL0_GPIO                  GPIOC
#define TMUX_SEL0_PIN                   GPIO_PIN_8
#define TMUX_SEL1_GPIO                  GPIO_PORT_PLACEHOLDER
#define TMUX_SEL1_PIN                   GPIO_PIN_PLACEHOLDER
#define TMUX_EN_N_GPIO                  GPIOC
#define TMUX_EN_N_PIN                   GPIO_PIN_9

#define TMUX_ENABLE_ACTIVE_LEVEL        GPIO_PIN_RESET
#define TMUX_ENABLE_INACTIVE_LEVEL      GPIO_PIN_SET

/* TMUX1574: EN# low enables all four switches; SEL low selects SxA, SEL high selects SxB. */
#define TMUX_NONE_SEL0_LEVEL            GPIO_PIN_RESET
#define TMUX_NONE_SEL1_LEVEL            GPIO_LEVEL_PLACEHOLDER
#define TMUX_G474_SEL0_LEVEL            GPIO_PIN_RESET
#define TMUX_G474_SEL1_LEVEL            GPIO_LEVEL_PLACEHOLDER
#define TMUX_F429_SEL0_LEVEL            GPIO_PIN_SET
#define TMUX_F429_SEL1_LEVEL            GPIO_LEVEL_PLACEHOLDER
#define TMUX_SETTLE_TIME_US             5u

#define PIN_G474_UART_TX                PIN_PA9
#define PIN_G474_UART_RX                PIN_PA10
#define PIN_F429_UART_TX                PIN_PC6
#define PIN_F429_UART_RX                PIN_PC7
#define PIN_ESP32_UART_TX               PIN_PA2
#define PIN_ESP32_UART_RX               PIN_PA3

#define G474_UART_TX_GPIO               GPIOA
#define G474_UART_TX_PIN                GPIO_PIN_9
#define G474_UART_RX_GPIO               GPIOA
#define G474_UART_RX_PIN                GPIO_PIN_10
#define G474_UART_GPIO_AF               GPIO_AF7_USART1

#define F429_UART_TX_GPIO               GPIOC
#define F429_UART_TX_PIN                GPIO_PIN_6
#define F429_UART_RX_GPIO               GPIOC
#define F429_UART_RX_PIN                GPIO_PIN_7
#define F429_UART_GPIO_AF               GPIO_AF8_USART6

#define ESP32_UART_TX_GPIO              GPIOA
#define ESP32_UART_TX_PIN               GPIO_PIN_2
#define ESP32_UART_RX_GPIO              GPIOA
#define ESP32_UART_RX_PIN               GPIO_PIN_3
#define ESP32_UART_GPIO_AF              GPIO_AF7_USART2

#define PIN_ESP32_EN                    PIN_PA1
#define PIN_ESP32_IO0                   PIN_PA0
#define ESP32_EN_GPIO                   GPIOA
#define ESP32_EN_PIN                    GPIO_PIN_1
#define ESP32_IO0_GPIO                  GPIOA
#define ESP32_IO0_PIN                   GPIO_PIN_0
#define ESP32_EN_ACTIVE_LEVEL           GPIO_PIN_SET
#define ESP32_EN_INACTIVE_LEVEL         GPIO_PIN_RESET
#define ESP32_IO0_BOOT_LEVEL            GPIO_PIN_RESET
#define ESP32_IO0_RUN_LEVEL             GPIO_PIN_SET
#define ESP32_RESET_DELAY_MS            50u
#define ESP32_BOOT_DELAY_MS             50u

#define PIN_STATUS_LED                  PIN_PC0
#define STATUS_LED_GPIO                 GPIOC
#define STATUS_LED_PIN                  GPIO_PIN_0
#define LED_ACTIVE_LEVEL                GPIO_PIN_SET
#define LED_INACTIVE_LEVEL              GPIO_PIN_RESET

#define PIN_VTREF_ADC                   PIN_PLACEHOLDER
#define VTREF_ADC_CHANNEL               ADC_CHANNEL_PLACEHOLDER
#define VTREF_R_TOP                     0u
#define VTREF_R_BOTTOM                  0u
#define ADC_VREF_MV                     PLACEHOLDER_MV
#define ADC_MAX_COUNTS                  4095u
#define VTREF_PRESENT_THRESHOLD_MV      PLACEHOLDER_MV

#define UF4_FW_VERSION_MAJOR            0u
#define UF4_FW_VERSION_MINOR            1u
#define UF4_FW_VERSION_PATCH            0u
#define UF4_BOARD_NAME                  "UF4-F401-DAP-PLACEHOLDER"

#ifdef __cplusplus
}
#endif

#endif /* BOARD_CONFIG_H */
