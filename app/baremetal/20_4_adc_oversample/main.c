/**
 * @file    main.c
 * @brief   20_4_adc_oversample: 256x ADC1_IN5 (PA5) one-shot DMA oversampling
 *          (16-bit result), re-armed after each processed block.
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"

#define ADC_OVERSAMPLE_COUNT     256U
#define ADC_DMA_GROUP_COUNT      10U
#define ADC_DMA_BUF_LEN_SAMPLE   (ADC_OVERSAMPLE_COUNT * ADC_DMA_GROUP_COUNT)
#define ADC_OVERSAMPLE_SHIFT_DIV 4U
#define ADC_VREF_MV              3300U
#define ADC_FULL_SCALE_COUNT     65536U
#define ADC_LOOP_MS              10U
#define BLINK_TICKS              2U

static const adc_channel_t g_chan[1] = { ADC_CH5 };
static uint16_t            g_adc_buf[ADC_DMA_BUF_LEN_SAMPLE];
static volatile bool       g_adc_ready;

static void on_adc_oversample(uint16_t offset)
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
    cfg.dma_mode = ADC_DMA_ONESHOT;
    cfg.chans    = g_chan;
    cfg.nchans   = 1U;
    cfg.dma_buf  = g_adc_buf;
    cfg.dma_len  = ADC_DMA_BUF_LEN_SAMPLE;
    cfg.dma_cb   = on_adc_oversample;
    adc_init(&cfg);

    printf(APP_BANNER "\r\n");

    for (;;)
    {
        if (g_adc_ready)
        {
            uint32_t sum = 0U;
            uint32_t i;
            uint32_t raw;
            uint32_t mv;

            g_adc_ready = false;
            for (i = 0U; i < ADC_DMA_BUF_LEN_SAMPLE; i++)
            {
                sum += g_adc_buf[i];
            }

            raw  = sum / ADC_DMA_GROUP_COUNT;
            raw >>= ADC_OVERSAMPLE_SHIFT_DIV;
            mv   = (raw * ADC_VREF_MV) / ADC_FULL_SCALE_COUNT;

            printf("ovs raw:%u vol:%lu.%03luV\r\n",
                   (unsigned int)raw,
                   (unsigned long)(mv / 1000U),
                   (unsigned long)(mv % 1000U));

            adc_dma_start();

            if ((++blink % BLINK_TICKS) == 0U)
            {
                led_toggle(LED0);
            }
        }

        delay_ms(ADC_LOOP_MS);
    }
}
