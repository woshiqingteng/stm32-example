/**
 * @file    main.c
 * @brief   18_3_stop: KEY0 enters stop mode, WK_UP wakes the MCU.
 */

#include <stdio.h>
#include "bsp.h"

#define LOOP_DELAY_MS 10U

int main(void)
{
    uint32_t t = 0U;

    bsp_init();

    pwr_wkup_key_init();
    printf("KEY0: enter stop mode, WK_UP: wake\r\n");

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            printf("Entering stop mode...\r\n");
            led_on(LED1);

            pwr_enter_stop();

            sys_clk_init(360U, 25U, 2U, 8U);
            led_off(LED1);
            printf("Woke from stop mode\r\n");
        }

        if ((++t % 20U) == 0U)
        {
            led_toggle(LED0);
        }
        delay_ms(LOOP_DELAY_MS);
    }
}
