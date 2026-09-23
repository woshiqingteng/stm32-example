/**
 * @file    main.c
 * @brief   23_pwm_dac: filtered PWM on TIM9_CH2 (PA3) used as a DAC. The duty
 *          (CCR) and its equivalent output voltage are printed over USART1.
 */

#include <stdio.h>
#include "bsp.h"

#define PWMDAC_ARR       255U
#define PWMDAC_PSC       1U
#define PWMDAC_STEP_MV   100U
#define PWMDAC_DELAY_MS  20U

static void pwmdac_show(void)
{
    char     buf[40];
    uint16_t vol = (uint16_t)(((uint32_t)pwmdac_get_code() * PWMDAC_VREF_MV) /
                              (PWMDAC_ARR + 1U));

    sprintf(buf, "PWM: %3lu  %u.%03luV",
            (unsigned long)pwmdac_get_code(),
            (unsigned)(vol / 1000U), (unsigned long)(vol % 1000U));
    printf("%s\r\n", buf);
}

int main(void)
{
    uint16_t vol;

    bsp_init();
    pwmdac_init(PWMDAC_ARR, PWMDAC_PSC);

    printf("23_pwm_dac ready\r\n");

    for (;;)
    {
        for (vol = 0U; vol <= PWMDAC_VREF_MV; vol += PWMDAC_STEP_MV)
        {
            pwmdac_set(vol);
            pwmdac_show();
            led_toggle(LED0);
            delay_ms(PWMDAC_DELAY_MS);
        }

        for (vol = PWMDAC_VREF_MV; vol >= PWMDAC_STEP_MV; vol -= PWMDAC_STEP_MV)
        {
            pwmdac_set(vol);
            pwmdac_show();
            led_toggle(LED0);
            delay_ms(PWMDAC_DELAY_MS);
        }
    }
}
