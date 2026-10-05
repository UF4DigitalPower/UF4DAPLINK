#ifndef UF4_BOARD_LED_H
#define UF4_BOARD_LED_H

#include "ch32v30x.h"

typedef enum
{
    UF4_LED_OFF,
    UF4_LED_COLOR1,
    UF4_LED_COLOR2
} UF4_LED_Color;

static inline void UF4_LED_Set( UF4_LED_Color color )
{
    GPIO_InitTypeDef gpio = {0};

    gpio.GPIO_Pin = GPIO_Pin_15;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    if( color == UF4_LED_OFF )
    {
        /* Both LED branches share PB15, so off requires high impedance. */
        gpio.GPIO_Mode = GPIO_Mode_AIN;
    }
    else
    {
        if( color == UF4_LED_COLOR1 )
        {
            GPIO_ResetBits( GPIOB, GPIO_Pin_15 );
        }
        else
        {
            GPIO_SetBits( GPIOB, GPIO_Pin_15 );
        }
        gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    }
    GPIO_Init( GPIOB, &gpio );
}

static inline void UF4_LED_Init( void )
{
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE );
    UF4_LED_Set( UF4_LED_OFF );
}

#endif
