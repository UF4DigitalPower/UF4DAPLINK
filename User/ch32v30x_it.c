/********************************** (C) COPYRIGHT *******************************
* File Name          : ch32v30x_it.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main Interrupt Service Routines.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "ch32v30x_it.h"
#include "UART.h"
#include "board_led.h"

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler( void )__attribute__((interrupt("WCH-Interrupt-fast")));

/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  none
 */
/* Fault indication uses a busy wait because SysTick may be unavailable. */
static void Fatal_LED_Flash( void )
{
    volatile uint32_t i;
    uint8_t lit = 1;

    UF4_LED_Init( );
    while( 1 )
    {
        UF4_LED_Set( lit ? UF4_LED_COLOR2 : UF4_LED_OFF );
        lit ^= 1;
        IWDG_ReloadCounter( );
        for( i = 0; i < 2000000; i++ )
        {
        }
    }
}

void NMI_Handler(void)
{
  Fatal_LED_Flash();
}

/*********************************************************************
 * @fn      TIM2_IRQHandler
 *
 * @brief   This function handles TIM2 exception.
 *
 * @return  none
 */
void TIM2_IRQHandler( void )
{
    /* Test IO */
    static uint8_t tog;
    tog ? (GPIOA->BSHR = GPIO_Pin_15):(GPIOA->BCR = GPIO_Pin_15);
    tog ^= 1;
    /* uart timeout counts */
    Uart.Rx_TimeOut++;
    Uart.USB_Up_TimeOut++;

    /* clear status */
    TIM2->INTFR = (uint16_t)~TIM_IT_Update;
}

/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   This function handles Hard Fault exception.
 *
 * @return  none
 */
void HardFault_Handler(void)
{
    Fatal_LED_Flash();
}


