/**
 * @file    main.c
 * @brief   20_3_adc_multi_dma: 6-channel ADC1 scan DMA acquisition (PA0..PA5).
 */

#include <stdio.h>
#include "bsp.h"

#define ADC_SCAN_SAMPLES 50U
#define ADC_DMA_BUF_LEN  (ADC_SCAN_SAMPLES * ADC_SCAN_CH_NUM)
#define ADC_VREF_MV      3300U
#define ADC_VREF_UV      (ADC_VREF_MV * 1000U)
#define ADC_FULL_SCALE   4096U
#define BLINK_PERIOD     5U

typedef enum
{
    ADC_SCAN_IDLE = 0,
    ADC_SCAN_DONE,
} adc_scan_state_t;

static uint16_t            g_adc_buf[ADC_DMA_BUF_LEN];
static volatile adc_scan_state_t g_adc_state = ADC_SCAN_IDLE;

static void on_adc_scan_complete(void)
{
    g_adc_state = ADC_SCAN_DONE;
}

int main(void)
{
    uint32_t blink = 0U;

    bsp_init();
    adc_scan_dma_init(g_adc_buf, ADC_DMA_BUF_LEN);
    adc_register_dma_hook(on_adc_scan_complete);
    adc_scan_dma_start(ADC_DMA_BUF_LEN);
    printf("20_3_adc_multi_dma ready\r\n");

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

                for (i = 0U; i < ADC_SCAN_SAMPLES; i++)
                {
                    sum += g_adc_buf[i * (uint32_t)ADC_SCAN_CH_NUM + ch];
                }

                raw   = sum / ADC_SCAN_SAMPLES;
                micro = (uint32_t)(((uint64_t)raw * ADC_VREF_UV) / ADC_FULL_SCALE);

                printf("ch%u raw:%u vol:%lu.%03luV\r\n",
                       (unsigned int)ch,
                       (unsigned int)raw,
                       (unsigned long)(micro / 1000000U),
                       (unsigned long)((micro % 1000000U) / 1000U));
            }

            printf("\r\n");

            g_adc_state = ADC_SCAN_IDLE;
            adc_scan_dma_start(ADC_DMA_BUF_LEN);

            if ((++blink % BLINK_PERIOD) == 0U)
            {
                led_toggle(LED0);
            }
        }

        delay_ms(10);
    }
}
