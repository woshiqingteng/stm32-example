/**
 * @file    main.c
 * @brief   32_ds18b20: DS18B20 1-Wire temperature test. The temperature is read
 *          periodically and printed over USART1 (tenths of a degree
 *          resolution).
 */

#include <stdio.h>
#include "bsp.h"

#define TEMP_PERIOD_MS  500U

int main(void)
{
    int16_t temperature;
    int16_t shown;
    char    line[48];

    bsp_init();

    if (ds18b20_init() != 0U)
    {
        printf("DS18B20 not found!\r\n");

        for (;;)
        {
            led_toggle(LED0);
            delay_ms(500U);
        }
    }

    printf("32_ds18b20 ready\r\n");

    for (;;)
    {
        temperature = ds18b20_get_temperature();
        shown = (temperature < 0) ? (int16_t)(-temperature) : temperature;

        sprintf(line, "Temp: %s%d.%d C", (temperature < 0) ? "-" : "",
                shown / 10, shown % 10);
        printf("%s\r\n", line);

        led_toggle(LED0);
        delay_ms(TEMP_PERIOD_MS);
    }
}
