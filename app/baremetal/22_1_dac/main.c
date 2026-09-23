/**
 * @file    main.c
 * @brief   22_1_dac: DAC1 channel 1 (PA4) software-triggered ramp output. The
 *          programmed code and its equivalent voltage are printed over USART1.
 */

#include <stdio.h>
#include "bsp.h"

#define DAC_RAMP_STEP     64U
#define DAC_RAMP_DELAY_MS 5U
#define DAC_MV_FULL       3300U

static void dac_show(uint16_t code)
{
    char     buf[40];
    uint32_t millivolt = ((uint32_t)code * DAC_MV_FULL) / DAC_FULL_SCALE;

    sprintf(buf, "DAC: %4u  %lu.%03luV", (unsigned)code,
            (unsigned long)(millivolt / 1000U),
            (unsigned long)(millivolt % 1000U));
    printf("%s\r\n", buf);
}

int main(void)
{
    uint16_t code;

    bsp_init();
    dac_init();

    printf("22_1_dac ready\r\n");

    for (;;)
    {
        for (code = 0U; code <= DAC_FULL_SCALE; code += DAC_RAMP_STEP)
        {
            dac_set(DAC_CH1, code);
            dac_show(code);
            led_toggle(LED0);
            delay_ms(DAC_RAMP_DELAY_MS);
        }

        for (code = DAC_FULL_SCALE; code >= DAC_RAMP_STEP; code -= DAC_RAMP_STEP)
        {
            dac_set(DAC_CH1, code);
            dac_show(code);
            led_toggle(LED0);
            delay_ms(DAC_RAMP_DELAY_MS);
        }

        dac_set(DAC_CH1, 0U);
        dac_show(0U);
    }
}
