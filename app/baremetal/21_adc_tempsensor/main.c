/**
 * @file    main.c
 * @brief   21_adc_tempsensor: internal temperature sensor read (ADC1 channel 18).
 *          Temperature is reported as degrees * 100 over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"

#define TEMP_SAMPLE_PERIOD_MS 500U
#define TEMP_AVG_COUNT        10U
#define TEMP_AVG_DELAY_MS     5U
#define ADC_VREF_MV           3300U
#define ADC_FULL_SCALE_COUNT  4096U
#define TEMP_V25_MV           760     /* datasheet: 0.76 V at 25 C */
#define TEMP_UV_PER_C         2500    /* datasheet: 2.5 mV/C */

static void temp_show(void)
{
    uint32_t sum = 0U;
    uint32_t i;
    uint32_t mv;
    int32_t  temp100;
    int32_t  temp_abs;

    for (i = 0U; i < TEMP_AVG_COUNT; i++)
    {
        sum += adc_read(ADC_ID_1, ADC_TEMP_CH);
        delay_ms(TEMP_AVG_DELAY_MS);
    }
    mv = sum * ADC_VREF_MV / (TEMP_AVG_COUNT * ADC_FULL_SCALE_COUNT);

    /* temp = (mv - 760) / 2.5 + 25  ->  temp*100 = (mv - 760) * 40 + 2500 */
    temp100  = ((int32_t)mv - TEMP_V25_MV) * (100000 / TEMP_UV_PER_C) + 2500;
    temp_abs = (temp100 < 0) ? -temp100 : temp100;

    printf("TEMP: %s%ld.%02ldC\r\n", (temp100 < 0) ? "-" : "",
           (long)(temp_abs / 100), (long)(temp_abs % 100));
}

int main(void)
{
    bsp_init();
    adc_init(NULL);

    printf(APP_BANNER "\r\n");

    for (;;)
    {
        temp_show();
        led_toggle(LED0);
        delay_ms(TEMP_SAMPLE_PERIOD_MS);
    }
}
