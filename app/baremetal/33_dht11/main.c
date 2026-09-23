/**
 * @file    main.c
 * @brief   33_dht11: DHT11 temperature / humidity test. A measurement is read
 *          periodically and printed over USART1.
 */

#include <stdio.h>
#include "bsp.h"

#define DHT11_PERIOD_MS 1000U

int main(void)
{
    uint8_t temperature = 0U;
    uint8_t humidity    = 0U;
    char    line[48];

    bsp_init();

    if (dht11_init() != 0U)
    {
        printf("DHT11 not found!\r\n");

        for (;;)
        {
            led_toggle(LED0);
            delay_ms(500U);
        }
    }

    printf("33_dht11 ready\r\n");

    for (;;)
    {
        if (dht11_read_data(&temperature, &humidity) != 0U)
        {
            printf("DHT11 read failed\r\n");
        }
        else
        {
            sprintf(line, "Temp: %u C  Humi: %u %%", temperature, humidity);
            printf("%s\r\n", line);
        }

        led_toggle(LED0);
        delay_ms(DHT11_PERIOD_MS);
    }
}
