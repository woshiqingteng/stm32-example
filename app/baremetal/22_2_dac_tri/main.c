/**
 * @file    main.c
 * @brief   22_2_dac_tri: DAC1 channel 1 triangle wave generated from a buffer
 *          by TIM6 TRGO + DMA1 (circular).
 */

#include <stdio.h>
#include "bsp.h"

#define TRI_TIMER_ARR 899U
#define TRI_TIMER_PSC 0U

int main(void)
{
    bsp_init();

    dac_triangle_init(TRI_TIMER_ARR, TRI_TIMER_PSC);
    dac_triangle_start();

    printf("22_2_dac_tri ready, ~1kHz triangle on PA4\r\n");

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}
