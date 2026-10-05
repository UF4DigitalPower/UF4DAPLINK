#include "usbhs_echo.h"
#include "ch32v30x.h"
#include "ch32v30x_usb.h"

#define ECHO_TX_IDLE 0U
#define ECHO_TX_DATA 1U
#define ECHO_TX_ZLP  2U

_Static_assert((USBHS_ECHO_SLOT_COUNT & (USBHS_ECHO_SLOT_COUNT - 1U)) == 0,
               "Echo slot count must be a power of two");
_Static_assert(USBHS_ECHO_SLOT_COUNT < 256U, "Echo indices must fit in uint8_t");

__attribute__((section(".bss.usb_dma"), aligned(4)))
uint8_t USBHS_Echo_Buffer[USBHS_ECHO_SLOT_COUNT][USBHS_ECHO_PACKET_SIZE];
static uint16_t echo_lengths[USBHS_ECHO_SLOT_COUNT];
static volatile uint8_t echo_read;
static volatile uint8_t echo_write;
static volatile uint8_t echo_count;
static volatile uint8_t echo_tx;
static volatile uint8_t echo_rx_paused;
static volatile uint8_t echo_need_zlp;
static volatile uint8_t echo_idle_polls;

static void Echo_ArmReceive( void )
{
    if( echo_count < USBHS_ECHO_SLOT_COUNT &&
        (USBHSD->UEP2_RX_CTRL & USBHS_UEP_R_RES_MASK) != USBHS_UEP_R_RES_STALL )
    {
        USBHSD->UEP2_RX_DMA = (uint32_t)(uintptr_t)USBHS_Echo_Buffer[echo_write];
        USBHSD->UEP2_RX_CTRL = (USBHSD->UEP2_RX_CTRL & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_ACK;
        echo_rx_paused = 0;
    }
    else
    {
        echo_rx_paused = 1;
    }
}

static void Echo_StartTransmit( void )
{
    if( echo_count != 0 && echo_tx == ECHO_TX_IDLE &&
        (USBHSD->UEP2_TX_CTRL & USBHS_UEP_T_RES_MASK) != USBHS_UEP_T_RES_STALL )
    {
        USBHSD->UEP2_TX_DMA = (uint32_t)(uintptr_t)USBHS_Echo_Buffer[echo_read];
        USBHSD->UEP2_TX_LEN = echo_lengths[echo_read];
        echo_tx = ECHO_TX_DATA;
        echo_need_zlp = 0;
        USBHSD->UEP2_TX_CTRL = (USBHSD->UEP2_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_ACK;
    }
}

void USBHS_Echo_Init( void )
{
    echo_read = 0;
    echo_write = 0;
    echo_count = 0;
    echo_tx = ECHO_TX_IDLE;
    echo_rx_paused = 0;
    echo_need_zlp = 0;
    echo_idle_polls = 0;
    USBHSD->UEP2_TX_DMA = (uint32_t)(uintptr_t)USBHS_Echo_Buffer[0];
    USBHSD->UEP2_TX_LEN = 0;
    USBHSD->UEP2_TX_CTRL = USBHS_UEP_T_RES_NAK;
    USBHSD->UEP2_RX_CTRL = USBHS_UEP_R_RES_NAK;
    Echo_ArmReceive( );
}

void USBHS_Echo_Receive( uint16_t length )
{
    if( echo_rx_paused || length > USBHS_ECHO_PACKET_SIZE )
    {
        return;
    }
    USBHSD->UEP2_RX_CTRL = (USBHSD->UEP2_RX_CTRL & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_NAK;
    USBHSD->UEP2_RX_CTRL ^= USBHS_UEP_R_TOG_DATA1;
    echo_idle_polls = 0;
    if( length != 0 )
    {
        /* The RX DMA slot becomes a TX slot without copying its contents. */
        echo_lengths[echo_write] = length;
        echo_write = (echo_write + 1U) & (USBHS_ECHO_SLOT_COUNT - 1U);
        echo_count++;
    }
    Echo_ArmReceive( );
    Echo_StartTransmit( );
}

void USBHS_Echo_TransmitComplete( void )
{
    uint16_t length = USBHSD->UEP2_TX_LEN;
    uint16_t max_packet = (USBHSD->SPEED_TYPE & USBHS_USB_SPEED_TYPE) == USBHS_USB_SPEED_HIGH
                        ? USBHS_ECHO_PACKET_SIZE : 64U;

    USBHSD->UEP2_TX_CTRL = (USBHSD->UEP2_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_NAK;
    USBHSD->UEP2_TX_CTRL ^= USBHS_UEP_T_TOG_DATA1;
    if( echo_tx == ECHO_TX_DATA )
    {
        echo_read = (echo_read + 1U) & (USBHS_ECHO_SLOT_COUNT - 1U);
        echo_count--;
        echo_need_zlp = (length == max_packet);
        echo_idle_polls = 0;
    }
    else
    {
        echo_need_zlp = 0;
    }
    echo_tx = ECHO_TX_IDLE;
    if( echo_rx_paused )
    {
        Echo_ArmReceive( );
    }
    Echo_StartTransmit( );
}

void USBHS_Echo_Poll( void )
{
    /* Called once per millisecond; finish a full-packet burst only after idle. */
    NVIC_DisableIRQ( USBHS_IRQn );
    if( (USBHSD->INT_FG & USBHS_UIF_TRANSFER) == 0 )
    {
        if( echo_rx_paused )
        {
            Echo_ArmReceive( );
        }
        Echo_StartTransmit( );
        if( echo_need_zlp && echo_tx == ECHO_TX_IDLE && echo_count == 0 &&
            (USBHSD->UEP2_TX_CTRL & USBHS_UEP_T_RES_MASK) != USBHS_UEP_T_RES_STALL )
        {
            if( ++echo_idle_polls >= 2U )
            {
                echo_need_zlp = 0;
                echo_tx = ECHO_TX_ZLP;
                USBHSD->UEP2_TX_LEN = 0;
                USBHSD->UEP2_TX_CTRL = (USBHSD->UEP2_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_ACK;
            }
        }
    }
    NVIC_EnableIRQ( USBHS_IRQn );
}

void USBHS_Echo_ClearHalt( uint8_t input )
{
    if( input )
    {
        USBHSD->UEP2_TX_CTRL = USBHS_UEP_T_RES_NAK;
        if( echo_tx != ECHO_TX_IDLE )
        {
            USBHSD->UEP2_TX_CTRL = USBHS_UEP_T_RES_ACK;
        }
        else
        {
            Echo_StartTransmit( );
        }
    }
    else
    {
        USBHSD->UEP2_RX_CTRL = USBHS_UEP_R_RES_NAK;
        Echo_ArmReceive( );
    }
}
