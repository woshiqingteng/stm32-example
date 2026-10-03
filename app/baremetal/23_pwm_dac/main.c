/**
 * @file    main.c
 * @brief   23_pwm_dac: filtered PWM on TIM9_CH2 (PA3) used as a DAC. WK_UP/KEY0
 *          step the duty; the CCR, its voltage and an ADC read-back of the pin
 *          are printed over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"
#include "pwmdac.h"

#define PWMDAC_ARR       255U
#define PWMDAC_PSC       1U
/* TIM9/APB2: 180 MHz/((1+1)*256) = 351.5625 kHz */
#define PWMDAC_STEP_MV   100U

#define ADC_AVG_COUNT     10U
#define ADC_AVG_DELAY_MS  5U

static uint32_t adc_read_avg(adc_channel_t ch)
{
    uint32_t sum = 0U;
    uint32_t i;

    for (i = 0U; i < ADC_AVG_COUNT; i++)
    {
        sum += adc_read(ADC_ID_1, ch);
        delay_ms(ADC_AVG_DELAY_MS);
    }
    return sum / ADC_AVG_COUNT;
}

static void pwmdac_show(void)
{
    uint16_t code = (uint16_t)pwmdac_get_code();
    uint16_t vol = (uint16_t)(((uint32_t)code * PWMDAC_VREF_MV) / (PWMDAC_ARR + 1U));
    uint32_t adc = adc_read_avg(ADC_CH3);
    uint32_t adc_mv = (adc * PWMDAC_VREF_MV) / 4095U;

    printf("PWM: %3u %u.%03uV  ADC: %4lu %lu.%03luV\r\n", (unsigned)code,
           (unsigned)(vol / 1000U), (unsigned)(vol % 1000U),
           (unsigned long)adc,
           (unsigned long)(adc_mv / 1000U), (unsigned long)(adc_mv % 1000U));
}

int main(void)
{
    uint16_t vol = PWMDAC_VREF_MV / 2U;

    bsp_init();
    printf(APP_BANNER "\r\n");
    pwmdac_init(PWMDAC_ARR, PWMDAC_PSC);
    adc_init(NULL);
    pwmdac_set(vol);

    printf("WKUP: +  KEY0: -\r\n");
    pwmdac_show();

    for (;;)
    {
        key_id_t key = key_scan(false);
        uint16_t next = vol;

        if (key == KEY_WKUP)
        {
            next = (uint16_t)((vol + PWMDAC_STEP_MV > PWMDAC_VREF_MV)
                              ? PWMDAC_VREF_MV : (vol + PWMDAC_STEP_MV));
        }
        else if (key == KEY0)
        {
            next = (uint16_t)((vol < PWMDAC_STEP_MV) ? 0U : (vol - PWMDAC_STEP_MV));
        }
        if (next != vol)
        {
            vol = next;
            pwmdac_set(vol);
            pwmdac_show();
            led_toggle(LED0);
        }

        delay_ms(50U);
    }
}
