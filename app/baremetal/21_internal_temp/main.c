/**
 * @file    main.c
 * @brief   21_internal_temp: internal temperature sensor read (ADC1 channel 18).
 *          Temperature is reported as degrees * 100 over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"

#define TEMP_SAMPLE_PERIOD_MS 500U

int main(void)
{
    bsp_init();
    adc_temp_init();

    printf(APP_BANNER "\r\n");

    for (;;)
    {
        int16_t temp = adc_get_temperature();
        int16_t temp_abs = (temp < 0) ? (int16_t)(-temp) : temp;

        printf("TEMP: %s%d.%02dC\r\n", (temp < 0) ? "-" : "",
               (int)(temp_abs / 100), (int)(temp_abs % 100));

        led_toggle(LED0);
        delay_ms(TEMP_SAMPLE_PERIOD_MS);
    }
}
