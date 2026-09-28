/**
 * @file    main.c
 * @brief   10_tpad: capacitive touch key toggles LED1.
 */

#include <stdio.h>

#include "bsp.h"

#define TPAD_LOOP_MS 10U

int main(void)
{
    uint32_t blink = 0;

    bsp_init();
    printf(APP_BANNER "\r\n");

    if (tpad_init(2) != TPAD_OK)
    {
        printf("tpad init failed\r\n");
    }

    for (;;)
    {
        if (tpad_scan(false))
        {
            led_toggle(LED1);
        }

        if ((++blink % 15U) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(TPAD_LOOP_MS);
    }
}
