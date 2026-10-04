/**
 * @file    main.c
 * @brief   26_i2c_als: AP3216C ambient light / proximity sensor test. The raw
 *          IR, PS and ALS channels are printed over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "als.h"

#define SAMPLE_PERIOD_MS   200U

/* Read one sample and report the raw IR (infrared), PS (proximity) and ALS
 * (ambient light) channel counts. */
static void als_show(void)
{
    uint16_t ir;
    uint16_t ps;
    uint16_t als;

    als_read(&ir, &ps, &als);

    printf("IR:%u PS:%u ALS:%u\r\n", ir, ps, als);
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");

    if (als_init() != 0U)
    {
        printf("AP3216C check failed\r\n");
    }

    for (;;)
    {
        als_show();
        led_toggle(LED0);
        delay_ms(SAMPLE_PERIOD_MS);
    }
}
