/**
 * @file    main.c
 * @brief   09_3_atim_cplm: TIM1 complementary PWM with dead time (PE9/PE8).
 */

#include <stdio.h>

#include "bsp.h"
#include "tim.h"

#define ATIM_CPLM_ARR     1000U
#define ATIM_CPLM_PSC     180U
#define ATIM_CPLM_CCR     300U
#define ATIM_CPLM_DTG     100U
/* 180 MHz/(180*1000) = 1 kHz; OCxREF duty = 300/1000 = 30%. Both outputs
 * active-low: PE9 high ~70%, PE8 high ~30% (7:3).
 *
 * Dead time = f(BDTR.DTG[7:0]), t_DTS = CKD*t_CK_INT = 4/180 MHz = 22.22 ns:
 *   DTG[7:5]=0xx -> DT = DTG[7:0]*t_DTS
 *   DTG[7:5]=10x -> DT = (64+DTG[5:0])*2*t_DTS
 *   DTG[7:5]=110 -> DT = (32+DTG[4:0])*8*t_DTS
 *   DTG[7:5]=111 -> DT = (32+DTG[4:0])*16*t_DTS
 * DTG=100 (0xx)             : 100*22.22 ns = 2.22 us
 * DTG=250 (111, DTG[4:0]=26): (32+26)*16*22.22 ns = 20.62 us */
#define ATIM_CPLM_LOOP_MS 500U

int main(void)
{
    tim_cfg_t cfg = { TIM_CFG_DEFAULT };

    bsp_init();
    printf(APP_BANNER "\r\n");

    cfg.id       = TIM_ID_1;
    cfg.mode     = TIM_MODE_CPLM;
    cfg.channel  = TIM_CH1;
    cfg.polarity = TIM_POL_LOW;
    cfg.pull     = TIM_PULL_UP;
    cfg.arr      = ATIM_CPLM_ARR - 1U;
    cfg.psc      = ATIM_CPLM_PSC - 1U;
    tim_init(&cfg);
    tim_set(TIM_ID_1, TIM_CH1, TIM_PARAM_CCR, ATIM_CPLM_CCR);
    tim_set(TIM_ID_1, TIM_CH1, TIM_PARAM_DTG, ATIM_CPLM_DTG);

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(ATIM_CPLM_LOOP_MS);
    }
}
