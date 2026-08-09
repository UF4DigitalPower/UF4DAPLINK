#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the VTREF ADC placeholder channel. */
void Board_ADC_Init(void);

/** Read VTREF in millivolts, applying the configured divider ratio. */
uint16_t Board_ADC_ReadVTREF_mV(void);

#ifdef __cplusplus
}
#endif

#endif /* ADC_H */
