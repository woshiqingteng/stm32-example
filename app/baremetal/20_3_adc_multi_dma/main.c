/**
 * @file    main.c
 * @brief   20_3_adc_multi_dma: 6-channel ADC1 scan DMA acquisition (PA0..PA5).
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"

#define ADC_SCAN_SAMPLE_COUNT   50U
#define ADC_DMA_BUF_LEN_SAMPLE  (ADC_SCAN_SAMPLE_COUNT * ADC_SCAN_CH_NUM)
#define ADC_VREF_MV             3300U
#define ADC_FULL_SCALE_COUNT    4096U
#define ADC_LOOP_MS             200U

/* Timing: ADCCLK = PCLK2/4 = 90/4 = 22.5 MHz; Tconv = (480+12)/22.5MHz ~= 21.9us.
 * A 6-channel scan needs 6 x 50 x 21.9us ~= 6.56 ms per block; the 200 ms loop
 * refreshes at ~5 Hz and keeps only the latest completed block. */

static uint16_t          g_adc_buf[ADC_DMA_BUF_LEN_SAMPLE];
static volatile bool     g_adc_ready;

static void on_adc_scan(uint16_t offset)
{
    (void)offset;
    g_adc_ready = true;
}

static void adc_show(void)
{
    uint32_t ch;

    for (ch = 0U; ch < (uint32_t)ADC_SCAN_CH_NUM; ch++)
    {
        uint32_t sum = 0U;
        uint32_t i;
        uint32_t raw;
        uint32_t mv;

        for (i = 0U; i < ADC_SCAN_SAMPLE_COUNT; i++)
        {
            sum += g_adc_buf[i * (uint32_t)ADC_SCAN_CH_NUM + ch];
        }
        raw = sum / ADC_SCAN_SAMPLE_COUNT;
        mv  = (raw * ADC_VREF_MV) / ADC_FULL_SCALE_COUNT;

        printf("ch%u raw:%u vol:%lu.%03luV\r\n",
               (unsigned int)ch,
               (unsigned int)raw,
               (unsigned long)(mv / 1000U),
               (unsigned long)(mv % 1000U));
    }

    printf("\r\n");
}

int main(void)
{
    adc_cfg_t cfg = { ADC_CFG_DEFAULT };

    bsp_init();

    cfg.mode     = ADC_MODE_DMA;
    cfg.dma_mode = ADC_DMA_CIRCULAR;
    cfg.chans    = (const adc_channel_t[]){ ADC_CH0, ADC_CH1, ADC_CH2, ADC_CH3, ADC_CH4, ADC_CH5 };
    cfg.nchans   = ADC_SCAN_CH_NUM;
    cfg.dma_buf  = g_adc_buf;
    cfg.dma_len  = ADC_DMA_BUF_LEN_SAMPLE;
    cfg.dma_cb   = on_adc_scan;
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
