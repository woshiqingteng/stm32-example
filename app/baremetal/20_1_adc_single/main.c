/**
 * @file    main.c
 * @brief   20_1_adc_single: polled ADC1_IN5 (PA5) sampling.
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"

#define ADC_AVG_COUNT         10U
#define ADC_AVG_DELAY_MS      5U
#define ADC_VREF_MV           3300U
#define ADC_FULL_SCALE_COUNT  4096U
#define ADC_SAMPLE_PERIOD_MS  100U   /* plus ADC_AVG_COUNT * ADC_AVG_DELAY_MS below */
#define BLINK_TICKS           10U    /* LED0 toggle interval, in loop iterations */

int main(void)
{
    uint32_t blink = 0U;

    bsp_init();
    adc_init(NULL);
    printf(APP_BANNER "\r\n");

    for (;;)
    {
        uint32_t sum = 0U;
        uint32_t i;
        uint32_t raw;
        uint32_t mv;

        for (i = 0U; i < ADC_AVG_COUNT; i++)
        {
            sum += adc_read(ADC_CH5);
            delay_ms(ADC_AVG_DELAY_MS);
        }
        raw = sum / ADC_AVG_COUNT;
        mv  = (raw * ADC_VREF_MV) / ADC_FULL_SCALE_COUNT;

        printf("ch5 raw:%u vol:%lu.%03luV\r\n",
               (unsigned int)raw,
               (unsigned long)(mv / 1000U),
               (unsigned long)(mv % 1000U));

        if ((++blink % BLINK_TICKS) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(ADC_SAMPLE_PERIOD_MS);
    }
}
