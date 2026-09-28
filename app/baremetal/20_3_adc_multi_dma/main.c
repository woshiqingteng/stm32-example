/**
 * @file    main.c
 * @brief   20_3_adc_multi_dma: 6-channel ADC1 scan DMA acquisition (PA0..PA5).
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"

#define ADC_SCAN_SAMPLE_COUNT     50U
#define ADC_DMA_BUF_LEN_SAMPLE      (ADC_SCAN_SAMPLE_COUNT * ADC_SCAN_CH_NUM)
#define ADC_VREF_MV          3300U
#define ADC_VREF_UV          (ADC_VREF_MV * 1000U)
#define ADC_UV_PER_VOLT      1000000U
#define ADC_UV_PER_MV        1000U
#define ADC_FULL_SCALE_COUNT       4096U
#define ADC_LOOP_MS   10U
#define BLINK_PERIOD_MS         5U

typedef enum
{
    ADC_SCAN_IDLE = 0,
    ADC_SCAN_DONE = 1,
} adc_scan_state_t;

static uint16_t                  g_adc_buf[ADC_DMA_BUF_LEN_SAMPLE];
static volatile adc_scan_state_t g_adc_state = ADC_SCAN_IDLE;

static void on_adc_scan_complete(void)
{
    g_adc_state = ADC_SCAN_DONE;
}

int main(void)
{
    uint32_t blink = 0U;

    bsp_init();
    adc_scan_dma_init(g_adc_buf, ADC_DMA_BUF_LEN_SAMPLE);
    adc_register_dma_hook(&on_adc_scan_complete);
    adc_scan_dma_start(ADC_DMA_BUF_LEN_SAMPLE);
    printf(APP_BANNER "\r\n");

    for (;;)
    {
        if (g_adc_state == ADC_SCAN_DONE)
        {
            uint32_t ch;

            for (ch = 0U; ch < (uint32_t)ADC_SCAN_CH_NUM; ch++)
            {
                uint32_t sum = 0U;
                uint32_t i;
                uint32_t raw;
                uint32_t micro;

                for (i = 0U; i < ADC_SCAN_SAMPLE_COUNT; i++)
                {
                    sum += g_adc_buf[i * (uint32_t)ADC_SCAN_CH_NUM + ch];
                }

                raw   = sum / ADC_SCAN_SAMPLE_COUNT;
                micro = (uint32_t)(((uint64_t)raw * ADC_VREF_UV) / ADC_FULL_SCALE_COUNT);

                printf("ch%u raw:%u vol:%lu.%03luV\r\n",
                       (unsigned int)ch,
                       (unsigned int)raw,
                       (unsigned long)(micro / ADC_UV_PER_VOLT),
                       (unsigned long)((micro % ADC_UV_PER_VOLT) / ADC_UV_PER_MV));
            }

            printf("\r\n");

            g_adc_state = ADC_SCAN_IDLE;
            adc_scan_dma_start(ADC_DMA_BUF_LEN_SAMPLE);

            if ((++blink % BLINK_PERIOD_MS) == 0U)
            {
                led_toggle(LED0);
            }
        }

        delay_ms(ADC_LOOP_MS);
    }
}
