#ifndef USER_SWD_SPI_H_
#define USER_SWD_SPI_H_

#include "ch32v30x.h"

/* UF4 control bits are asserted-high at the CH32V307 pins. */
#define UF4_CTRL_G4_NRST   0x01u
#define UF4_CTRL_WIFI_BOOT 0x02u
#define UF4_CTRL_WIFI_NRST 0x04u

void SWD_SPI_Init(void);
void SWD_SPI_SetClock(uint32_t hz);
uint32_t SWD_SPI_GetClock(void);
uint8_t SWD_SPI_Connect(void);
uint8_t SWD_SPI_Transfer(uint8_t request, uint32_t *data);
void SWD_SPI_ResetSequence(void);
void SWD_SPI_SwjSequence(const uint8_t *data, uint16_t bit_count);

void SWD_SetTargetReset(uint8_t asserted);
void SWD_SetWifiBoot(uint8_t asserted);
void SWD_SetWifiReset(uint8_t asserted);
uint8_t SWD_GetControlState(void);

#endif
