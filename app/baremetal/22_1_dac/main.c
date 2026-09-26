/**
 * @file    main.c
 * @brief   22_1_dac: DAC1 channel 1 (PA4) interactive output. WK_UP/KEY0 step
 *          the code; the programmed code, its voltage and an ADC read-back of
 *          the pin are printed over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#define DAC_STEP     256U
#define DAC_MV_FULL  3300U

static void dac_show(uint16_t code)
{
    uint32_t millivolt = ((uint32_t)code * DAC_MV_FULL) / DAC_FULL_SCALE;
    uint32_t adc = adc_get_result_average(ADC_CH4, 10U);
    uint32_t adc_mv = (adc * DAC_MV_FULL) / DAC_FULL_SCALE;

    printf("DAC: %4u %lu.%03luV  ADC: %4lu %lu.%03luV\r\n", (unsigned)code,
           (unsigned long)(millivolt / 1000U), (unsigned long)(millivolt % 1000U),
           (unsigned long)adc,
           (unsigned long)(adc_mv / 1000U), (unsigned long)(adc_mv % 1000U));
}

int main(void)
{
    uint16_t code = DAC_FULL_SCALE / 2U;

    bsp_init();
    dac_init();
    adc_init();
    dac_set(DAC_CH1, code);

    printf("22_1_dac ready (WKUP/KEY0 adjust)\r\n");
    dac_show(code);

    for (;;)
    {
        key_id_t key = key_scan(false);
        uint16_t next = code;

        if (key == KEY_WKUP)
        {
            next = (uint16_t)((code + DAC_STEP > DAC_FULL_SCALE)
                              ? DAC_FULL_SCALE : (code + DAC_STEP));
        }
        else if (key == KEY0)
        {
            next = (uint16_t)((code < DAC_STEP) ? 0U : (code - DAC_STEP));
        }
        if (next != code)
        {
            code = next;
            dac_set(DAC_CH1, code);
            dac_show(code);
            led_toggle(LED0);
        }

        delay_ms(50U);
    }
}
