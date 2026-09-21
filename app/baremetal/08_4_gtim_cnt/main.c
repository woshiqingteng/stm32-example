/**
 * @file    main.c
 * @brief   08_4_gtim_cnt: TIM2_CH1 (PA0) external pulse counter; KEY0 restarts.
 */

#include <stdio.h>
#include "bsp.h"

#define GTIM_CNT_PSC       0U
#define GTIM_CNT_BLINK_DIV 20U
#define GTIM_CNT_LOOP_MS   10U

int main(void)
{
    uint32_t old_count = 0;
    uint32_t blink = 0;

    bsp_init();
    gtim_timx_cnt_chy_init(GTIM_CNT_PSC);
    gtim_timx_cnt_chy_restart();

    for (;;)
    {
        uint32_t count;

        if (key_scan(false) == KEY0)
        {
            gtim_timx_cnt_chy_restart();
        }

        count = gtim_timx_cnt_chy_get_count();
        if (count != old_count)
        {
            printf("CNT:%lu\r\n", (unsigned long)count);
            old_count = count;
        }

        if ((++blink % GTIM_CNT_BLINK_DIV) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(GTIM_CNT_LOOP_MS);
    }
}
