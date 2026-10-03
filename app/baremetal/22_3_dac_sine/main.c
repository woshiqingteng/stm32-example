/**
 * @file    main.c
 * @brief   22_3_dac_sine: DAC1 channel 1 sine wave (run-time generated table)
 *          driven by TIM7 TRGO + DMA1. KEY0 switches between ~3 kHz and
 *          ~30 kHz; an ADC read-back of the pin is printed over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"
#include "dac.h"

#define SIN_TIMER_ARR   9U

/* 90 MHz/((PSC+1)*(ARR+1))/100: PSC=29 -> 3 kHz, PSC=2 -> 30 kHz. */
static const uint16_t g_sin_psc[] = { 29U, 2U };
static const char *const g_sin_label[] = { "~3kHz", "~30kHz" };

#define ADC_AVG_COUNT     10U
#define ADC_AVG_DELAY_MS  5U

static uint32_t adc_read_avg(adc_channel_t ch)
{
    uint32_t sum = 0U;
    uint32_t i;

    for (i = 0U; i < ADC_AVG_COUNT; i++)
    {
        sum += adc_read(ADC_ID_1, ch);
        delay_ms(ADC_AVG_DELAY_MS);
    }
    return sum / ADC_AVG_COUNT;
}

static void dac_sine_show(uint8_t idx)
{
    uint32_t adc = adc_read_avg(ADC_CH4);

    printf("sine %s  ADC(Pin4): %lu\r\n", g_sin_label[idx], (unsigned long)adc);
}

int main(void)
{
    uint8_t idx = 0U;

    bsp_init();
    printf(APP_BANNER "\r\n");
    adc_init(NULL);

    dac_sine_init(SIN_TIMER_ARR, g_sin_psc[idx]);
    dac_sine_start();

    printf("KEY0: switch frequency\r\n");
    dac_sine_show(idx);

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            idx ^= 1U;

            dac_sine_stop();
            dac_sine_init(SIN_TIMER_ARR, g_sin_psc[idx]);
            dac_sine_start();

            dac_sine_show(idx);
        }

        led_toggle(LED0);
        delay_ms(1000U);
    }
}
