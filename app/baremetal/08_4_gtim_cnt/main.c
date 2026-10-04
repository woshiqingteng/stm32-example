/**
 * @file    main.c
 * @brief   08_4_gtim_cnt: TIM2_CH1 (PA0) external pulse counter; KEY0 restarts.
 */

#include <stdio.h>

#include "bsp.h"
#include "tim.h"

#define GTIM_CNT_PSC       0U
/* PSC=0: external edges counted 1:1; 32-bit CNT, 64-bit total */
#define GTIM_CNT_BLINK_COUNT 20U
#define GTIM_CNT_LOOP_MS   10U

/* Print a 64-bit value in decimal without relying on %llu (nano.specs). */
static void print_u64(uint64_t value)
{
    char     buf[20];
    uint32_t i = 0U;

    if (value == 0U)
    {
        printf("0");
        return;
    }

    while ((value != 0U) && (i < sizeof(buf)))
    {
        buf[i++] = (char)('0' + (uint32_t)(value % 10U));
        value /= 10U;
    }

    while (i > 0U)
    {
        printf("%c", buf[--i]);
    }
}

int main(void)
{
    tim_cfg_t cfg = { TIM_CFG_DEFAULT };
    uint32_t old_count = 0;
    uint32_t blink = 0;

    bsp_init();
    printf(APP_BANNER "\r\n");

    cfg.id      = TIM_ID_2;
    cfg.mode    = TIM_MODE_COUNTER;
    cfg.channel = TIM_CH1;
    cfg.pull    = TIM_PULL_DOWN;
    cfg.arr     = 0xFFFFFFFFU;
    cfg.psc     = GTIM_CNT_PSC;
    tim_init(&cfg);
    tim_set(TIM_ID_2, TIM_CH1, TIM_PARAM_COUNT, 0U);
    printf("KEY0: restart count\r\n");

    for (;;)
    {
        uint32_t count;

        if (key_scan(false) == KEY0)
        {
            tim_set(TIM_ID_2, TIM_CH1, TIM_PARAM_COUNT, 0U);
        }

        count = tim_get(TIM_ID_2, TIM_CH1, TIM_PARAM_COUNT);
        if (count != old_count)
        {
            printf("CNT:");
            print_u64(count);
            printf("\r\n");
            old_count = count;
        }

        if ((++blink % GTIM_CNT_BLINK_COUNT) == 0U)
        {
            led_toggle(LED0);
        }

        delay_ms(GTIM_CNT_LOOP_MS);
    }
}
