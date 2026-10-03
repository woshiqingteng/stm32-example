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
#define SIN_TIMER_PSC 29U
#define SIN_MAX_LEN   100U
#define SIN_PI        3.14159265f

/* f_trgo = 90 MHz/((29+1)*(9+1)) = 300 kHz; wave = f_trgo / samples:
 * 100 samples -> 3 kHz, 10 samples -> 30 kHz (keeps the DAC update rate at
 * 300 kHz, within its settling limit, instead of 3 MHz). */
static const uint16_t g_sin_len[] = { 100U, 10U };
static const char *const g_sin_label[] = { "~3kHz", "~30kHz" };

static uint16_t g_sin_buf[SIN_MAX_LEN];

#define APP_LOOP_MS       10U
#define ADC_AVG_COUNT     10U
#define ADC_AVG_DELAY_MS  5U
#define LED_BLINK_MS      500U
#define LED_BLINK_TICKS   (LED_BLINK_MS / APP_LOOP_MS)

/* One full sine period over @p len samples: 2048 * (1 + sin). */
static void sin_build(uint16_t len)
{
    uint16_t i;

    for (i = 0U; i < len; i++)
    {
        g_sin_buf[i] = (uint16_t)(2048.0f +
                       2047.0f * sinf(2.0f * SIN_PI * (float)i / (float)len));
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

    sin_build(g_sin_len[idx]);
    cfg.mode          = DAC_MODE_WAVE;
    cfg.channel       = DAC_CH1;
    cfg.buffer_enable = true;
    cfg.buf           = g_sin_buf;
    cfg.len           = g_sin_len[idx];
    cfg.timer         = DAC_TIMER_7;
    cfg.arr           = SIN_TIMER_ARR;
    cfg.psc           = SIN_TIMER_PSC;
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
            sin_build(g_sin_len[idx]);
            cfg.len = g_sin_len[idx];
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
