/**
 * @file    main.c
 * @brief   22_1_dac: DAC1 channel 1 (PA4) interactive output. WK_UP/KEY0 step
 *          the code; the programmed code, its voltage and an ADC read-back of
 *          PA5 (jumpered to the DAC output PA4) are printed over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"
#include "dac.h"

#define DAC_STEP_COUNT     256U
#define VREF_MV            3300U
#define ADC_AVG_COUNT      10U
#define ADC_AVG_DELAY_MS   5U

static void dac_show(uint16_t code)
{
    uint32_t sum = 0U;
    uint32_t i;
    uint32_t adc;
    uint32_t dac_mv;
    uint32_t adc_mv;

    dac_mv = ((uint32_t)code * VREF_MV) / DAC_FULL_SCALE_COUNT;

    for (i = 0U; i < ADC_AVG_COUNT; i++)
    {
        sum += adc_read(ADC_ID_1, ADC_CH5);
        delay_ms(ADC_AVG_DELAY_MS);
    }
    adc    = sum / ADC_AVG_COUNT;
    adc_mv = (adc * VREF_MV) / DAC_FULL_SCALE_COUNT;

    printf("DAC: %4u %lu.%03luV  ADC: %4lu %lu.%03luV\r\n", (unsigned)code,
           (unsigned long)(dac_mv / 1000U), (unsigned long)(dac_mv % 1000U),
           (unsigned long)adc,
           (unsigned long)(adc_mv / 1000U), (unsigned long)(adc_mv % 1000U));
}

int main(void)
{
    uint16_t code = DAC_FULL_SCALE_COUNT / 2U;

    bsp_init();
    printf(APP_BANNER "\r\n");
    dac_init(NULL);
    adc_init(NULL);
    dac_write(DAC_CH1, code);

    printf("WKUP: +  KEY0: -\r\n");
    dac_show(code);

    for (;;)
    {
        key_id_t key = key_scan(false);
        uint16_t next = code;

        if (key == KEY_WKUP)
        {
            next = (uint16_t)((code + DAC_STEP_COUNT > DAC_FULL_SCALE_COUNT)
                              ? DAC_FULL_SCALE_COUNT : (code + DAC_STEP_COUNT));
        }
        else if (key == KEY0)
        {
            next = (uint16_t)((code < DAC_STEP_COUNT) ? 0U : (code - DAC_STEP_COUNT));
        }
        if (next != code)
        {
            code = next;
            dac_write(DAC_CH1, code);
            dac_show(code);
            led_toggle(LED1);          /* action LED */
        }

        led_toggle(LED0);              /* run LED, tied to the loop delay */
        delay_ms(50U);
    }
}
