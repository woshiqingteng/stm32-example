/**
 * @file    ir.c
 * @brief   NEC infrared ir receiver, ported from the vendor IR example.
 *          TIM1_CH1 (PA8) is used in input-capture mode with 1 us resolution;
 *          the capture/update interrupts are handled directly.
 */

#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "ir.h"

#define IR_PRESCALER        (180U - 1U) /* 1 tick = 1 us at 180 MHz */
#define IR_PERIOD_TICK           10000U
#define IR_IC_FILTER        0x03U
#define IR_NVIC_PREEMP      1U
#define IR_NVIC_UP_SUB      3U
#define IR_NVIC_CC_SUB      2U

#define IR_REPEAT_MAX_COUNT       14U

/** @brief  NEC decoder state. */
typedef enum
{
    IR_STATE_IDLE = 0,   /*!< waiting for a leader */
    IR_STATE_FRAME = 1   /*!< leader seen, decoding the 32 bits */
} ir_state_t;

#define IR_BIT0_MIN_US         300U
#define IR_BIT0_MAX_US         800U
#define IR_BIT1_MIN_US         1400U
#define IR_BIT1_MAX_US         1800U
#define IR_REPEAT_MIN_US       2000U
#define IR_REPEAT_MAX_VAL_US   3000U
#define IR_LEAD_MIN_US         4200U
#define IR_LEAD_MAX_US         4700U

static TIM_HandleTypeDef g_ir_handle;
static ir_state_t        g_ir_state;
static bool              g_ir_in_high;      /* a high-level pulse is in progress */
static bool              g_ir_key_pending;  /* a decoded key waits in ir_scan() */
static uint32_t          g_ir_data;
static uint8_t           g_ir_timeout;      /* update ticks since the last capture */
static uint8_t           g_ir_cnt;          /* repeat-frame counter */

void ir_init(void)
{
    GPIO_InitTypeDef   gpio_init = {0};
    TIM_IC_InitTypeDef ic        = {0};

    /* ---- MSP begin: TIM1 clock + PA8 + NVIC ---- */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_init.Pin       = IR_IN_GPIO_PIN;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = IR_IN_GPIO_AF;
    HAL_GPIO_Init(IR_IN_GPIO_PORT, &gpio_init);

    HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, IR_NVIC_PREEMP, IR_NVIC_UP_SUB);
    HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
    HAL_NVIC_SetPriority(TIM1_CC_IRQn, IR_NVIC_PREEMP, IR_NVIC_CC_SUB);
    HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
    /* ---- MSP end ---- */

    g_ir_handle.Instance               = IR_IN_TIMX;
    g_ir_handle.Init.Prescaler         = IR_PRESCALER;
    g_ir_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_ir_handle.Init.Period            = IR_PERIOD_TICK;
    g_ir_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_ir_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    (void)HAL_TIM_IC_Init(&g_ir_handle);

    ic.ICPolarity  = TIM_ICPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter    = IR_IC_FILTER;
    (void)HAL_TIM_IC_ConfigChannel(&g_ir_handle, &ic, IR_IN_TIMX_CHY);

    g_ir_state       = IR_STATE_IDLE;
    g_ir_in_high     = false;
    g_ir_key_pending = false;
    g_ir_data        = 0U;
    g_ir_timeout     = 0U;
    g_ir_cnt         = 0U;

    __HAL_TIM_ENABLE_IT(&g_ir_handle, TIM_IT_UPDATE);
    (void)HAL_TIM_IC_Start_IT(&g_ir_handle, IR_IN_TIMX_CHY);
}

static void ir_update_isr(void)
{
    if (g_ir_state == IR_STATE_FRAME)
    {
        g_ir_in_high = false;

        if (g_ir_timeout == 0U)
        {
            g_ir_key_pending = true;
        }

        if (g_ir_timeout < IR_REPEAT_MAX_COUNT)
        {
            g_ir_timeout++;
        }
        else
        {
            g_ir_state   = IR_STATE_IDLE;
            g_ir_timeout = 0U;
        }
    }
}

static void ir_capture_isr(void)
{
    uint16_t dval;

    if (HAL_GPIO_ReadPin(IR_IN_GPIO_PORT, IR_IN_GPIO_PIN) != GPIO_PIN_RESET)
    {
        /* rising edge: restart timing */
        __HAL_TIM_SET_CAPTUREPOLARITY(&g_ir_handle, IR_IN_TIMX_CHY,
                                      TIM_INPUTCHANNELPOLARITY_FALLING);
        __HAL_TIM_SET_COUNTER(&g_ir_handle, 0U);
        g_ir_in_high = true;
    }
    else
    {
        /* falling edge: the high-level width carries the bit value */
        dval = (uint16_t)HAL_TIM_ReadCapturedValue(&g_ir_handle, IR_IN_TIMX_CHY);
        __HAL_TIM_SET_CAPTUREPOLARITY(&g_ir_handle, IR_IN_TIMX_CHY,
                                      TIM_INPUTCHANNELPOLARITY_RISING);

        if (g_ir_in_high)
        {
            if (g_ir_state == IR_STATE_FRAME)
            {
                if ((dval > IR_BIT0_MIN_US) && (dval < IR_BIT0_MAX_US))
                {
                    g_ir_data >>= 1;
                    g_ir_data &= ~(0x80000000U);
                }
                else if ((dval > IR_BIT1_MIN_US) && (dval < IR_BIT1_MAX_US))
                {
                    g_ir_data >>= 1;
                    g_ir_data |= 0x80000000U;
                }
                else if ((dval > IR_REPEAT_MIN_US) && (dval < IR_REPEAT_MAX_VAL_US))
                {
                    g_ir_cnt++;
                    g_ir_timeout = 0U;
                }
                else
                {
                    /* out-of-range pulse: ignore */
                }
            }
            else if ((dval > IR_LEAD_MIN_US) && (dval < IR_LEAD_MAX_US))
            {
                g_ir_state = IR_STATE_FRAME;
                g_ir_cnt   = 0U;
            }
            else
            {
                /* not a leader: stay idle */
            }
        }

        g_ir_in_high = false;
    }
}

void TIM1_CC_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_ir_handle, TIM_FLAG_CC1) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_ir_handle, TIM_FLAG_CC1);
        ir_capture_isr();
    }
}

void TIM1_UP_TIM10_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_ir_handle, TIM_FLAG_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_ir_handle, TIM_FLAG_UPDATE);
        ir_update_isr();
    }
}

uint8_t ir_parse(uint32_t frame)
{
    uint8_t addr;
    uint8_t addr_inv;
    uint8_t cmd;
    uint8_t cmd_inv;

    addr     = (uint8_t)frame;
    addr_inv = (uint8_t)(frame >> 8);

    if ((addr != (uint8_t)~addr_inv) || (addr != IR_ID))
    {
        return 0U;
    }

    cmd     = (uint8_t)(frame >> 16);
    cmd_inv = (uint8_t)(frame >> 24);

    if (cmd != (uint8_t)~cmd_inv)
    {
        return 0U;
    }

    return cmd;
}

uint8_t ir_scan(void)
{
    uint8_t key = 0U;

    if (g_ir_key_pending)
    {
        key = ir_parse(g_ir_data);

        if ((key == 0U) || (g_ir_state != IR_STATE_FRAME))
        {
            g_ir_key_pending = false;
            g_ir_cnt         = 0U;
        }
    }

    return key;
}

uint8_t ir_repeat_count(void)
{
    return g_ir_cnt;
}
