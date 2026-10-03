/**
 * @file    main.c
 * @brief   22_2_dac_tri: DAC1 channel 1 triangle wave played from an
 *          application buffer by TIM6 TRGO + DMA1 (circular); an ADC read-back
 *          of PA5 (jumpered to the DAC output PA4) is printed over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"
#include "dac.h"

#define TRI_TIMER_ARR 899U
#define TRI_TIMER_PSC 0U
/* f_sample = 90 MHz/((0+1)*900) = 100 kHz; /100 samples -> ~1 kHz */

#define TRI_WAVE_LEN 100U

#define APP_LOOP_MS       500U
#define ADC_AVG_COUNT     10U
#define ADC_AVG_DELAY_MS  5U

/* One full triangle period: 50 rising samples followed by 50 falling. */
static const uint16_t g_tri_buf[TRI_WAVE_LEN] =
{
       0,   84,  167,  251,  334,  418,  501,  585,  669,  752,
     836,  919, 1003, 1086, 1170, 1254, 1337, 1421, 1504, 1588,
    1671, 1755, 1839, 1922, 2006, 2089, 2173, 2256, 2340, 2424,
    2507, 2591, 2674, 2758, 2841, 2925, 3009, 3092, 3176, 3259,
    3343, 3426, 3510, 3594, 3677, 3761, 3844, 3928, 4011, 4095,
    4095, 4011, 3928, 3844, 3761, 3677, 3594, 3510, 3426, 3343,
    3259, 3176, 3092, 3009, 2925, 2841, 2758, 2674, 2591, 2507,
    2424, 2340, 2256, 2173, 2089, 2006, 1922, 1839, 1755, 1671,
    1588, 1504, 1421, 1337, 1254, 1170, 1086, 1003,  919,  836,
     752,  669,  585,  501,  418,  334,  251,  167,   84,    0,
};

static void tri_show(void)
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

    printf("tri ADC(Pin5): %lu\r\n", (unsigned long)adc);
}

int main(void)
{
    dac_cfg_t cfg = { DAC_CFG_DEFAULT };

    bsp_init();
    printf(APP_BANNER "\r\n");
    adc_init(NULL);

    cfg.mode          = DAC_MODE_WAVE;
    cfg.channel       = DAC_CH1;
    cfg.buffer_enable = true;
    cfg.buf           = g_tri_buf;
    cfg.len           = TRI_WAVE_LEN;
    cfg.timer         = DAC_TIMER_6;
    cfg.arr           = TRI_TIMER_ARR;
    cfg.psc           = TRI_TIMER_PSC;
    dac_init(&cfg);
    dac_start();

    printf("22_2_dac_tri ready, ~1kHz triangle on PA4\r\n");

    for (;;)
    {
        tri_show();
        led_toggle(LED0);          /* run indicator, ~500 ms */
        delay_ms(APP_LOOP_MS);
    }
}
