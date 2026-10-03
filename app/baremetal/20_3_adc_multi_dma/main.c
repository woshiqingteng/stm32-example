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
#define ADC_LOOP_MS             10U
#define BLINK_TICKS             5U

static const adc_channel_t g_scan[ADC_SCAN_CH_NUM] =
{
    ADC_CH0, ADC_CH1, ADC_CH2, ADC_CH3, ADC_CH4, ADC_CH5,
};
static uint16_t          g_adc_buf[ADC_DMA_BUF_LEN_SAMPLE];
static volatile bool     g_adc_ready;

static void on_adc_scan(uint16_t offset)
{
    (void)offset;
    g_adc_ready = true;
}

int main(void)
{
    uint32_t  blink = 0U;
    adc_cfg_t cfg   = { ADC_CFG_DEFAULT };

    bsp_init();

    cfg.mode     = ADC_MODE_DMA;
    cfg.dma_mode = ADC_DMA_CIRCULAR;
    cfg.chans    = g_scan;
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
            uint32_t ch;

            g_adc_ready = false;

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

            if ((++blink % BLINK_TICKS) == 0U)
            {
                led_toggle(LED0);
            }
        }

        delay_ms(ADC_LOOP_MS);
    }
}
