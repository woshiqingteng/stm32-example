/**
 * @file    main.c
 * @brief   20_2_adc_dma: single-channel (PA5) circular DMA acquisition with
 *          half/full double buffering.
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"

#define ADC_DMA_BUF_LEN_SAMPLE  50U
#define ADC_HALF_LEN_SAMPLE     (ADC_DMA_BUF_LEN_SAMPLE / 2U)
#define ADC_VREF_MV             3300U
#define ADC_FULL_SCALE_COUNT    4096U
#define ADC_LOOP_MS             200U

/* Timing: ADCCLK = PCLK2/4 = 90/4 = 22.5 MHz; Tconv = (480+12)/22.5MHz ~= 21.9us
 * -> ~45.7 kSa/s; a half (25 samples) fills every ~0.55 ms, a full buffer ~1.1 ms.
 * The 200 ms loop refreshes at ~5 Hz and keeps only the latest completed half. */

static uint16_t            g_adc_buf[ADC_DMA_BUF_LEN_SAMPLE];
static volatile uint16_t   g_adc_offset;
static volatile bool       g_adc_ready;

static void on_adc_dma(uint16_t offset)
{
    g_adc_offset = offset;
    g_adc_ready  = true;
}

static void adc_show(void)
{
    uint16_t half_off = g_adc_offset;   /* snapshot: DMA may switch halves mid-loop */
    uint32_t sum = 0U;
    uint32_t i;
    uint32_t raw;
    uint32_t mv;

    for (i = 0U; i < ADC_HALF_LEN_SAMPLE; i++)
    {
        sum += g_adc_buf[half_off + i];
    }
    raw = sum / ADC_HALF_LEN_SAMPLE;
    mv  = (raw * ADC_VREF_MV) / ADC_FULL_SCALE_COUNT;

    printf("dma raw:%u vol:%lu.%03luV\r\n",
           (unsigned int)raw,
           (unsigned long)(mv / 1000U),
           (unsigned long)(mv % 1000U));
}

int main(void)
{
    adc_cfg_t cfg   = { ADC_CFG_DEFAULT };

    bsp_init();

    cfg.mode        = ADC_MODE_DMA;
    cfg.dma_mode    = ADC_DMA_CIRCULAR;
    cfg.chans       = (const adc_channel_t[]){ ADC_CH5 };
    cfg.nchans      = 1U;
    cfg.dma_buf     = g_adc_buf;
    cfg.dma_len     = ADC_DMA_BUF_LEN_SAMPLE;
    cfg.dma_half_cb = true;
    cfg.dma_cb      = on_adc_dma;
    adc_init(&cfg);

    printf(APP_BANNER "\r\n");

    for (;;)
    {
        if (g_adc_ready)
        {
            g_adc_ready = false;
            adc_show();
        }

        led_toggle(LED0);          /* run indicator */
        delay_ms(ADC_LOOP_MS);
    }
}
