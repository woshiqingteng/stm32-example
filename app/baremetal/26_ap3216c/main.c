/**
 * @file    main.c
 * @brief   26_ap3216c: AP3216C ambient light / proximity sensor test. The raw
 *          IR, PS and ALS channels are printed over USART1.
 */

#include <stdio.h>
#include "bsp.h"

#define SAMPLE_PERIOD   120U

int main(void)
{
    char line[48];
    uint16_t ir;
    uint16_t ps;
    uint16_t als;

    bsp_init();

    if (ap3216c_init() != 0U)
    {
        printf("AP3216C check failed\r\n");
    }
    else
    {
        printf("AP3216C ready\r\n");
    }

    printf("26_ap3216c ready\r\n");

    for (;;)
    {
        ap3216c_read_data(&ir, &ps, &als);

        sprintf(line, "IR:%u PS:%u ALS:%u", (unsigned)ir, (unsigned)ps, (unsigned)als);
        printf("%s\r\n", line);

        led_toggle(LED0);
        delay_ms(SAMPLE_PERIOD);
    }
}
