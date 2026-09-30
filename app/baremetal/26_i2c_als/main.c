/**
 * @file    main.c
 * @brief   26_i2c_als: AP3216C ambient light / proximity sensor test. The raw
 *          IR, PS and ALS channels are printed over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "als.h"

#define SAMPLE_PERIOD_MS   120U

int main(void)
{
    uint16_t ir;
    uint16_t ps;
    uint16_t als;

    bsp_init();
    printf(APP_BANNER "\r\n");

    if (als_init() != 0U)
    {
        printf("AP3216C check failed\r\n");
    }
    else
    {
        printf("AP3216C ready\r\n");
    }

    printf("26_i2c_als ready\r\n");

    for (;;)
    {
        als_read(&ir, &ps, &als);

        printf("IR:%u PS:%u ALS:%u\r\n", (unsigned)ir, (unsigned)ps, (unsigned)als);

        led_toggle(LED0);
        delay_ms(SAMPLE_PERIOD_MS);
    }
}
