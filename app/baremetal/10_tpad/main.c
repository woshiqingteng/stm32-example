/**
 * @file    main.c
 * @brief   10_tpad: capacitive touch key toggles LED1.
 */

#include <stdio.h>
#include "bsp.h"

int main(void)
{
    uint32_t blink = 0;

    bsp_init();

    if (tpad_init(2) != 0U)
    {
        printf("tpad init failed\r\n");
    }

    for (;;)
    {
        if (tpad_scan(false) != 0U)
        {
            led_toggle(LED1);
        }

        if ((++blink % 15U) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(10);
    }
}
