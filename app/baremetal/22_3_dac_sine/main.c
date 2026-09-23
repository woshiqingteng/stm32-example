/**
 * @file    main.c
 * @brief   22_3_dac_sine: DAC1 channel 1 sine wave generated from a 100-point
 *          table by TIM7 TRGO + DMA1 (circular).
 */

#include <stdio.h>
#include "bsp.h"

#define SIN_TIMER_ARR 9U
#define SIN_TIMER_PSC 29U

int main(void)
{
    bsp_init();

    dac_sine_init(SIN_TIMER_ARR, SIN_TIMER_PSC);
    dac_sine_start();

    printf("22_3_dac_sine ready, ~3kHz sine on PA4\r\n");

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500U);
    }
}
