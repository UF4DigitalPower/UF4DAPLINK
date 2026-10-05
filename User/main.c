/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

/*
 *@Note
 * USBHS composite device: three CDC ACM serial ports plus a CMSIS-DAP/WebUSB
 * vendor interface. The CDC ports are wired to USART1, USART3 and UART8.
 *
 * Clock: SYSCLK = HSE (external 8 MHz) * 18 = 144 MHz.
 *        USBHS PHY reference = HSE / 2 = 4 MHz.
 *
 * Dual-color LED on PB15 (low = color 1, high = color 2, high-Z = off):
 *   off           -> no USB event yet
 *   color 1       -> USB bus reset seen, no SETUP yet
 *   color 2       -> CDC enumerated
 *   color 1 blink -> SETUP seen, enumeration in progress
 *   color 2 blink -> clock fault, HardFault or NMI
 */

#include "UART.h"
#include "debug.h"
#include "ch32v30x_usbhs_device.h"
#include "board_led.h"
#include "usb_composite.h"

static void LED_Wait( uint16_t milliseconds )
{
    while( milliseconds-- )
    {
        Delay_Ms( 1 );
        USBComposite_Poll( );
    }
}

static void LED_Indicate( UF4_LED_Color color, uint8_t blink )
{
    UF4_LED_Set( color );
    if( blink )
    {
        LED_Wait( 250 );
        UF4_LED_Set( UF4_LED_OFF );
        LED_Wait( 250 );
    }
    else
    {
        LED_Wait( 50 );
    }
}

static void IWDG_SoftwareInit( void )
{
    IWDG_WriteAccessCmd( IWDG_WriteAccess_Enable );
    IWDG_SetPrescaler( IWDG_Prescaler_32 );
    IWDG_SetReload( 4000 );
    IWDG_ReloadCounter( );
    IWDG_Enable( );
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    uint8_t clock_fault;

    SystemCoreClockUpdate( );
    Delay_Init( );
    UF4_LED_Init( );

    /* Clock self test, read straight from the RCC registers. */
    clock_fault = 0;
    if( ( RCC->CTLR & RCC_HSERDY ) == 0 )
    {
        clock_fault |= 0x01;                                    /* HSE not ready */
    }
    if( ( RCC->CTLR & RCC_PLLRDY ) == 0 )
    {
        clock_fault |= 0x02;                                    /* PLL never locked */
    }
    if( ( RCC->CFGR0 & RCC_SWS ) != RCC_SWS_PLL )
    {
        clock_fault |= 0x04;                                    /* not running from PLL */
    }
    if( ( RCC->CFGR0 & RCC_PLLSRC ) != RCC_PLLSRC_HSE )
    {
        clock_fault |= 0x08;                                    /* PLL not sourced from HSE */
    }

    /* Start the watchdog before USB initialization. */
    IWDG_SoftwareInit( );
    RCC_Configuration( );
    USBComposite_Init( );
    USBHS_RCC_Init( );
    USBHS_Device_Init( ENABLE );

    while(1)
    {
        IWDG_ReloadCounter( );

        if( ( clock_fault != 0 ) || ( USBHS_HseFail != 0 ) )
        {
            LED_Indicate( UF4_LED_COLOR2, 1 );
        }
        else if( USBHS_DevEnumStatus != 0 )
        {
            LED_Indicate( UF4_LED_COLOR2, 0 );
        }
        else if( USBHS_SetupCnt != 0 )
        {
            LED_Indicate( UF4_LED_COLOR1, 1 );
        }
        else if( USBHS_BusResetCnt != 0 )
        {
            LED_Indicate( UF4_LED_COLOR1, 0 );
        }
        else
        {
            LED_Indicate( UF4_LED_OFF, 0 );
        }
    }
}
