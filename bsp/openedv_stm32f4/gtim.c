/**
 * @file    gtim.c
 * @brief   General timer driver: TIM3 update, TIM3_CH4 PWM, TIM5_CH1 capture,
 *          TIM2_CH1 external counter and the TIM14 1 Hz frame-rate counter.
 *          MSP content is inlined; dispatch uses function pointers and an
 *          explicit capture state machine.
 */

#include <stdio.h>

#include "stm32f4xx_hal.h"
#include "gtim.h"
#include "sys.h"

#define GTIM_TIMER_MODULUS     0x10000U
#define GTIM_TIMER_MAX_COUNT   0xFFFFU
#define GTIM_PWM_HALF_DUTY_DIV 2U
#define GTIM_CAP_MAX_OVERFLOW_COUNT  63U
#define GTIM_CAP_INVALID_VALUE 0xFFFFU

/* ---- TIM3 update interrupt ---- */
static TIM_HandleTypeDef g_gtim_int_handle;
static gtim_cb_t         g_gtim_int_cb;

void gtim_timx_int_init(uint16_t arr, uint16_t psc)
{
    __HAL_RCC_TIM3_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM3_IRQn, 1U, 3U);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    g_gtim_int_handle.Instance         = TIM3;
    g_gtim_int_handle.Init.Prescaler   = psc;
    g_gtim_int_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_gtim_int_handle.Init.Period      = arr;
    HAL_TIM_Base_Init(&g_gtim_int_handle);
    HAL_TIM_Base_Start_IT(&g_gtim_int_handle);
}

void gtim_timx_int_register(gtim_cb_t cb)
{
    g_gtim_int_cb = cb;
}

void TIM3_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_gtim_int_handle, TIM_FLAG_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_gtim_int_handle, TIM_FLAG_UPDATE);
        if (g_gtim_int_cb != 0)
        {
            g_gtim_int_cb();
        }
    }
}

/* ---- TIM3_CH4 (PB1) PWM ---- */
static TIM_HandleTypeDef g_gtim_pwm_handle;

void gtim_timx_pwm_chy_init(uint16_t arr, uint16_t psc)
{
    GPIO_InitTypeDef gpio_init = {0};
    TIM_OC_InitTypeDef oc      = {0};

    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin       = GPIO_PIN_1;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    g_gtim_pwm_handle.Instance         = TIM3;
    g_gtim_pwm_handle.Init.Prescaler   = psc;
    g_gtim_pwm_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_gtim_pwm_handle.Init.Period      = arr;
    HAL_TIM_PWM_Init(&g_gtim_pwm_handle);

    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = arr / GTIM_PWM_HALF_DUTY_DIV;
    oc.OCPolarity = TIM_OCPOLARITY_LOW;
    HAL_TIM_PWM_ConfigChannel(&g_gtim_pwm_handle, &oc, TIM_CHANNEL_4);
    HAL_TIM_PWM_Start(&g_gtim_pwm_handle, TIM_CHANNEL_4);
}

void gtim_timx_pwm_chy_set(uint16_t ccr)
{
    __HAL_TIM_SET_COMPARE(&g_gtim_pwm_handle, TIM_CHANNEL_4, ccr);
}

/* ---- TIM5_CH1 (PA0) input capture ---- */
static TIM_HandleTypeDef g_gtim_cap_handle;
static gtim_cap_state_t  g_gtim_cap_state;
static uint32_t          g_gtim_cap_overflows;
static uint32_t          g_gtim_cap_value;

void gtim_timx_cap_chy_init(uint32_t arr, uint16_t psc)
{
    GPIO_InitTypeDef gpio_init = {0};
    TIM_IC_InitTypeDef ic      = {0};

    __HAL_RCC_TIM5_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM5_IRQn, 1U, 3U);
    HAL_NVIC_EnableIRQ(TIM5_IRQn);

    gpio_init.Pin       = GPIO_PIN_0;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLDOWN;
    gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    g_gtim_cap_handle.Instance         = TIM5;
    g_gtim_cap_handle.Init.Prescaler   = psc;
    g_gtim_cap_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_gtim_cap_handle.Init.Period      = arr;
    HAL_TIM_IC_Init(&g_gtim_cap_handle);

    ic.ICPolarity  = TIM_ICPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter    = 0;
    HAL_TIM_IC_ConfigChannel(&g_gtim_cap_handle, &ic, TIM_CHANNEL_1);

    g_gtim_cap_state     = GTIM_CAP_IDLE;
    g_gtim_cap_overflows = 0;
    g_gtim_cap_value     = 0;

    __HAL_TIM_ENABLE_IT(&g_gtim_cap_handle, TIM_IT_UPDATE);
    HAL_TIM_IC_Start_IT(&g_gtim_cap_handle, TIM_CHANNEL_1);
}

gtim_cap_state_t gtim_timx_cap_chy_state(void)
{
    return g_gtim_cap_state;
}

uint32_t gtim_timx_cap_chy_value(void)
{
    uint32_t value;

    sys_intx_disable();
    value = (g_gtim_cap_overflows * GTIM_TIMER_MODULUS) + g_gtim_cap_value;
    sys_intx_enable();

    return value;
}

void gtim_timx_cap_chy_clear(void)
{
    g_gtim_cap_state     = GTIM_CAP_IDLE;
    g_gtim_cap_overflows = 0;
    g_gtim_cap_value     = 0;
}

void TIM5_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_gtim_cap_handle, TIM_FLAG_CC1) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_gtim_cap_handle, TIM_FLAG_CC1);

        if (g_gtim_cap_state == GTIM_CAP_IDLE)
        {
            g_gtim_cap_state     = GTIM_CAP_RISING;
            g_gtim_cap_overflows = 0;
            __HAL_TIM_DISABLE(&g_gtim_cap_handle);
            __HAL_TIM_SET_COUNTER(&g_gtim_cap_handle, 0);
            TIM_RESET_CAPTUREPOLARITY(&g_gtim_cap_handle, TIM_CHANNEL_1);
            TIM_SET_CAPTUREPOLARITY(&g_gtim_cap_handle, TIM_CHANNEL_1, TIM_ICPOLARITY_FALLING);
            __HAL_TIM_ENABLE(&g_gtim_cap_handle);
        }
        else if (g_gtim_cap_state == GTIM_CAP_RISING)
        {
            g_gtim_cap_value = HAL_TIM_ReadCapturedValue(&g_gtim_cap_handle, TIM_CHANNEL_1);
            g_gtim_cap_state = GTIM_CAP_DONE;
            TIM_RESET_CAPTUREPOLARITY(&g_gtim_cap_handle, TIM_CHANNEL_1);
            TIM_SET_CAPTUREPOLARITY(&g_gtim_cap_handle, TIM_CHANNEL_1, TIM_ICPOLARITY_RISING);
        }
    }

    if (__HAL_TIM_GET_FLAG(&g_gtim_cap_handle, TIM_FLAG_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_gtim_cap_handle, TIM_FLAG_UPDATE);

        if (g_gtim_cap_state == GTIM_CAP_RISING)
        {
            g_gtim_cap_overflows++;
            if (g_gtim_cap_overflows >= GTIM_CAP_MAX_OVERFLOW_COUNT)
            {
                TIM_RESET_CAPTUREPOLARITY(&g_gtim_cap_handle, TIM_CHANNEL_1);
                TIM_SET_CAPTUREPOLARITY(&g_gtim_cap_handle, TIM_CHANNEL_1, TIM_ICPOLARITY_RISING);
                g_gtim_cap_value = GTIM_CAP_INVALID_VALUE;
                g_gtim_cap_state = GTIM_CAP_DONE;
            }
        }
    }
}

/* ---- TIM2_CH1 (PA0) external pulse counter ---- */
static TIM_HandleTypeDef g_gtim_cnt_handle;
static uint32_t          g_gtim_cnt_overflows;

void gtim_timx_cnt_chy_init(uint16_t psc)
{
    GPIO_InitTypeDef gpio_init = {0};
    TIM_SlaveConfigTypeDef slave = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM2_IRQn, 1U, 3U);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    gpio_init.Pin       = GPIO_PIN_0;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLDOWN;
    gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    g_gtim_cnt_handle.Instance         = TIM2;
    g_gtim_cnt_handle.Init.Prescaler   = psc;
    g_gtim_cnt_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_gtim_cnt_handle.Init.Period      = GTIM_TIMER_MAX_COUNT;
    HAL_TIM_IC_Init(&g_gtim_cnt_handle);

    slave.SlaveMode       = TIM_SLAVEMODE_EXTERNAL1;
    slave.InputTrigger    = TIM_TS_TI1FP1;
    slave.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
    slave.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
    slave.TriggerFilter   = 0;
    HAL_TIM_SlaveConfigSynchro(&g_gtim_cnt_handle, &slave);

    g_gtim_cnt_overflows = 0;
    __HAL_TIM_ENABLE_IT(&g_gtim_cnt_handle, TIM_IT_UPDATE);
    HAL_TIM_IC_Start(&g_gtim_cnt_handle, TIM_CHANNEL_1);
}

uint32_t gtim_timx_cnt_chy_get_count(void)
{
    uint32_t count;

    sys_intx_disable();
    count = (g_gtim_cnt_overflows * GTIM_TIMER_MODULUS) + __HAL_TIM_GET_COUNTER(&g_gtim_cnt_handle);
    sys_intx_enable();

    return count;
}

void gtim_timx_cnt_chy_restart(void)
{
    __HAL_TIM_DISABLE(&g_gtim_cnt_handle);
    g_gtim_cnt_overflows = 0;
    __HAL_TIM_SET_COUNTER(&g_gtim_cnt_handle, 0);
    __HAL_TIM_ENABLE(&g_gtim_cnt_handle);
}

void TIM2_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_gtim_cnt_handle, TIM_FLAG_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_gtim_cnt_handle, TIM_FLAG_UPDATE);
        g_gtim_cnt_overflows++;
    }
}

/* ---- TIM14 1 Hz frame-rate timer ---- */
#define GTIM_FRAME_TIMX                  TIM14
#define GTIM_FRAME_TIMX_IRQN             TIM8_TRG_COM_TIM14_IRQn
/* 90 MHz / (9000 * 10000) = 1 Hz. */
#define GTIM_FRAME_PRESCALER             9000U
#define GTIM_FRAME_PERIOD_TICK                10000U

static TIM_HandleTypeDef g_gtim_frame_handle;

static volatile uint32_t g_gtim_frame_count;
static volatile uint32_t g_gtim_frame_total;
static volatile uint32_t g_gtim_frame_rate;
static volatile uint32_t g_gtim_frame_uptime;

void gtim_frame_init(void)
{
    __HAL_RCC_TIM14_CLK_ENABLE();

    HAL_NVIC_SetPriority(GTIM_FRAME_TIMX_IRQN, 1U, 3U);
    HAL_NVIC_EnableIRQ(GTIM_FRAME_TIMX_IRQN);

    g_gtim_frame_handle.Instance           = GTIM_FRAME_TIMX;
    g_gtim_frame_handle.Init.Prescaler     = GTIM_FRAME_PRESCALER - 1U;
    g_gtim_frame_handle.Init.CounterMode   = TIM_COUNTERMODE_UP;
    g_gtim_frame_handle.Init.Period        = GTIM_FRAME_PERIOD_TICK - 1U;
    g_gtim_frame_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    (void)HAL_TIM_Base_Init(&g_gtim_frame_handle);
    (void)HAL_TIM_Base_Start_IT(&g_gtim_frame_handle);
}

void gtim_frame_inc(void)
{
    g_gtim_frame_count++;
    g_gtim_frame_total++;
}

uint32_t gtim_frame_count(void)
{
    return g_gtim_frame_total;
}

uint32_t gtim_frame_rate(void)
{
    return g_gtim_frame_rate;
}

uint32_t gtim_frame_uptime(void)
{
    return g_gtim_frame_uptime;
}

void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_gtim_frame_handle, TIM_FLAG_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_gtim_frame_handle, TIM_FLAG_UPDATE);

        g_gtim_frame_rate = g_gtim_frame_count;
        g_gtim_frame_count = 0U;
        g_gtim_frame_uptime++;

        printf("frame:%u\r\n", (unsigned int)g_gtim_frame_rate);
    }
}
