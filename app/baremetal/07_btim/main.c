/**
 * @file    main.c
 * @brief   07_btim: TIM6 500 ms interrupt toggles LED1.
 */

#include <stdio.h>

#include "bsp.h"
#include "tim.h"

#define BTIM_ARR 5000U
#define BTIM_PSC 9000U
/* 90 MHz / (9000 * 5000) = 2 Hz -> 500 ms */
#define BTIM_LOOP_MS   200U

static void on_tim6(void)
{
    led_toggle(LED1);
}

int main(void)
{
    tim_cfg_t cfg = { TIM_CFG_DEFAULT };

    bsp_init();
    printf(APP_BANNER "\r\n");

    cfg.id        = TIM_ID_6;
    cfg.mode      = TIM_MODE_BASE;
    cfg.arr       = BTIM_ARR - 1U;
    cfg.psc       = BTIM_PSC - 1U;
    cfg.update_cb = &on_tim6;
    tim_init(&cfg);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(BTIM_LOOP_MS);
    }
}
