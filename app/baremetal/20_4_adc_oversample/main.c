/**
 * @file    main.c
 * @brief   20_4_adc_oversample: 256x ADC1_IN5 DMA oversampling (16-bit result).
 */

#include <stdio.h>

#include "bsp.h"

#define ADC_OVERSAMPLE_COUNT 256U
#define ADC_DMA_GROUP_COUNT       10U
#define ADC_DMA_BUF_LEN_SAMPLE      (ADC_OVERSAMPLE_COUNT * ADC_DMA_GROUP_COUNT)
#define ADC_OVERSAMPLE_SHIFT_DIV 4U
#define ADC_VREF_MV          3300U
#define ADC_VREF_UV          (ADC_VREF_MV * 1000U)
#define ADC_UV_PER_VOLT      1000000U
#define ADC_UV_PER_MV        1000U
#define ADC_FULL_SCALE_COUNT       65536U
#define ADC_LOOP_MS   10U
#define BLINK_PERIOD_MS         2U

typedef enum
{
    ADC_OVS_IDLE = 0,
    ADC_OVS_DONE = 1,
} adc_ovs_state_t;

static uint16_t                 g_adc_buf[ADC_DMA_BUF_LEN_SAMPLE];
static volatile adc_ovs_state_t g_adc_state = ADC_OVS_IDLE;

static void on_adc_oversample_complete(void)
{
    g_adc_state = ADC_OVS_DONE;
}

int main(void)
{
    uint32_t blink = 0U;

    bsp_init();
    adc_dma_init(g_adc_buf, ADC_DMA_BUF_LEN_SAMPLE);
    adc_register_dma_hook(on_adc_oversample_complete);
    adc_dma_start(ADC_DMA_BUF_LEN_SAMPLE);
    printf(APP_BANNER "\r\n");

    for (;;)
    {
        if (g_adc_state == ADC_OVS_DONE)
        {
            uint32_t sum = 0U;
            uint32_t i;
            uint32_t raw;
            uint32_t micro;

            for (i = 0U; i < ADC_DMA_BUF_LEN_SAMPLE; i++)
            {
                sum += g_adc_buf[i];
            }

            raw  = sum / ADC_DMA_GROUP_COUNT;
            raw >>= ADC_OVERSAMPLE_SHIFT_DIV;
            micro = (uint32_t)(((uint64_t)raw * ADC_VREF_UV) / ADC_FULL_SCALE_COUNT);

            printf("ovs raw:%u vol:%lu.%03luV\r\n",
                   (unsigned int)raw,
                   (unsigned long)(micro / ADC_UV_PER_VOLT),
                   (unsigned long)((micro % ADC_UV_PER_VOLT) / ADC_UV_PER_MV));

            g_adc_state = ADC_OVS_IDLE;
            adc_dma_start(ADC_DMA_BUF_LEN_SAMPLE);

            if ((++blink % BLINK_PERIOD_MS) == 0U)
            {
                led_toggle(LED0);
            }
        }

        delay_ms(ADC_LOOP_MS);
    }
}
