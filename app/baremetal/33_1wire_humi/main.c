/**
 * @file    main.c
 * @brief   33_1wire_humi: DHT11 temperature / humidity test. A measurement is read
 *          periodically and printed over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "humi.h"

#define DHT11_PERIOD_MS 1000U

int main(void)
{
    uint8_t temperature = 0U;
    uint8_t humidity    = 0U;

    bsp_init();

    if (humi_init() != 0U)
    {
        printf("DHT11 not found!\r\n");

        for (;;)
        {
            led_toggle(LED0);
            delay_ms(500U);
        }
    }

    printf(APP_BANNER "\r\n");

    for (;;)
    {
        if (humi_read(&temperature, &humidity) != 0U)
        {
            printf("DHT11 read failed\r\n");
        }
        else
        {
            printf("Temp: %u C  Humi: %u %%\r\n", temperature, humidity);
        }

        led_toggle(LED0);
        delay_ms(DHT11_PERIOD_MS);
    }
}
