/**
 * @file    atim.c
 * @brief   Advanced timer driver (TIM8 / TIM1). MSP content is inlined; the two
 *          TIM8 users (NPWM and PWM-input) share one TIM8_UP handler selected by
 *          an explicit mode enum.
 */

#include "stm32f4xx_hal.h"
#include "atim.h"

#define ATIM_NPWM_BATCH 256U

typedef enum
{
    ATIM_MODE_NONE = 0,
    ATIM_MODE_NPWM,
    ATIM_MODE_PWMIN,
} atim_mode_t;

static atim_mode_t g_atim_mode;

/* ===================== TIM8 NPWM (PC6 / CH1) ===================== */

static TIM_HandleTypeDef g_atim_npwm_handle;
static uint32_t          g_atim_npwm_remain;

void atim_timx_npwm_chy_init(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef gpio_init = {0};
    TIM_OC_InitTypeDef oc      = {0};

    __HAL_RCC_TIM8_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, 1, 3);
    HAL_NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);

    gpio_init.Pin       = GPIO_PIN_6;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    g_atim_npwm_handle.Instance               = TIM8;
    g_atim_npwm_handle.Init.Prescaler         = psc;
    g_atim_npwm_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_atim_npwm_handle.Init.Period            = arr;
    g_atim_npwm_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    g_atim_npwm_handle.Init.RepetitionCounter = 0;
    HAL_TIM_PWM_Init(&g_atim_npwm_handle);

    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = arr / 2U;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&g_atim_npwm_handle, &oc, TIM_CHANNEL_1);

    g_atim_mode = ATIM_MODE_NPWM;
    __HAL_TIM_ENABLE_IT(&g_atim_npwm_handle, TIM_IT_UPDATE);
    HAL_TIM_PWM_Start(&g_atim_npwm_handle, TIM_CHANNEL_1);
}

void atim_timx_npwm_chy_set(uint32_t npwm)
{
    if (npwm == 0U)
    {
        return;
    }

    g_atim_npwm_remain = npwm;
    HAL_TIM_GenerateEvent(&g_atim_npwm_handle, TIM_EVENTSOURCE_UPDATE);
    __HAL_TIM_ENABLE(&g_atim_npwm_handle);
}

static void atim_npwm_isr(void)
{
    uint16_t npwm = 0;

    if (__HAL_TIM_GET_FLAG(&g_atim_npwm_handle, TIM_FLAG_UPDATE) == RESET)
    {
        return;
    }

    if (g_atim_npwm_remain >= ATIM_NPWM_BATCH)
    {
        g_atim_npwm_remain -= ATIM_NPWM_BATCH;
        npwm = ATIM_NPWM_BATCH;
    }
    else if ((g_atim_npwm_remain % ATIM_NPWM_BATCH) != 0U)
    {
        npwm = (uint16_t)(g_atim_npwm_remain % ATIM_NPWM_BATCH);
        g_atim_npwm_remain = 0;
    }

    if (npwm != 0U)
    {
        TIM8->RCR = (uint16_t)(npwm - 1U);
        HAL_TIM_GenerateEvent(&g_atim_npwm_handle, TIM_EVENTSOURCE_UPDATE);
        __HAL_TIM_ENABLE(&g_atim_npwm_handle);
    }
    else
    {
        TIM8->CR1 &= ~TIM_CR1_CEN;
    }

    __HAL_TIM_CLEAR_IT(&g_atim_npwm_handle, TIM_IT_UPDATE);
}

/* ================= TIM8 output compare (PC6..PC9) ================= */

static TIM_HandleTypeDef g_atim_comp_handle;

void atim_timx_comp_pwm_init(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef gpio_init = {0};
    TIM_OC_InitTypeDef oc      = {0};

    __HAL_RCC_TIM8_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF3_TIM8;
    for (uint16_t pin = GPIO_PIN_6; pin <= GPIO_PIN_9; pin = (uint16_t)(pin << 1))
    {
        gpio_init.Pin = pin;
        HAL_GPIO_Init(GPIOC, &gpio_init);
    }

    g_atim_comp_handle.Instance               = TIM8;
    g_atim_comp_handle.Init.Prescaler         = psc;
    g_atim_comp_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_atim_comp_handle.Init.Period            = arr;
    g_atim_comp_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_OC_Init(&g_atim_comp_handle);

    oc.OCMode     = TIM_OCMODE_TOGGLE;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;

    oc.Pulse = 250U - 1U;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_1);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_1);

    oc.Pulse = 500U - 1U;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_2);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_2);

    oc.Pulse = 750U - 1U;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_3);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_3);

    oc.Pulse        = 1000U - 1U;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_4);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_4);

    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_1);
    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_2);
    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_3);
    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_4);
}

void atim_timx_comp_pwm_set(uint32_t channel, uint16_t ccr)
{
    __HAL_TIM_SET_COMPARE(&g_atim_comp_handle, channel, ccr);
}

/* ============ TIM1 complementary PWM + dead time (PE9/PE8/PE15) ============ */

static TIM_HandleTypeDef                  g_atim_cplm_handle;
static TIM_BreakDeadTimeConfigTypeDef     g_atim_cplm_break = {0};

void atim_timx_cplm_pwm_init(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef gpio_init = {0};
    TIM_OC_InitTypeDef oc      = {0};

    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF1_TIM1;

    gpio_init.Pin = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOE, &gpio_init);
    gpio_init.Pin = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOE, &gpio_init);
    gpio_init.Pin = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &gpio_init);

    g_atim_cplm_handle.Instance               = TIM1;
    g_atim_cplm_handle.Init.Prescaler         = psc;
    g_atim_cplm_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_atim_cplm_handle.Init.Period            = arr;
    g_atim_cplm_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV4;
    g_atim_cplm_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&g_atim_cplm_handle);

    oc.OCMode       = TIM_OCMODE_PWM1;
    oc.OCPolarity   = TIM_OCPOLARITY_LOW;
    oc.OCNPolarity  = TIM_OCNPOLARITY_LOW;
    oc.OCIdleState  = TIM_OCIDLESTATE_SET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_SET;
    HAL_TIM_PWM_ConfigChannel(&g_atim_cplm_handle, &oc, TIM_CHANNEL_1);

    g_atim_cplm_break.OffStateRunMode  = TIM_OSSR_DISABLE;
    g_atim_cplm_break.OffStateIDLEMode = TIM_OSSI_DISABLE;
    g_atim_cplm_break.LockLevel        = TIM_LOCKLEVEL_OFF;
    g_atim_cplm_break.BreakState       = TIM_BREAK_ENABLE;
    g_atim_cplm_break.BreakPolarity    = TIM_BREAKPOLARITY_LOW;
    g_atim_cplm_break.AutomaticOutput  = TIM_AUTOMATICOUTPUT_ENABLE;
    HAL_TIMEx_ConfigBreakDeadTime(&g_atim_cplm_handle, &g_atim_cplm_break);

    HAL_TIM_PWM_Start(&g_atim_cplm_handle, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&g_atim_cplm_handle, TIM_CHANNEL_1);
}

void atim_timx_cplm_pwm_set(uint16_t ccr, uint8_t dtg)
{
    g_atim_cplm_break.DeadTime = dtg;
    HAL_TIMEx_ConfigBreakDeadTime(&g_atim_cplm_handle, &g_atim_cplm_break);
    __HAL_TIM_MOE_ENABLE(&g_atim_cplm_handle);
    __HAL_TIM_SET_COMPARE(&g_atim_cplm_handle, TIM_CHANNEL_1, ccr);
}

/* ===================== TIM8 PWM input (PC6 / CH1) ===================== */

static TIM_HandleTypeDef g_atim_pwmin_handle;
static atim_pwmin_state_t g_atim_pwmin_state;
static uint16_t          g_atim_pwmin_psc;
static uint32_t          g_atim_pwmin_hval;
static uint32_t          g_atim_pwmin_cval;

void atim_timx_pwmin_chy_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    TIM_SlaveConfigTypeDef slave = {0};
    TIM_IC_InitTypeDef ic = {0};

    __HAL_RCC_TIM8_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, 1, 3);
    HAL_NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
    HAL_NVIC_SetPriority(TIM8_CC_IRQn, 1, 3);
    HAL_NVIC_EnableIRQ(TIM8_CC_IRQn);

    gpio_init.Pin       = GPIO_PIN_6;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLDOWN;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    g_atim_pwmin_handle.Instance         = TIM8;
    g_atim_pwmin_handle.Init.Prescaler   = 0;
    g_atim_pwmin_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_atim_pwmin_handle.Init.Period      = 65535;
    HAL_TIM_IC_Init(&g_atim_pwmin_handle);

    slave.SlaveMode       = TIM_SLAVEMODE_RESET;
    slave.InputTrigger    = TIM_TS_TI1FP1;
    slave.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    slave.TriggerFilter   = 0;
    HAL_TIM_SlaveConfigSynchro(&g_atim_pwmin_handle, &slave);

    ic.ICPolarity  = TIM_INPUTCHANNELPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter    = 0;
    HAL_TIM_IC_ConfigChannel(&g_atim_pwmin_handle, &ic, TIM_CHANNEL_1);

    ic.ICPolarity  = TIM_INPUTCHANNELPOLARITY_FALLING;
    ic.ICSelection = TIM_ICSELECTION_INDIRECTTI;
    HAL_TIM_IC_ConfigChannel(&g_atim_pwmin_handle, &ic, TIM_CHANNEL_2);

    g_atim_mode = ATIM_MODE_PWMIN;

    __HAL_TIM_ENABLE_IT(&g_atim_pwmin_handle, TIM_IT_UPDATE);
    HAL_TIM_IC_Start_IT(&g_atim_pwmin_handle, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&g_atim_pwmin_handle, TIM_CHANNEL_2);
}

void atim_timx_pwmin_chy_restart(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    g_atim_pwmin_state = ATIM_PWMIN_IDLE;
    g_atim_pwmin_psc   = 0;
    __HAL_TIM_SET_PRESCALER(&g_atim_pwmin_handle, 0);
    __HAL_TIM_SET_COUNTER(&g_atim_pwmin_handle, 0);
    __HAL_TIM_ENABLE_IT(&g_atim_pwmin_handle, TIM_IT_CC1);
    __HAL_TIM_ENABLE_IT(&g_atim_pwmin_handle, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE(&g_atim_pwmin_handle);
    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1);
    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC2);
    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);
    if (primask == 0U)
    {
        __enable_irq();
    }
}

atim_pwmin_state_t atim_timx_pwmin_chy_state(void)
{
    return g_atim_pwmin_state;
}

uint16_t atim_timx_pwmin_chy_psc(void)
{
    return g_atim_pwmin_psc;
}

uint32_t atim_timx_pwmin_chy_hval(void)
{
    return g_atim_pwmin_hval;
}

uint32_t atim_timx_pwmin_chy_cval(void)
{
    return g_atim_pwmin_cval;
}

static void atim_pwmin_process(void)
{
    static uint8_t first_done = 0;

    if (g_atim_pwmin_state == ATIM_PWMIN_DONE)
    {
        g_atim_pwmin_psc = 0;
        __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1);
        __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC2);
        __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);
        __HAL_TIM_SET_COUNTER(&g_atim_pwmin_handle, 0);
        return;
    }

    if (__HAL_TIM_GET_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE))
    {
        __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);

        if (__HAL_TIM_GET_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1) == 0)
        {
            first_done = 0;
            if (g_atim_pwmin_psc == 0U)
            {
                g_atim_pwmin_psc = 1;
            }
            else if (g_atim_pwmin_psc == 65535U)
            {
                g_atim_pwmin_psc = 0;
            }
            else if (g_atim_pwmin_psc > 32767U)
            {
                g_atim_pwmin_psc = 65535;
            }
            else
            {
                g_atim_pwmin_psc = (uint16_t)(g_atim_pwmin_psc * 2U);
            }

            __HAL_TIM_SET_PRESCALER(&g_atim_pwmin_handle, g_atim_pwmin_psc);
            __HAL_TIM_SET_COUNTER(&g_atim_pwmin_handle, 0);
            __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1);
            __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC2);
            __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);
            return;
        }
    }

    if (first_done == 0U)
    {
        if (__HAL_TIM_GET_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1))
        {
            first_done = 1;
        }
        __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1);
        __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC2);
        __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);
        return;
    }

    if (g_atim_pwmin_state == ATIM_PWMIN_IDLE)
    {
        if (__HAL_TIM_GET_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1))
        {
            g_atim_pwmin_hval = HAL_TIM_ReadCapturedValue(&g_atim_pwmin_handle, TIM_CHANNEL_2) + 1U;
            g_atim_pwmin_cval = HAL_TIM_ReadCapturedValue(&g_atim_pwmin_handle, TIM_CHANNEL_1) + 1U;

            if (g_atim_pwmin_hval < g_atim_pwmin_cval)
            {
                g_atim_pwmin_state = ATIM_PWMIN_DONE;
                g_atim_pwmin_psc   = (uint16_t)TIM8->PSC;

                if (g_atim_pwmin_psc == 0U)
                {
                    g_atim_pwmin_hval++;
                    g_atim_pwmin_cval++;
                }

                first_done = 0;
                TIM8->CR1 &= ~TIM_CR1_CEN;
                __HAL_TIM_DISABLE_IT(&g_atim_pwmin_handle, TIM_IT_CC1);
                __HAL_TIM_DISABLE_IT(&g_atim_pwmin_handle, TIM_IT_CC2);
                __HAL_TIM_DISABLE_IT(&g_atim_pwmin_handle, TIM_IT_UPDATE);
                __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1);
                __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC2);
                __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);
            }
            else
            {
                atim_timx_pwmin_chy_restart();
            }
        }
    }

    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1);
    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC2);
    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);
}

/* ===================== shared TIM8 handlers ===================== */

void TIM8_UP_TIM13_IRQHandler(void)
{
    if (g_atim_mode == ATIM_MODE_NPWM)
    {
        atim_npwm_isr();
    }
    else if (g_atim_mode == ATIM_MODE_PWMIN)
    {
        atim_pwmin_process();
    }
}

void TIM8_CC_IRQHandler(void)
{
    if (g_atim_mode == ATIM_MODE_PWMIN)
    {
        atim_pwmin_process();
    }
}
