/**
 * @file    tim.c
 * @brief   Unified timer driver (TIM1..TIM14).
 *
 * All hardware facts live in the static tim_hw_t table; per-instance state is
 * held in tim_handle_t. GPIO comes from the generic gpio_hw helper. Only the
 * hardware is configured and raw events are reported through callbacks; all
 * measurement/decision logic belongs to the caller.
 */

#include "stm32f4xx_hal.h"
#include "tim.h"
#include "gpio_hw.h"

/* This HAL release lacks a repetition-counter setter macro. */
#ifndef __HAL_TIM_SET_REPETITIONCOUNTER
#define __HAL_TIM_SET_REPETITIONCOUNTER(__HANDLE__, __REPET__) \
    ((__HANDLE__)->Instance->RCR = (__REPET__))
#endif

/* This HAL release only provides the setter form of the prescaler macro. */
#ifndef __HAL_TIM_GET_PRESCALER
#define __HAL_TIM_GET_PRESCALER(__HANDLE__) ((__HANDLE__)->Instance->PSC)
#endif

#define TIM_IRQ_NONE ((IRQn_Type)0x7FFFFFFF)
#define TIM_NPWM_BATCH_COUNT 256U

/* ===== hardware descriptors ===== */

typedef struct
{
    TIM_TypeDef       *instance;
    volatile uint32_t *rcc_reg;   /* &RCC->APB1ENR / &RCC->APB2ENR */
    uint32_t           rcc_en;    /* RCC_APBxENR_TIMxEN */
    IRQn_Type          irqn;      /* update/UP IRQ (TIM_IRQ_NONE when unused) */
    IRQn_Type          cc_irqn;   /* capture/compare IRQ */
    gpio_hw_t          gpio[TIM_CH_NUM];
} tim_hw_t;

#define TIM_PIN(port, en, pin, af) \
    { (port), (en), (pin), GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, (af) }

static const tim_hw_t g_tim_hw[TIM_ID_NUM] =
{
    [TIM_ID_1] = {
        .instance = TIM1, .rcc_reg = &RCC->APB2ENR, .rcc_en = RCC_APB2ENR_TIM1EN,
        .irqn = TIM1_UP_TIM10_IRQn, .cc_irqn = TIM1_CC_IRQn,
        .gpio = { TIM_PIN(GPIOA, RCC_AHB1ENR_GPIOAEN, GPIO_PIN_8, GPIO_AF1_TIM1) },
    },
    [TIM_ID_2] = {
        .instance = TIM2, .rcc_reg = &RCC->APB1ENR, .rcc_en = RCC_APB1ENR_TIM2EN,
        .irqn = TIM2_IRQn, .cc_irqn = TIM_IRQ_NONE,
        .gpio = { TIM_PIN(GPIOA, RCC_AHB1ENR_GPIOAEN, GPIO_PIN_0, GPIO_AF1_TIM2) },
    },
    [TIM_ID_3] = {
        .instance = TIM3, .rcc_reg = &RCC->APB1ENR, .rcc_en = RCC_APB1ENR_TIM3EN,
        .irqn = TIM3_IRQn, .cc_irqn = TIM_IRQ_NONE,
        .gpio = { { 0 }, { 0 }, { 0 },
                  TIM_PIN(GPIOB, RCC_AHB1ENR_GPIOBEN, GPIO_PIN_1, GPIO_AF2_TIM3) },
    },
    [TIM_ID_4] = {
        .instance = TIM4, .rcc_reg = &RCC->APB1ENR, .rcc_en = RCC_APB1ENR_TIM4EN,
        .irqn = TIM_IRQ_NONE, .cc_irqn = TIM_IRQ_NONE,
    },
    [TIM_ID_5] = {
        .instance = TIM5, .rcc_reg = &RCC->APB1ENR, .rcc_en = RCC_APB1ENR_TIM5EN,
        .irqn = TIM5_IRQn, .cc_irqn = TIM_IRQ_NONE,
        .gpio = { TIM_PIN(GPIOA, RCC_AHB1ENR_GPIOAEN, GPIO_PIN_0, GPIO_AF2_TIM5) },
    },
    [TIM_ID_6] = {
        .instance = TIM6, .rcc_reg = &RCC->APB1ENR, .rcc_en = RCC_APB1ENR_TIM6EN,
        .irqn = TIM6_DAC_IRQn, .cc_irqn = TIM_IRQ_NONE,
    },
    [TIM_ID_7] = {
        .instance = TIM7, .rcc_reg = &RCC->APB1ENR, .rcc_en = RCC_APB1ENR_TIM7EN,
        .irqn = TIM7_IRQn, .cc_irqn = TIM_IRQ_NONE,
    },
    [TIM_ID_8] = {
        .instance = TIM8, .rcc_reg = &RCC->APB2ENR, .rcc_en = RCC_APB2ENR_TIM8EN,
        .irqn = TIM8_UP_TIM13_IRQn, .cc_irqn = TIM8_CC_IRQn,
        .gpio = {
            TIM_PIN(GPIOC, RCC_AHB1ENR_GPIOCEN, GPIO_PIN_6, GPIO_AF3_TIM8),
            TIM_PIN(GPIOC, RCC_AHB1ENR_GPIOCEN, GPIO_PIN_7, GPIO_AF3_TIM8),
            TIM_PIN(GPIOC, RCC_AHB1ENR_GPIOCEN, GPIO_PIN_8, GPIO_AF3_TIM8),
            TIM_PIN(GPIOC, RCC_AHB1ENR_GPIOCEN, GPIO_PIN_9, GPIO_AF3_TIM8),
        },
    },
    [TIM_ID_9] = {
        .instance = TIM9, .rcc_reg = &RCC->APB2ENR, .rcc_en = RCC_APB2ENR_TIM9EN,
        .irqn = TIM_IRQ_NONE, .cc_irqn = TIM_IRQ_NONE,
        .gpio = { { 0 },
                  TIM_PIN(GPIOA, RCC_AHB1ENR_GPIOAEN, GPIO_PIN_3, GPIO_AF3_TIM9) },
    },
    [TIM_ID_14] = {
        .instance = TIM14, .rcc_reg = &RCC->APB1ENR, .rcc_en = RCC_APB1ENR_TIM14EN,
        .irqn = TIM8_TRG_COM_TIM14_IRQn, .cc_irqn = TIM_IRQ_NONE,
    },
};

/* ===== per-instance state ===== */

typedef struct
{
    const tim_hw_t *hw;
    tim_cfg_t       cfg;
    TIM_HandleTypeDef htim;
    tim_edge_t      cap_edge;      /* IC: edge currently armed */
    uint32_t        npwm_remain;   /* NPWM: pulses left */
    TIM_BreakDeadTimeConfigTypeDef break_cfg; /* CPLM */
} tim_handle_t;

static tim_handle_t g_tim[TIM_ID_NUM];

/* ===== helpers ===== */

static uint32_t tim_channel_hal(tim_channel_t ch)
{
    switch (ch)
    {
        case TIM_CH1: return TIM_CHANNEL_1;
        case TIM_CH2: return TIM_CHANNEL_2;
        case TIM_CH3: return TIM_CHANNEL_3;
        case TIM_CH4: return TIM_CHANNEL_4;
        default:      return TIM_CHANNEL_1;
    }
}

static uint32_t tim_pull_hal(tim_pull_t pull)
{
    if (pull == TIM_PULL_UP)
    {
        return GPIO_PULLUP;
    }
    if (pull == TIM_PULL_DOWN)
    {
        return GPIO_PULLDOWN;
    }
    return GPIO_NOPULL;
}

/* Special case (mirrors ADC_TEMP_CH): TIM2_CH1 input capture is wired to PA5
 * (tpad), not the descriptor default PA0 used by the external counter. */
static bool tim_pin_special(tim_id_t id, tim_channel_t ch, tim_mode_t mode, gpio_hw_t *out)
{
    if ((id == TIM_ID_2) && (ch == TIM_CH1) && (mode == TIM_MODE_IC))
    {
        out->port      = GPIOA;
        out->rcc_en    = RCC_AHB1ENR_GPIOAEN;
        out->pin       = GPIO_PIN_5;
        out->mode      = GPIO_MODE_AF_PP;
        out->pull      = GPIO_NOPULL;
        out->speed     = GPIO_SPEED_FREQ_HIGH;
        out->alternate = GPIO_AF1_TIM2;
        return true;
    }
    return false;
}

static void tim_pin_setup(const tim_hw_t *hw, tim_id_t id, tim_channel_t ch,
                          tim_mode_t mode, tim_pull_t pull)
{
    gpio_hw_t g;

    if (tim_pin_special(id, ch, mode, &g))
    {
        g.pull = tim_pull_hal(pull);
        gpio_hw_setup(&g);
        return;
    }
    g = hw->gpio[ch];
    g.pull = tim_pull_hal(pull);
    gpio_hw_setup(&g);
}

static void tim_nvic(IRQn_Type irqn, const tim_cfg_t *c)
{
    if (irqn == TIM_IRQ_NONE)
    {
        return;
    }
    HAL_NVIC_SetPriority(irqn, (uint32_t)c->irq_prio, (uint32_t)c->irq_sub);
    HAL_NVIC_EnableIRQ(irqn);
}

static void tim_base_fields(tim_handle_t *h, uint32_t clock_div, uint32_t arpe)
{
    h->htim.Instance               = h->hw->instance;
    h->htim.Init.Prescaler         = h->cfg.psc;
    h->htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    h->htim.Init.Period            = h->cfg.arr;
    h->htim.Init.ClockDivision     = clock_div;
    h->htim.Init.AutoReloadPreload = arpe;
}

/* ---- input capture arming (IC) ---- */

static void tim_cap_arm(tim_handle_t *h, tim_edge_t edge)
{
    uint32_t pol = (edge == TIM_EDGE_RISING) ? TIM_ICPOLARITY_RISING
                                             : TIM_ICPOLARITY_FALLING;

    TIM_RESET_CAPTUREPOLARITY(&h->htim, TIM_CHANNEL_1);
    TIM_SET_CAPTUREPOLARITY(&h->htim, TIM_CHANNEL_1, pol);
    h->cap_edge = edge;
}

static void tim_cap_reset(tim_handle_t *h)
{
    __HAL_TIM_DISABLE(&h->htim);
    __HAL_TIM_SET_COUNTER(&h->htim, 0U);
    __HAL_TIM_ENABLE(&h->htim);
}

/* ---- CPLM dead time ---- */

static void tim_cplm_dtg(tim_handle_t *h, uint8_t dtg)
{
    h->break_cfg.DeadTime = dtg;
    (void)HAL_TIMEx_ConfigBreakDeadTime(&h->htim, &h->break_cfg);
    __HAL_TIM_MOE_ENABLE(&h->htim);
}

/* ---- NPWM burst ---- */

static void tim_npwm_isr(tim_handle_t *h)
{
    uint16_t npwm = 0U;

    if (h->npwm_remain >= TIM_NPWM_BATCH_COUNT)
    {
        h->npwm_remain -= TIM_NPWM_BATCH_COUNT;
        npwm = TIM_NPWM_BATCH_COUNT;
    }
    else if ((h->npwm_remain % TIM_NPWM_BATCH_COUNT) != 0U)
    {
        npwm = (uint16_t)h->npwm_remain;
        h->npwm_remain = 0U;
    }

    if (npwm != 0U)
    {
        __HAL_TIM_SET_REPETITIONCOUNTER(&h->htim, (uint16_t)(npwm - 1U));
        HAL_TIM_GenerateEvent(&h->htim, TIM_EVENTSOURCE_UPDATE);
        __HAL_TIM_ENABLE(&h->htim);
    }
    else
    {
        __HAL_TIM_DISABLE(&h->htim);
    }
    __HAL_TIM_CLEAR_IT(&h->htim, TIM_IT_UPDATE);
}

static void tim_npwm_trigger(tim_handle_t *h, uint32_t npwm)
{
    if (npwm == 0U)
    {
        return;
    }
    h->npwm_remain = npwm;
    HAL_TIM_GenerateEvent(&h->htim, TIM_EVENTSOURCE_UPDATE);
    __HAL_TIM_ENABLE(&h->htim);
}

/* ===== per-mode configuration ===== */

static void tim_cfg_base(tim_handle_t *h)
{
    TIM_MasterConfigTypeDef master = {0};

    tim_base_fields(h, TIM_CLOCKDIVISION_DIV1, TIM_AUTORELOAD_PRELOAD_DISABLE);
    (void)HAL_TIM_Base_Init(&h->htim);

    master.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    (void)HAL_TIMEx_MasterConfigSynchronization(&h->htim, &master);

    if (h->cfg.update_cb != 0)
    {
        tim_nvic(h->hw->irqn, &h->cfg);
        (void)HAL_TIM_Base_Start_IT(&h->htim);
    }
    else
    {
        (void)HAL_TIM_Base_Start(&h->htim);
    }
}

static void tim_cfg_pwm(tim_handle_t *h)
{
    TIM_OC_InitTypeDef oc = {0};
    uint32_t ch = tim_channel_hal(h->cfg.channel);

    tim_base_fields(h, TIM_CLOCKDIVISION_DIV1, TIM_AUTORELOAD_PRELOAD_DISABLE);
    (void)HAL_TIM_PWM_Init(&h->htim);

    tim_pin_setup(h->hw, h->cfg.id, h->cfg.channel, TIM_MODE_PWM, h->cfg.pull);

    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = h->cfg.arr / 2U;
    oc.OCPolarity = (h->cfg.polarity == TIM_POL_LOW) ? TIM_OCPOLARITY_LOW
                                                     : TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    (void)HAL_TIM_PWM_ConfigChannel(&h->htim, &oc, ch);
    (void)HAL_TIM_PWM_Start(&h->htim, ch);
}

static void tim_cfg_oc(tim_handle_t *h)
{
    TIM_OC_InitTypeDef oc = {0};
    tim_channel_t ch;

    tim_base_fields(h, TIM_CLOCKDIVISION_DIV1, TIM_AUTORELOAD_PRELOAD_ENABLE);
    (void)HAL_TIM_OC_Init(&h->htim);

    oc.OCMode     = TIM_OCMODE_TOGGLE;
    oc.OCPolarity = (h->cfg.polarity == TIM_POL_LOW) ? TIM_OCPOLARITY_LOW
                                                     : TIM_OCPOLARITY_HIGH;
    for (ch = TIM_CH1; ch < TIM_CH_NUM; ch++)
    {
        uint32_t hal = tim_channel_hal(ch);
        tim_pin_setup(h->hw, h->cfg.id, ch, TIM_MODE_OC, h->cfg.pull);
        oc.Pulse = h->cfg.arr / 2U;
        if (ch == TIM_CH4)
        {
            oc.OCIdleState = TIM_OCIDLESTATE_RESET;
        }
        (void)HAL_TIM_OC_ConfigChannel(&h->htim, &oc, hal);
        __HAL_TIM_ENABLE_OCxPRELOAD(&h->htim, hal);
        (void)HAL_TIM_OC_Start(&h->htim, hal);
    }
}

static void tim_cfg_ic(tim_handle_t *h)
{
    TIM_IC_InitTypeDef ic = {0};
    uint32_t ch = tim_channel_hal(h->cfg.channel);
    tim_edge_t edge = (h->cfg.polarity == TIM_POL_LOW) ? TIM_EDGE_FALLING
                                                       : TIM_EDGE_RISING;

    tim_base_fields(h, TIM_CLOCKDIVISION_DIV1, TIM_AUTORELOAD_PRELOAD_DISABLE);
    (void)HAL_TIM_IC_Init(&h->htim);

    tim_pin_setup(h->hw, h->cfg.id, h->cfg.channel, TIM_MODE_IC, h->cfg.pull);

    ic.ICPolarity  = (edge == TIM_EDGE_RISING) ? TIM_ICPOLARITY_RISING
                                               : TIM_ICPOLARITY_FALLING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter    = h->cfg.ic_filter;
    (void)HAL_TIM_IC_ConfigChannel(&h->htim, &ic, ch);

    h->cap_edge = edge;

    if (h->cfg.capture_cb != 0)
    {
        tim_nvic(h->hw->irqn, &h->cfg);
        __HAL_TIM_ENABLE_IT(&h->htim, TIM_IT_UPDATE);
        (void)HAL_TIM_IC_Start_IT(&h->htim, ch);
    }
    else
    {
        (void)HAL_TIM_IC_Start(&h->htim, ch);
    }
}

static void tim_cfg_counter(tim_handle_t *h)
{
    TIM_SlaveConfigTypeDef slave = {0};
    uint32_t ch = tim_channel_hal(h->cfg.channel);

    tim_base_fields(h, TIM_CLOCKDIVISION_DIV1, TIM_AUTORELOAD_PRELOAD_DISABLE);
    (void)HAL_TIM_IC_Init(&h->htim);

    tim_pin_setup(h->hw, h->cfg.id, h->cfg.channel, TIM_MODE_COUNTER, h->cfg.pull);

    slave.SlaveMode        = TIM_SLAVEMODE_EXTERNAL1;
    slave.InputTrigger     = TIM_TS_TI1FP1;
    slave.TriggerPolarity  = TIM_TRIGGERPOLARITY_RISING;
    slave.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
    slave.TriggerFilter    = 0U;
    (void)HAL_TIM_SlaveConfigSynchro(&h->htim, &slave);

    if (h->cfg.update_cb != 0)
    {
        tim_nvic(h->hw->irqn, &h->cfg);
        __HAL_TIM_ENABLE_IT(&h->htim, TIM_IT_UPDATE);
    }
    (void)HAL_TIM_IC_Start(&h->htim, ch);
}

static void tim_cfg_pwmin(tim_handle_t *h)
{
    TIM_IC_InitTypeDef ic = {0};
    TIM_SlaveConfigTypeDef slave = {0};

    tim_base_fields(h, TIM_CLOCKDIVISION_DIV1, TIM_AUTORELOAD_PRELOAD_DISABLE);
    (void)HAL_TIM_IC_Init(&h->htim);

    tim_pin_setup(h->hw, h->cfg.id, TIM_CH1, TIM_MODE_PWMIN, h->cfg.pull);

    slave.SlaveMode       = TIM_SLAVEMODE_RESET;
    slave.InputTrigger    = TIM_TS_TI1FP1;
    slave.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    slave.TriggerFilter   = 0U;
    (void)HAL_TIM_SlaveConfigSynchro(&h->htim, &slave);

    ic.ICPolarity  = TIM_INPUTCHANNELPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter    = h->cfg.ic_filter;
    (void)HAL_TIM_IC_ConfigChannel(&h->htim, &ic, TIM_CHANNEL_1);

    ic.ICPolarity  = TIM_INPUTCHANNELPOLARITY_FALLING;
    ic.ICSelection = TIM_ICSELECTION_INDIRECTTI;
    (void)HAL_TIM_IC_ConfigChannel(&h->htim, &ic, TIM_CHANNEL_2);

    if (h->cfg.capture_cb != 0)
    {
        tim_nvic(h->hw->irqn, &h->cfg);
        tim_nvic(h->hw->cc_irqn, &h->cfg);
        __HAL_TIM_ENABLE_IT(&h->htim, TIM_IT_UPDATE);
        (void)HAL_TIM_IC_Start_IT(&h->htim, TIM_CHANNEL_1);
        (void)HAL_TIM_IC_Start_IT(&h->htim, TIM_CHANNEL_2);
    }
}

static void tim_cfg_cplm(tim_handle_t *h)
{
    TIM_OC_InitTypeDef oc = {0};
    gpio_hw_t pe = { GPIOE, RCC_AHB1ENR_GPIOEEN, 0U, GPIO_MODE_AF_PP,
                     GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF1_TIM1 };

    tim_base_fields(h, TIM_CLOCKDIVISION_DIV4, TIM_AUTORELOAD_PRELOAD_ENABLE);
    (void)HAL_TIM_PWM_Init(&h->htim);

    pe.pull = tim_pull_hal(h->cfg.pull);
    pe.pin = GPIO_PIN_9;  gpio_hw_setup(&pe);   /* CH1  */
    pe.pin = GPIO_PIN_8;  gpio_hw_setup(&pe);   /* CH1N */
    pe.pin = GPIO_PIN_15; gpio_hw_setup(&pe);   /* BKIN */

    oc.OCMode       = TIM_OCMODE_PWM1;
    oc.OCPolarity   = TIM_OCPOLARITY_LOW;
    oc.OCNPolarity  = TIM_OCNPOLARITY_LOW;
    oc.OCIdleState  = TIM_OCIDLESTATE_SET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_SET;
    (void)HAL_TIM_PWM_ConfigChannel(&h->htim, &oc, TIM_CHANNEL_1);

    h->break_cfg.OffStateRunMode  = TIM_OSSR_DISABLE;
    h->break_cfg.OffStateIDLEMode = TIM_OSSI_DISABLE;
    h->break_cfg.LockLevel        = TIM_LOCKLEVEL_OFF;
    h->break_cfg.BreakState       = TIM_BREAK_ENABLE;
    h->break_cfg.BreakPolarity    = TIM_BREAKPOLARITY_LOW;
    h->break_cfg.AutomaticOutput  = TIM_AUTOMATICOUTPUT_ENABLE;
    h->break_cfg.DeadTime         = 0U;
    (void)HAL_TIMEx_ConfigBreakDeadTime(&h->htim, &h->break_cfg);

    (void)HAL_TIM_PWM_Start(&h->htim, TIM_CHANNEL_1);
    (void)HAL_TIMEx_PWMN_Start(&h->htim, TIM_CHANNEL_1);
}

static void tim_cfg_npwm(tim_handle_t *h)
{
    TIM_OC_InitTypeDef oc = {0};

    tim_base_fields(h, TIM_CLOCKDIVISION_DIV1, TIM_AUTORELOAD_PRELOAD_ENABLE);
    h->htim.Init.RepetitionCounter = 0U;
    (void)HAL_TIM_PWM_Init(&h->htim);

    tim_pin_setup(h->hw, h->cfg.id, TIM_CH1, TIM_MODE_NPWM, h->cfg.pull);

    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = h->cfg.arr / 2U;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    (void)HAL_TIM_PWM_ConfigChannel(&h->htim, &oc, TIM_CHANNEL_1);

    h->npwm_remain = 0U;
    tim_nvic(h->hw->irqn, &h->cfg);
    __HAL_TIM_ENABLE_IT(&h->htim, TIM_IT_UPDATE);
    (void)HAL_TIM_PWM_Start(&h->htim, TIM_CHANNEL_1);
}

/* ===== public API ===== */

void tim_init(const tim_cfg_t *cfg)
{
    static const tim_cfg_t cfg_default = { TIM_CFG_DEFAULT };
    const tim_cfg_t *c = (cfg != 0) ? cfg : &cfg_default;
    tim_handle_t    *h;

    if ((c->id >= TIM_ID_NUM) || (g_tim_hw[c->id].instance == 0))
    {
        return; /* reserved/unwired instance */
    }
    h = &g_tim[c->id];
    h->hw  = &g_tim_hw[c->id];
    h->cfg = *c;

    SET_BIT(*h->hw->rcc_reg, h->hw->rcc_en);

    switch (c->mode)
    {
        case TIM_MODE_BASE:    tim_cfg_base(h);    break;
        case TIM_MODE_PWM:     tim_cfg_pwm(h);     break;
        case TIM_MODE_OC:      tim_cfg_oc(h);      break;
        case TIM_MODE_IC:      tim_cfg_ic(h);      break;
        case TIM_MODE_COUNTER: tim_cfg_counter(h); break;
        case TIM_MODE_PWMIN:   tim_cfg_pwmin(h);   break;
        case TIM_MODE_CPLM:    tim_cfg_cplm(h);    break;
        case TIM_MODE_NPWM:    tim_cfg_npwm(h);    break;
        default: break;
    }
}

void tim_enable(tim_id_t id, bool on)
{
    tim_handle_t *h;

    if (id >= TIM_ID_NUM)
    {
        return;
    }
    h = &g_tim[id];
    if (h->hw == 0)
    {
        return;
    }
    if (on)
    {
        __HAL_TIM_ENABLE(&h->htim);
    }
    else
    {
        __HAL_TIM_DISABLE(&h->htim);
    }
}

void tim_set(tim_id_t id, tim_channel_t ch, tim_param_t param, uint32_t value)
{
    tim_handle_t *h;

    if (id >= TIM_ID_NUM)
    {
        return;
    }
    h = &g_tim[id];
    if (h->hw == 0)
    {
        return;
    }

    switch (param)
    {
        case TIM_PARAM_CCR:
            __HAL_TIM_SET_COMPARE(&h->htim, tim_channel_hal(ch), value);
            break;
        case TIM_PARAM_COUNT:
            __HAL_TIM_SET_COUNTER(&h->htim, value);
            break;
        case TIM_PARAM_FLAG:
        {
            uint32_t f = 0U;
            if ((value & TIM_PEND_UPDATE) != 0U)
            {
                f |= TIM_FLAG_UPDATE;
            }
            if ((value & TIM_PEND_CC) != 0U)
            {
                f |= TIM_FLAG_CC1 | TIM_FLAG_CC2 | TIM_FLAG_CC3 | TIM_FLAG_CC4;
            }
            __HAL_TIM_CLEAR_FLAG(&h->htim, f);
            break;
        }
        case TIM_PARAM_PSC:
            __HAL_TIM_SET_PRESCALER(&h->htim, (uint16_t)value);
            break;
        case TIM_PARAM_DTG:
            tim_cplm_dtg(h, (uint8_t)value);
            break;
        case TIM_PARAM_BURST:
            tim_npwm_trigger(h, value);
            break;
        default:
            break;
    }
}

uint32_t tim_get(tim_id_t id, tim_channel_t ch, tim_param_t param)
{
    tim_handle_t *h;

    if (id >= TIM_ID_NUM)
    {
        return 0U;
    }
    h = &g_tim[id];
    if (h->hw == 0)
    {
        return 0U;
    }

    switch (param)
    {
        case TIM_PARAM_CCR:
            return HAL_TIM_ReadCapturedValue(&h->htim, tim_channel_hal(ch));
        case TIM_PARAM_COUNT:
            return __HAL_TIM_GET_COUNTER(&h->htim);
        case TIM_PARAM_FLAG:
        {
            uint32_t f = 0U;
            if (__HAL_TIM_GET_FLAG(&h->htim, TIM_FLAG_UPDATE) != RESET)
            {
                f |= TIM_PEND_UPDATE;
            }
            if (__HAL_TIM_GET_FLAG(&h->htim, TIM_FLAG_CC1) != RESET)
            {
                f |= TIM_PEND_CC;
            }
            return f;
        }
        case TIM_PARAM_PSC:
            return __HAL_TIM_GET_PRESCALER(&h->htim);
        default:
            return 0U;
    }
}

/* ===== interrupt dispatch ===== */

static void tim_update_irq(tim_id_t id)
{
    tim_handle_t *h = &g_tim[id];

    if ((h->hw == 0) || (__HAL_TIM_GET_FLAG(&h->htim, TIM_FLAG_UPDATE) == RESET))
    {
        return;
    }
    __HAL_TIM_CLEAR_FLAG(&h->htim, TIM_FLAG_UPDATE);

    if (h->cfg.mode == TIM_MODE_NPWM)
    {
        tim_npwm_isr(h);
        return;
    }
    if (h->cfg.update_cb != 0)
    {
        h->cfg.update_cb();
    }
}

static void tim_cc_irq(tim_id_t id)
{
    tim_handle_t *h = &g_tim[id];

    if (h->hw == 0)
    {
        return;
    }

    if (__HAL_TIM_GET_FLAG(&h->htim, TIM_FLAG_CC1) != RESET)
    {
        uint32_t value = HAL_TIM_ReadCapturedValue(&h->htim, TIM_CHANNEL_1);
        __HAL_TIM_CLEAR_FLAG(&h->htim, TIM_FLAG_CC1);

        if (h->cfg.mode == TIM_MODE_IC)
        {
            tim_edge_t edge = h->cap_edge;
            tim_edge_t next = edge;

            if (edge == TIM_EDGE_RISING)
            {
                tim_cap_reset(h);
            }
            if (h->cfg.capture_cb != 0)
            {
                next = h->cfg.capture_cb(TIM_CAP_CH1, value, edge);
            }
            tim_cap_arm(h, next);
        }
        else if (h->cfg.mode == TIM_MODE_PWMIN)
        {
            if (h->cfg.capture_cb != 0)
            {
                (void)h->cfg.capture_cb(TIM_CAP_CH1, value, TIM_EDGE_RISING);
            }
        }
    }

    if (__HAL_TIM_GET_FLAG(&h->htim, TIM_FLAG_CC2) != RESET)
    {
        uint32_t value = HAL_TIM_ReadCapturedValue(&h->htim, TIM_CHANNEL_2);
        __HAL_TIM_CLEAR_FLAG(&h->htim, TIM_FLAG_CC2);

        if ((h->cfg.mode == TIM_MODE_PWMIN) && (h->cfg.capture_cb != 0))
        {
            (void)h->cfg.capture_cb(TIM_CAP_CH2, value, TIM_EDGE_FALLING);
        }
    }
}

void TIM1_UP_TIM10_IRQHandler(void)   { tim_update_irq(TIM_ID_1); }
void TIM1_CC_IRQHandler(void)         { tim_cc_irq(TIM_ID_1); }
void TIM2_IRQHandler(void)            { tim_update_irq(TIM_ID_2); tim_cc_irq(TIM_ID_2); }
void TIM3_IRQHandler(void)            { tim_update_irq(TIM_ID_3); tim_cc_irq(TIM_ID_3); }
void TIM5_IRQHandler(void)            { tim_update_irq(TIM_ID_5); tim_cc_irq(TIM_ID_5); }
void TIM6_DAC_IRQHandler(void)        { tim_update_irq(TIM_ID_6); }
void TIM7_IRQHandler(void)            { tim_update_irq(TIM_ID_7); }
void TIM8_UP_TIM13_IRQHandler(void)   { tim_update_irq(TIM_ID_8); }
void TIM8_CC_IRQHandler(void)         { tim_cc_irq(TIM_ID_8); }
void TIM8_TRG_COM_TIM14_IRQHandler(void) { tim_update_irq(TIM_ID_14); }
