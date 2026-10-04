/**
 * @file    pwmdac.c
 * @brief   PWM DAC driver on TIM9_CH2 (PA3): filtered PWM used as a DAC.
 *          Built on the unified tim driver.
 */

#include "pwmdac.h"
#include "tim.h"

static uint16_t g_pwmdac_arr;

/* TIM9 (APB2): f_PWM = 180 MHz/((PSC+1)(ARR+1)). */
void pwmdac_init(uint16_t arr, uint16_t psc)
{
    tim_cfg_t cfg = { TIM_CFG_DEFAULT };

    g_pwmdac_arr = arr;

    cfg.id       = TIM_ID_9;
    cfg.mode     = TIM_MODE_PWM;
    cfg.channel  = TIM_CH2;
    cfg.polarity = TIM_POL_HIGH;
    cfg.pull     = TIM_PULL_UP;
    cfg.arr      = arr;
    cfg.psc      = psc;
    tim_init(&cfg);
}

void pwmdac_set(uint16_t vol)
{
    uint32_t span = (uint32_t)g_pwmdac_arr + 1U;
    uint32_t ccr;

    if (vol > PWMDAC_VREF_MV)
    {
        vol = PWMDAC_VREF_MV;
    }

    ccr = ((uint32_t)vol * span) / PWMDAC_VREF_MV;
    tim_set(TIM_ID_9, TIM_CH2, TIM_PARAM_CCR, ccr);
}

uint16_t pwmdac_get_code(void)
{
    return (uint16_t)tim_get(TIM_ID_9, TIM_CH2, TIM_PARAM_CCR);
}
