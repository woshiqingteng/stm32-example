/**
 * @file    atim.c
 * @brief   Advanced timer driver (TIM8 / TIM1). MSP content is inlined; the two
 *          TIM8 users (NPWM and PWM-input) register the ISR hooks invoked from
 *          the shared TIM8 handlers.
 */

#include "stm32f4xx_hal.h"
#include "atim.h"
#include "sys.h"

/* This HAL release only provides the setter form of the prescaler macro. */
#ifndef __HAL_TIM_GET_PRESCALER
#define __HAL_TIM_GET_PRESCALER(__HANDLE__) ((__HANDLE__)->Instance->PSC)
#endif

/* This HAL release has no repetition-counter (RCR) setter macro. */
#ifndef __HAL_TIM_SET_REPETITIONCOUNTER
#define __HAL_TIM_SET_REPETITIONCOUNTER(__HANDLE__, __REPET__) \
    ((__HANDLE__)->Instance->RCR = (__REPET__))
#endif

#define ATIM_NPWM_BATCH_COUNT              256U
#define ATIM_NPWM_DEFAULT_PULSE_DIV  2U
#define ATIM_REPETITION_COUNT      0U

/* Default compare ticks; the app overrides them via atim_timx_comp_pwm_set(). */
#define ATIM_OC_COMPARE_CH1_TICK          250U
#define ATIM_OC_COMPARE_CH2_TICK          500U
#define ATIM_OC_COMPARE_CH3_TICK          750U
#define ATIM_OC_COMPARE_CH4_TICK          1000U
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

/* ===================== TIM8 NPWM (PC6 / CH1) =====================
 * NPWM: RCR makes the update event fire every (RCR+1) PWM periods, so one
 * ISR entry covers one batch (<=256 pulses); the ISR then chains batches. */

static TIM_HandleTypeDef g_atim_npwm_handle;
static uint32_t          g_atim_npwm_remain;

/* TIM8 (APB2): f = 180 MHz/((PSC+1)(ARR+1)); burst pulses = RCR+1. */
void atim_timx_npwm_chy_init(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef gpio_init = {0};
    TIM_OC_InitTypeDef oc      = {0};

    __HAL_RCC_TIM8_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, 1U, 3U);
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
    g_atim_npwm_handle.Init.RepetitionCounter = ATIM_REPETITION_COUNT;
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
    /* Force a UEV (EGR.UG: reset CNT, reload registers, set UIF) so the ISR
     * loads batch 1 now. npwm == 0 is ignored and does not cancel a burst. */
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

    if (g_atim_npwm_remain >= ATIM_NPWM_BATCH_COUNT)
    {
        g_atim_npwm_remain -= ATIM_NPWM_BATCH_COUNT;
        npwm = ATIM_NPWM_BATCH_COUNT;
    }
    else if ((g_atim_npwm_remain % ATIM_NPWM_BATCH_COUNT) != 0U)
    {
        npwm = (uint16_t)g_atim_npwm_remain;
        g_atim_npwm_remain = 0;
    }

    if (npwm != 0U)
    {
        /* RCR = npwm-1 -> npwm pulses before the next update. The forced UEV
         * reloads RCR and resets CNT so the batch starts now. */
        __HAL_TIM_SET_REPETITIONCOUNTER(&g_atim_npwm_handle, (uint16_t)(npwm - 1U));
        HAL_TIM_GenerateEvent(&g_atim_npwm_handle, TIM_EVENTSOURCE_UPDATE);
        __HAL_TIM_ENABLE(&g_atim_npwm_handle);
    }
    else
    {
        __HAL_TIM_DISABLE(&g_atim_npwm_handle);
    }

    /* Clear UIF so our own forced UG does not re-enter immediately. */
    __HAL_TIM_CLEAR_IT(&g_atim_npwm_handle, TIM_IT_UPDATE);
}

/* ================= TIM8 output compare (PC6..PC9) ================= */
/* TIM8 (APB2): f = 180 MHz/((PSC+1)(ARR+1)); CHn = toggle, 50% duty, edge
 * (phase) at tick CHn; output frequency = half the counter rate. */

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

    oc.Pulse = ATIM_OC_COMPARE_CH1_TICK - 1U;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_1);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_1);

    oc.Pulse = ATIM_OC_COMPARE_CH2_TICK - 1U;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_2);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_2);

    oc.Pulse = ATIM_OC_COMPARE_CH3_TICK - 1U;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_3);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_3);

    oc.Pulse        = ATIM_OC_COMPARE_CH4_TICK - 1U;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    HAL_TIM_OC_ConfigChannel(&g_atim_comp_handle, &oc, TIM_CHANNEL_4);
    __HAL_TIM_ENABLE_OCxPRELOAD(&g_atim_comp_handle, TIM_CHANNEL_4);

    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_1);
    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_2);
    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_3);
    HAL_TIM_OC_Start(&g_atim_comp_handle, TIM_CHANNEL_4);
}

void atim_timx_comp_pwm_set(atim_channel_t channel, uint16_t ccr)
{
    uint32_t hal_channel;

    switch (channel)
    {
        case ATIM_CH1:
            hal_channel = TIM_CHANNEL_1;
            break;
        case ATIM_CH2:
            hal_channel = TIM_CHANNEL_2;
            break;
        case ATIM_CH3:
            hal_channel = TIM_CHANNEL_3;
            break;
        case ATIM_CH4:
            hal_channel = TIM_CHANNEL_4;
            break;
        default:
            hal_channel = TIM_CHANNEL_1;
            break;
    }

    __HAL_TIM_SET_COMPARE(&g_atim_comp_handle, hal_channel, ccr);
}

/* ============ TIM1 complementary PWM + dead time (PE9/PE8/PE15) ============ */
/* TIM1 (APB2): f = 180 MHz/((PSC+1)(ARR+1)); ClockDivision DIV4 -> t_DTS = 4/180 MHz.
 * OCPolarity/OCNPolarity = LOW -> OC1/OC1N active-low (inverted), keep both equal
 * to stay complementary; OCIdleState = SET -> both high at MOE=0. */

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
/* TIM8 (APB2): f = 180 MHz; PSC auto-doubles at runtime to keep ARR in range. */

typedef enum
{
    ATIM_PWMIN_IDLE = 0,  /*!< waiting for the first capture */
    ATIM_PWMIN_ARMED = 1, /*!< first capture discarded, measuring */
    ATIM_PWMIN_DONE = 2,  /*!< high and cycle times available */
} atim_pwmin_state_t;

static TIM_HandleTypeDef g_atim_pwmin_handle;
static atim_pwmin_state_t   g_atim_pwmin_state;
static uint16_t          g_atim_pwmin_psc;
static atim_pwmin_cb_t   g_atim_pwmin_cb;

void atim_timx_pwmin_chy_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    TIM_SlaveConfigTypeDef slave = {0};
    TIM_IC_InitTypeDef ic = {0};

    __HAL_RCC_TIM8_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, 1U, 3U);
    HAL_NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
    HAL_NVIC_SetPriority(TIM8_CC_IRQn, 1U, 3U);
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

    g_atim_pwmin_cb = 0;
    g_atim_up_hook = atim_pwmin_process;
    g_atim_cc_hook = atim_pwmin_process;

    __HAL_TIM_ENABLE_IT(&g_atim_pwmin_handle, TIM_IT_UPDATE);
    HAL_TIM_IC_Start_IT(&g_atim_pwmin_handle, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&g_atim_pwmin_handle, TIM_CHANNEL_2);
}

void atim_timx_pwmin_chy_register(atim_pwmin_cb_t cb)
{
    g_atim_pwmin_cb = cb;
}

/* Clear the capture/update flags. */
static void atim_pwmin_clear_flags(void)
{
    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1);
    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC2);
    __HAL_TIM_CLEAR_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);
}

/* Advance the ranging prescaler (0->1, x2, cap at MAX then wrap to 0) and apply. */
static void atim_pwmin_advance_psc(void)
{
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
}

/* Rearm acquisition; call with interrupts masked (or from the ISR). */
static void atim_pwmin_rearm(void)
{
    if (g_atim_pwmin_state == ATIM_PWMIN_DONE)
    {
        g_atim_pwmin_state = ATIM_PWMIN_IDLE;
    }

    g_atim_pwmin_psc = ATIM_PWMIN_PSC_DEFAULT;
    __HAL_TIM_SET_PRESCALER(&g_atim_pwmin_handle, ATIM_PWMIN_PSC_DEFAULT);
    __HAL_TIM_SET_COUNTER(&g_atim_pwmin_handle, 0);
    __HAL_TIM_ENABLE_IT(&g_atim_pwmin_handle, TIM_IT_CC1);
    __HAL_TIM_ENABLE_IT(&g_atim_pwmin_handle, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE(&g_atim_pwmin_handle);
    atim_pwmin_clear_flags();
}

void atim_timx_pwmin_chy_restart(void)
{
    sys_intx_disable();
    atim_pwmin_rearm();
    sys_intx_enable();
}

/* Stop acquisition and publish one measurement. */
static void atim_pwmin_finish(uint16_t psc, uint32_t hval, uint32_t cval)
{
    g_atim_pwmin_state = ATIM_PWMIN_DONE;
    __HAL_TIM_DISABLE(&g_atim_pwmin_handle);
    __HAL_TIM_DISABLE_IT(&g_atim_pwmin_handle, TIM_IT_CC1);
    __HAL_TIM_DISABLE_IT(&g_atim_pwmin_handle, TIM_IT_CC2);
    __HAL_TIM_DISABLE_IT(&g_atim_pwmin_handle, TIM_IT_UPDATE);
    atim_pwmin_clear_flags();

    if (g_atim_pwmin_cb != 0)
    {
        g_atim_pwmin_cb(psc, hval, cval);
    }
}

static void atim_pwmin_process(void)
{
    uint32_t upd = __HAL_TIM_GET_FLAG(&g_atim_pwmin_handle, TIM_FLAG_UPDATE);
    uint32_t cc1 = __HAL_TIM_GET_FLAG(&g_atim_pwmin_handle, TIM_FLAG_CC1);

    switch (g_atim_pwmin_state)
    {
        case ATIM_PWMIN_DONE:
            g_atim_pwmin_psc = ATIM_PWMIN_PSC_DEFAULT;
            __HAL_TIM_SET_COUNTER(&g_atim_pwmin_handle, 0);
            break;

        case ATIM_PWMIN_IDLE:
            if (cc1)
            {
                g_atim_pwmin_state = ATIM_PWMIN_ARMED;  /* discard the first edge */
            }
            else if (upd)
            {
                atim_pwmin_advance_psc();               /* timeout -> widen range */
            }
            break;

        case ATIM_PWMIN_ARMED:
            if (cc1)
            {
                uint32_t hval = HAL_TIM_ReadCapturedValue(&g_atim_pwmin_handle, TIM_CHANNEL_2) + ATIM_PWMIN_TICKS_OFFSET;
                uint32_t cval = HAL_TIM_ReadCapturedValue(&g_atim_pwmin_handle, TIM_CHANNEL_1) + ATIM_PWMIN_TICKS_OFFSET;

                if (hval < cval)
                {
                    if (g_atim_pwmin_psc == ATIM_PWMIN_PSC_DEFAULT)
                    {
                        hval++;
                        cval++;
                    }
                    atim_pwmin_finish(g_atim_pwmin_psc, hval, cval);
                }
                else
                {
                    atim_pwmin_rearm();                 /* glitch -> re-measure */
                }
            }
            else if (upd)
            {
                atim_pwmin_advance_psc();               /* timeout -> widen range */
            }
            break;

        default:
            break;
    }

    atim_pwmin_clear_flags();
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
