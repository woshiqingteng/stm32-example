/**
 * @file    main.c
 * @brief   20_1_adc: polled ADC1_IN5 (PA5) sampling.
 */

#include <stdio.h>
#include "bsp.h"

#define ADC_AVG_TIMES  10U
#define ADC_VREF_MV    3300U
#define ADC_VREF_UV    (ADC_VREF_MV * 1000U)
#define ADC_FULL_SCALE 4096U
#define BLINK_PERIOD   10U

int main(void)
{
    uint32_t blink = 0U;

    bsp_init();
    adc_init();
    printf("20_1_adc ready\r\n");

    for (;;)
    {
        uint32_t raw   = adc_get_result_average(ADC_CH5, ADC_AVG_TIMES);
        uint32_t micro = (uint32_t)(((uint64_t)raw * ADC_VREF_UV) / ADC_FULL_SCALE);

        printf("ch5 raw:%u vol:%lu.%03luV\r\n",
               (unsigned int)raw,
               (unsigned long)(micro / 1000000U),
               (unsigned long)((micro % 1000000U) / 1000U));

        if ((++blink % BLINK_PERIOD) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(100);
    }
}
