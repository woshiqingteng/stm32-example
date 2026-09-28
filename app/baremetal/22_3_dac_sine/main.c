/**
 * @file    main.c
 * @brief   22_3_dac_sine: DAC1 channel 1 sine wave (run-time generated table)
 *          driven by TIM7 TRGO + DMA1. KEY0 switches between ~3 kHz and
 *          ~30 kHz; an ADC read-back of the pin is printed over USART1.
 */

#include <stdio.h>

#include "bsp.h"

#define SIN_TIMER_ARR_TICK   9U

static const uint16_t g_sin_psc[] = { 29U, 2U };
static const char *const g_sin_label[] = { "~3kHz", "~30kHz" };

static void dac_sine_show(uint8_t idx)
{
    uint32_t adc = adc_get_result_average(ADC_CH4, 10U);

    printf("sine %s  ADC(Pin4): %lu\r\n", g_sin_label[idx], (unsigned long)adc);
}

int main(void)
{
    uint8_t idx = 0U;

    bsp_init();
    printf(APP_BANNER "\r\n");
    adc_init();

    dac_sine_init(SIN_TIMER_ARR_TICK, g_sin_psc[idx]);
    dac_sine_start();

    printf("22_3_dac_sine ready (KEY0 switches frequency)\r\n");
    dac_sine_show(idx);

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            idx ^= 1U;

            dac_sine_stop();
            dac_sine_init(SIN_TIMER_ARR_TICK, g_sin_psc[idx]);
            dac_sine_start();

            dac_sine_show(idx);
        }

        led_toggle(LED0);
        delay_ms(1000U);
    }
}
