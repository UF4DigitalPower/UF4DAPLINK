#ifndef USBHS_ECHO_H
#define USBHS_ECHO_H

#include <stdint.h>

#define USBHS_ECHO_PACKET_SIZE 512U
#define USBHS_ECHO_SLOT_COUNT  64U

void USBHS_Echo_Init( void );
void USBHS_Echo_Receive( uint16_t length );
void USBHS_Echo_TransmitComplete( void );
void USBHS_Echo_ClearHalt( uint8_t input );
void USBHS_Echo_Poll( void );

#endif
