#include "adc.h"
#include "board.h"
#include "board_config.h"
#include "../Core/Inc/adc.h"

#include <stdbool.h>
#include <stdint.h>

#define ADC_READ_TIMEOUT_MS    1u
#define ADC_SAMPLE_COUNT       8u

void Board_ADC_Init(void)
{
    /* MX_ADC1_Init() already configures PA4 as ADC1_IN4. Nothing else to do. */
}

static bool Board_ADC_ReadRaw(uint16_t *value)
{
    bool ok = false;

    if ((value == 0) || (hadc1.Instance == 0)) {
        return false;
    }

    *value = 0u;
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return false;
    }

    if (HAL_ADC_PollForConversion(&hadc1, ADC_READ_TIMEOUT_MS) == HAL_OK) {
        *value = (uint16_t)HAL_ADC_GetValue(&hadc1);
        ok = true;
    }

    (void)HAL_ADC_Stop(&hadc1);
    return ok;
}

uint16_t Board_ADC_ReadVTREF_mV(void)
{
    uint32_t sum = 0u;
    uint32_t count = 0u;
    uint64_t mv;
    uint32_t divider_num = VTREF_R_TOP + VTREF_R_BOTTOM;
    uint32_t divider_den = VTREF_R_BOTTOM;

    if ((divider_num == 0u) || (divider_den == 0u)) {
        divider_num = 2u;
        divider_den = 1u;
    }

    for (uint32_t i = 0u; i < ADC_SAMPLE_COUNT; i++) {
        uint16_t sample = 0u;
        if (!Board_ADC_ReadRaw(&sample)) {
            break;
        }
        sum += sample;
        count++;
    }

    if (count == 0u) {
        return 0u;
    }

    mv = (uint64_t)(sum / count);
    mv = (mv * ADC_VREF_MV * divider_num) /
         ((uint64_t)ADC_MAX_COUNTS * divider_den);

    if (mv > UINT16_MAX) {
        mv = UINT16_MAX;
    }

    return (uint16_t)mv;
}
