/**
 * @file    atim.c
 * @brief   Advanced timer driver (TIM8 / TIM1). MSP content is inlined; the two
 *          TIM8 users (NPWM and PWM-input) register the ISR hooks invoked from
 *          the shared TIM8 handlers.
 */

#include "stm32f4xx_hal.h"
#include "atim.h"

/* This HAL release only provides the setter form of the prescaler macro. */
#ifndef __HAL_TIM_GET_PRESCALER
#define __HAL_TIM_GET_PRESCALER(__HANDLE__) ((__HANDLE__)->Instance->PSC)
#endif

#define ATIM_NVIC_PRIORITY           1U
#define ATIM_NVIC_SUBPRIORITY        3U

#define ATIM_NPWM_BATCH              256U
#define ATIM_NPWM_DEFAULT_PULSE_DIV  2U
#define ATIM_REPETITION_COUNTER      0U

#define ATIM_OC_COMPARE_CH1          250U
#define ATIM_OC_COMPARE_CH2          500U
#define ATIM_OC_COMPARE_CH3          750U
#define ATIM_OC_COMPARE_CH4          1000U
#define ATIM_OC_GPIO_PINS            (GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9)

#define ATIM_PWMIN_ARR               0xFFFFU
#define ATIM_PWMIN_PSC_DEFAULT       0U
#define ATIM_PWMIN_PSC_FIRST         1U
#define ATIM_PWMIN_PSC_STEP          2U
#define ATIM_PWMIN_PSC_DOUBLE_LIMIT  0x7FFFU
#define ATIM_PWMIN_PSC_MAX           0xFFFFU
#define ATIM_PWMIN_TICKS_OFFSET      1U

typedef void (*atim_isr_hook_t)(void);

static atim_isr_hook_t g_atim_up_hook;
static atim_isr_hook_t g_atim_cc_hook;

static void atim_npwm_isr(void);
static void atim_pwmin_process(void);

/* F4 HAL has no public setter for the repetition counter (RCR), so it is
 * written directly. */
static void atim_set_repetition(uint16_t n)
{
    TIM8->RCR = n;
}

/* ===================== TIM8 NPWM (PC6 / CH1) ===================== */

static TIM_HandleTypeDef g_atim_npwm_handle;
static uint32_t          g_atim_npwm_remain;

void atim_timx_npwm_chy_init(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef gpio_init = {0};
    TIM_OC_InitTypeDef oc      = {0};

    __HAL_RCC_TIM8_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, ATIM_NVIC_PRIORITY, ATIM_NVIC_SUBPRIORITY);
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
    g_atim_npwm_handle.Init.RepetitionCounter = ATIM_REPETITION_COUNTER;
    HAL_TIM_PWM_Init(&g_atim_npwm_handle);

    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = arr / ATIM_NPWM_DEFAULT_PULSE_DIV;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&g_atim_npwm_handle, &oc, TIM_CHANNEL_1);

    g_atim_up_hook = atim_npwm_isr;
    g_atim_cc_hook = 0;
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
        atim_set_repetition((uint16_t)(npwm - 1U));
        HAL_TIM_GenerateEvent(&g_atim_npwm_handle, TIM_EVENTSOURCE_UPDATE);
        __HAL_TIM_ENABLE(&g_atim_npwm_handle);
    }
    else
    {
        __HAL_TIM_DISABLE(&g_atim_npwm_handle);
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

    gpio_init.Pin       = ATIM_OC_GPIO_PINS;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    g_atim_comp_handle.Instance               = TIM8;
    g_atim_comp_handle.Init.Prescaler         = psc;
    g_atim_comp_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_atim_comp_handle.Init.Period            = arr;
    g_atim_comp_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_OC_Init(&g_atim_comp_handle);

    oc.OCMode     = TIM_OCMODE_TOGGLE;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;

    oc.Pulse = ATIM_OC_COMPARE_CH1 - 1U;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_1);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_1);

    oc.Pulse = ATIM_OC_COMPARE_CH2 - 1U;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_2);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_2);

    oc.Pulse = ATIM_OC_COMPARE_CH3 - 1U;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_3);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_3);

    oc.Pulse        = ATIM_OC_COMPARE_CH4 - 1U;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_4);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_4);

    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_1);
    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_2);
    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_3);
    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_4);
}

static uint32_t atim_channel_hal(atim_channel_t channel)
{
    switch (channel)
    {
        case ATIM_CH1:
            return TIM_CHANNEL_1;
        case ATIM_CH2:
            return TIM_CHANNEL_2;
        case ATIM_CH3:
            return TIM_CHANNEL_3;
        case ATIM_CH4:
            return TIM_CHANNEL_4;
        default:
            return TIM_CHANNEL_1;
    }
}

void atim_timx_comp_pwm_set(atim_channel_t channel, uint16_t ccr)
{
    __HAL_TIM_SET_COMPARE(&g_atim_comp_handle, atim_channel_hal(channel), ccr);
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

typedef enum
{
    ATIM_PWMIN_SM_IDLE = 0, /*!< waiting for the first capture */
    ATIM_PWMIN_SM_ARMED,    /*!< first capture discarded, measuring */
    ATIM_PWMIN_SM_DONE,     /*!< high and cycle times available */
} atim_pwmin_sm_t;

static TIM_HandleTypeDef g_atim_pwmin_handle;
static atim_pwmin_sm_t   g_atim_pwmin_sm;
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
    HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, ATIM_NVIC_PRIORITY, ATIM_NVIC_SUBPRIORITY);
    HAL_NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
    HAL_NVIC_SetPriority(TIM8_CC_IRQn, ATIM_NVIC_PRIORITY, ATIM_NVIC_SUBPRIORITY);
    HAL_NVIC_EnableIRQ(TIM8_CC_IRQn);

    gpio_init.Pin       = GPIO_PIN_6;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLDOWN;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    g_atim_pwmin_handle.Instance         = TIM8;
    g_atim_pwmin_handle.Init.Prescaler   = ATIM_PWMIN_PSC_DEFAULT;
    g_atim_pwmin_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_atim_pwmin_handle.Init.Period      = ATIM_PWMIN_ARR;
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

    g_atim_up_hook = atim_pwmin_process;
    g_atim_cc_hook = atim_pwmin_process;

    __HAL_TIM_ENABLE_IT(&g_atim_pwmin_handle, TIM_IT_UPDATE);
    HAL_TIM_IC_Start_IT(&g_atim_pwmin_handle, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&g_atim_pwmin_handle, TIM_CHANNEL_2);
}

void atim_timx_pwmin_chy_restart(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    if (g_atim_pwmin_sm == ATIM_PWMIN_SM_DONE)
    {
        g_atim_pwmin_sm = ATIM_PWMIN_SM_IDLE;
    }
    g_atim_pwmin_psc = ATIM_PWMIN_PSC_DEFAULT;
    __HAL_TIM_SET_PRESCALER(&g_atim_pwmin_handle, ATIM_PWMIN_PSC_DEFAULT);
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
    return (g_atim_pwmin_sm == ATIM_PWMIN_SM_DONE) ? ATIM_PWMIN_DONE : ATIM_PWMIN_IDLE;
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
    if (g_atim_pwmin_sm == ATIM_PWMIN_SM_DONE)
    {
        g_atim_pwmin_psc = ATIM_PWMIN_PSC_DEFAULT;
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
            g_atim_pwmin_sm = ATIM_PWMIN_SM_IDLE;

            if (g_atim_pwmin_psc == ATIM_PWMIN_PSC_DEFAULT)
            {
                g_atim_pwmin_psc = ATIM_PWMIN_PSC_FIRST;
            }
            else if (g_atim_pwmin_psc == ATIM_PWMIN_PSC_MAX)
            {
                g_atim_pwmin_psc = ATIM_PWMIN_PSC_DEFAULT;
            }
            else if (g_atim_pwmin_psc > ATIM_PWMIN_PSC_DOUBLE_LIMIT)
            {
                g_atim_pwmin_psc = ATIM_PWMIN_PSC_MAX;
            }
            else
            {
                g_atim_pwmin_psc = (uint16_t)(g_atim_pwmin_psc * ATIM_PWMIN_PSC_STEP);
            }

            __HAL_TIM_SET_PRESCALER(&g_atim_pwmin_handle, g_atim_pwmin_psc);
            __HAL_TIM_SET_COUNTER(&g_atim_pwmin_handle, 0);
            __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1);
            __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC2);
            __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);
            return;
        }
    }

    if (g_atim_pwmin_sm != ATIM_PWMIN_SM_ARMED)
    {
        if (__HAL_TIM_GET_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1))
        {
            g_atim_pwmin_sm = ATIM_PWMIN_SM_ARMED;
        }
        __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1);
        __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC2);
        __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);
        return;
    }

    if (__HAL_TIM_GET_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1))
    {
        g_atim_pwmin_hval = HAL_TIM_ReadCapturedValue(&g_atim_pwmin_handle, TIM_CHANNEL_2) + ATIM_PWMIN_TICKS_OFFSET;
        g_atim_pwmin_cval = HAL_TIM_ReadCapturedValue(&g_atim_pwmin_handle, TIM_CHANNEL_1) + ATIM_PWMIN_TICKS_OFFSET;

        if (g_atim_pwmin_hval < g_atim_pwmin_cval)
        {
            g_atim_pwmin_sm  = ATIM_PWMIN_SM_DONE;
            g_atim_pwmin_psc = (uint16_t)__HAL_TIM_GET_PRESCALER(&g_atim_pwmin_handle);

            if (g_atim_pwmin_psc == ATIM_PWMIN_PSC_DEFAULT)
            {
                g_atim_pwmin_hval++;
                g_atim_pwmin_cval++;
            }

            __HAL_TIM_DISABLE(&g_atim_pwmin_handle);
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

    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1);
    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC2);
    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);
}

/* ===================== shared TIM8 handlers ===================== */

void TIM8_UP_TIM13_IRQHandler(void)
{
    if (g_atim_up_hook != 0)
    {
        g_atim_up_hook();
    }
}

void TIM8_CC_IRQHandler(void)
{
    if (g_atim_cc_hook != 0)
    {
        g_atim_cc_hook();
    }
}
