/**
 * @file    main.c
 * @brief   20_4_adc_oversample: 256x ADC1_IN5 DMA oversampling (16-bit result).
 */

#include <stdio.h>
#include "bsp.h"

#define ADC_OVERSAMPLE_TIMES 256U
#define ADC_DMA_GROUPS       10U
#define ADC_DMA_BUF_LEN      (ADC_OVERSAMPLE_TIMES * ADC_DMA_GROUPS)
#define ADC_OVERSAMPLE_SHIFT 4U
#define ADC_VREF_MV          3300U
#define ADC_VREF_UV          (ADC_VREF_MV * 1000U)
#define ADC_UV_PER_VOLT      1000000U
#define ADC_UV_PER_MV        1000U
#define ADC_FULL_SCALE       65536U
#define ADC_POLL_PERIOD_MS   10U
#define BLINK_PERIOD         2U

typedef enum
{
    ADC_OVS_IDLE = 0,
    ADC_OVS_DONE,
} adc_ovs_state_t;

static uint16_t                 g_adc_buf[ADC_DMA_BUF_LEN];
static volatile adc_ovs_state_t g_adc_state = ADC_OVS_IDLE;

static void on_adc_oversample_complete(void)
{
    g_adc_state = ADC_OVS_DONE;
}

int main(void)
{
    uint32_t blink = 0U;

    bsp_init();
    adc_dma_init(g_adc_buf, ADC_DMA_BUF_LEN);
    adc_register_dma_hook(on_adc_oversample_complete);
    adc_dma_start(ADC_DMA_BUF_LEN);
    printf("20_4_adc_oversample ready\r\n");

    for (;;)
    {
        if (g_adc_state == ADC_OVS_DONE)
        {
            uint32_t sum = 0U;
            uint32_t i;
            uint32_t raw;
            uint32_t micro;

            for (i = 0U; i < ADC_DMA_BUF_LEN; i++)
            {
                sum += g_adc_buf[i];
            }

            raw  = sum / ADC_DMA_GROUPS;
            raw >>= ADC_OVERSAMPLE_SHIFT;
            micro = (uint32_t)(((uint64_t)raw * ADC_VREF_UV) / ADC_FULL_SCALE);

            printf("ovs raw:%u vol:%lu.%03luV\r\n",
                   (unsigned int)raw,
                   (unsigned long)(micro / ADC_UV_PER_VOLT),
                   (unsigned long)((micro % ADC_UV_PER_VOLT) / ADC_UV_PER_MV));

            g_adc_state = ADC_OVS_IDLE;
            adc_dma_start(ADC_DMA_BUF_LEN);

            if ((++blink % BLINK_PERIOD) == 0U)
            {
                led_toggle(LED0);
            }
        }

        delay_ms(ADC_POLL_PERIOD_MS);
    }
}
