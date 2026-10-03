/**
 * @file    main.c
 * @brief   22_3_dac_sine: DAC1 channel 1 sine wave (application-generated
 *          buffer) driven by TIM7 TRGO + DMA1. KEY0 switches between ~3 kHz and
 *          ~30 kHz; an ADC read-back of the pin is printed over USART1.
 */

#include <math.h>
#include <stdio.h>

#include "bsp.h"
#include "adc.h"
#include "dac.h"

#define SIN_TIMER_ARR 9U
#define SIN_WAVE_LEN  100U
#define SIN_PI        3.14159265f

/* 90 MHz/((PSC+1)*(ARR+1))/100: PSC=29 -> 3 kHz, PSC=2 -> 30 kHz. */
static const uint16_t g_sin_psc[] = { 29U, 2U };
static const char *const g_sin_label[] = { "~3kHz", "~30kHz" };

static uint16_t g_sin_buf[SIN_WAVE_LEN];

#define APP_LOOP_MS       10U
#define ADC_AVG_COUNT     10U
#define ADC_AVG_DELAY_MS  5U
#define LED_BLINK_MS      500U
#define LED_BLINK_TICKS   (LED_BLINK_MS / APP_LOOP_MS)

/* One full sine period: 2048 * (1 + sin). */
static void sin_build(void)
{
    uint16_t i;

    for (i = 0U; i < SIN_WAVE_LEN; i++)
    {
        g_sin_buf[i] = (uint16_t)(2048.0f +
                       2047.0f * sinf(2.0f * SIN_PI * (float)i / (float)SIN_WAVE_LEN));
    }
}

static void dac_sine_show(uint8_t idx)
{
    uint32_t sum = 0U;
    uint32_t i;
    uint32_t adc;

    for (i = 0U; i < ADC_AVG_COUNT; i++)
    {
        sum += adc_read(ADC_ID_1, ADC_CH5);
        delay_ms(ADC_AVG_DELAY_MS);
    }
    adc = sum / ADC_AVG_COUNT;

    printf("sine %s  ADC(Pin5): %lu\r\n", g_sin_label[idx], (unsigned long)adc);
}

int main(void)
{
    uint8_t   idx   = 0U;
    uint32_t  blink = 0U;
    dac_cfg_t cfg   = { DAC_CFG_DEFAULT };

    bsp_init();
    printf(APP_BANNER "\r\n");
    adc_init(NULL);

    sin_build();
    cfg.mode    = DAC_MODE_WAVE;
    cfg.channel = DAC_CH1;
    cfg.buf     = g_sin_buf;
    cfg.len     = SIN_WAVE_LEN;
    cfg.timer   = DAC_TIMER_7;
    cfg.arr     = SIN_TIMER_ARR;
    cfg.psc     = g_sin_psc[idx];
    dac_init(&cfg);
    dac_start();

    printf("KEY0: switch frequency\r\n");
    dac_sine_show(idx);

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            idx ^= 1U;

            dac_stop();
            cfg.psc = g_sin_psc[idx];
            dac_init(&cfg);
            dac_start();

            dac_sine_show(idx);
            led_toggle(LED1);      /* action indicator */
        }

        if ((++blink % LED_BLINK_TICKS) == 0U)
        {
            led_toggle(LED0);      /* run indicator, ~500 ms */
        }
        delay_ms(APP_LOOP_MS);
    }
}
