/**
 * @file    main.c
 * @brief   23_pwm_dac: filtered PWM on TIM9_CH2 (PA3) used as a DAC. WK_UP/KEY0
 *          step the duty; the CCR, its voltage and an ADC read-back of the pin
 *          are printed over USART1.
 */

#include <stdio.h>

#include "bsp.h"
#include "adc.h"
#include "tim.h"

#define PWMDAC_ARR       255U
#define PWMDAC_PSC       1U
#define PWMDAC_VREF_MV   3300U
/* TIM9/APB2: 180 MHz/((1+1)*256) = 351.5625 kHz */
#define PWMDAC_STEP_MV   100U

#define APP_LOOP_MS       10U
#define ADC_AVG_COUNT     10U
#define ADC_AVG_DELAY_MS  5U
#define LED_BLINK_MS      500U
#define LED_BLINK_TICKS   (LED_BLINK_MS / APP_LOOP_MS)

/* Set the filtered-PWM output voltage (vol in mV). */
static void pwmdac_set(uint16_t vol)
{
    uint32_t ccr;

    if (vol > PWMDAC_VREF_MV)
    {
        vol = PWMDAC_VREF_MV;
    }
    ccr = ((uint32_t)vol * (PWMDAC_ARR + 1U)) / PWMDAC_VREF_MV;
    tim_set(TIM_ID_9, TIM_CH2, TIM_PARAM_CCR, ccr);
}

static void pwmdac_show(void)
{
    uint16_t code   = (uint16_t)tim_get(TIM_ID_9, TIM_CH2, TIM_PARAM_CCR);
    uint16_t vol    = (uint16_t)(((uint32_t)code * PWMDAC_VREF_MV) / (PWMDAC_ARR + 1U));
    uint32_t sum    = 0U;
    uint32_t i;
    uint32_t adc;
    uint32_t adc_mv;

    for (i = 0U; i < ADC_AVG_COUNT; i++)
    {
        sum += adc_read(ADC_ID_1, ADC_CH3);
        delay_ms(ADC_AVG_DELAY_MS);
    }
    adc    = sum / ADC_AVG_COUNT;
    adc_mv = (adc * PWMDAC_VREF_MV) / 4095U;

    printf("PWM: %3u %u.%03uV  ADC: %4lu %lu.%03luV\r\n", (unsigned)code,
           (unsigned)(vol / 1000U), (unsigned)(vol % 1000U),
           (unsigned long)adc,
           (unsigned long)(adc_mv / 1000U), (unsigned long)(adc_mv % 1000U));
}

int main(void)
{
    tim_cfg_t cfg  = { TIM_CFG_DEFAULT };
    uint16_t vol   = PWMDAC_VREF_MV / 2U;
    uint32_t blink = 0U;

    bsp_init();
    printf(APP_BANNER "\r\n");

    cfg.id       = TIM_ID_9;
    cfg.mode     = TIM_MODE_PWM;
    cfg.channel  = TIM_CH2;
    cfg.polarity = TIM_POL_HIGH;
    cfg.pull     = TIM_PULL_UP;
    cfg.arr      = PWMDAC_ARR;
    cfg.psc      = PWMDAC_PSC;
    tim_init(&cfg);

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
            led_toggle(LED1);      /* action indicator */
        }

        if ((++blink % LED_BLINK_TICKS) == 0U)
        {
            led_toggle(LED0);      /* run indicator, ~500 ms */
        }
        delay_ms(APP_LOOP_MS);
    }
}
