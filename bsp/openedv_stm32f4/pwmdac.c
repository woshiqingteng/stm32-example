/**
 * @file    pwmdac.c
 * @brief   PWM DAC driver on TIM9_CH2. TIM9_CH2 is available on PA3 (AF3) and
 *          PE6 (AF3); this port uses PA3 as in the vendor example. MSP content
 *          (clock + GPIO) is inlined into pwmdac_init().
 */

#include "stm32f4xx_hal.h"
#include "pwmdac.h"

#define PWMDAC_GPIO_PORT   GPIOA
#define PWMDAC_GPIO_PIN    GPIO_PIN_3
#define PWMDAC_GPIO_AF     GPIO_AF3_TIM9

#define PWMDAC_TIMX        TIM9
#define PWMDAC_TIMX_CH     TIM_CHANNEL_2

static TIM_HandleTypeDef g_pwmdac_handle;
static uint16_t          g_pwmdac_arr;

void pwmdac_init(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef   gpio_init = {0};
    TIM_OC_InitTypeDef oc_init   = {0};

    /* ---- MSP begin: clock + GPIO ---- */
    __HAL_RCC_TIM9_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_init.Pin       = PWMDAC_GPIO_PIN;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = PWMDAC_GPIO_AF;
    HAL_GPIO_Init(PWMDAC_GPIO_PORT, &gpio_init);
    /* ---- MSP end ---- */

    g_pwmdac_arr = arr;

    g_pwmdac_handle.Instance               = PWMDAC_TIMX;
    g_pwmdac_handle.Init.Prescaler         = psc;
    g_pwmdac_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_pwmdac_handle.Init.Period            = arr;
    g_pwmdac_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_pwmdac_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&g_pwmdac_handle);

    oc_init.OCMode     = TIM_OCMODE_PWM1;
    oc_init.Pulse      = arr / 2U;
    oc_init.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc_init.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&g_pwmdac_handle, &oc_init, PWMDAC_TIMX_CH);
    HAL_TIM_PWM_Start(&g_pwmdac_handle, PWMDAC_TIMX_CH);
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
    __HAL_TIM_SET_COMPARE(&g_pwmdac_handle, PWMDAC_TIMX_CH, ccr);
}

uint16_t pwmdac_get_code(void)
{
    return (uint16_t)__HAL_TIM_GET_COMPARE(&g_pwmdac_handle, PWMDAC_TIMX_CH);
}
