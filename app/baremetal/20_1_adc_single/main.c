/**
 * @file    main.c
 * @brief   20_1_adc_single: polled ADC1_IN5 (PA5) sampling.
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"

#define ADC_AVG_COUNT         10U
#define ADC_VREF_MV           3300U
#define ADC_VREF_UV           (ADC_VREF_MV * 1000U)
#define ADC_UV_PER_VOLT       1000000U
#define ADC_UV_PER_MV         1000U
#define ADC_FULL_SCALE_COUNT        4096U
#define ADC_SAMPLE_PERIOD_MS  100U
#define BLINK_PERIOD_MS          10U

int main(void)
{
    uint32_t blink = 0U;

    bsp_init();
    adc_init();
    printf(APP_BANNER "\r\n");

    for (;;)
    {
        uint32_t raw   = adc_get_result_average(ADC_CH5, ADC_AVG_COUNT);
        uint32_t micro = (uint32_t)(((uint64_t)raw * ADC_VREF_UV) / ADC_FULL_SCALE_COUNT);

        printf("ch5 raw:%u vol:%lu.%03luV\r\n",
               (unsigned int)raw,
               (unsigned long)(micro / ADC_UV_PER_VOLT),
               (unsigned long)((micro % ADC_UV_PER_VOLT) / ADC_UV_PER_MV));

        if ((++blink % BLINK_PERIOD_MS) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(ADC_SAMPLE_PERIOD_MS);
    }
}
