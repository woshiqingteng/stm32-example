/**
 * @file    tim.h
 * @brief   Unified HAL-free timer driver (TIM1..TIM14): base timebase, PWM,
 *          output-compare, input capture, external counter, PWM input,
 *          complementary PWM + dead time and N-pulse burst.
 *          This interface is HAL-free (own enums); the HAL is used only inside
 *          tim.c for peripheral initialisation. Every mode only configures the
 *          hardware and reports raw events; measurement/decision logic belongs
 *          to the caller.
 */

#ifndef BSP_TIM_H
#define BSP_TIM_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Timer instance selector (id, not the CMSIS TIMx pointer macro). */
typedef enum
{
    TIM_ID_1 = 0,
    TIM_ID_2,
    TIM_ID_3,
    TIM_ID_4,
    TIM_ID_5,
    TIM_ID_6,
    TIM_ID_7,
    TIM_ID_8,
    TIM_ID_9,
    TIM_ID_10,
    TIM_ID_11,
    TIM_ID_12,
    TIM_ID_13,
    TIM_ID_14,
    TIM_ID_NUM
} tim_id_t;

/** @brief Capture/compare channel. */
typedef enum
{
    TIM_CH1 = 0,
    TIM_CH2,
    TIM_CH3,
    TIM_CH4,
    TIM_CH_NUM
} tim_channel_t;

/** @brief Operating mode. */
typedef enum
{
    TIM_MODE_BASE = 0, /*!< timebase / update (optionally interrupt) */
    TIM_MODE_PWM,      /*!< PWM output on one channel */
    TIM_MODE_OC,       /*!< output-compare toggle (all 4 channels) */
    TIM_MODE_IC,       /*!< input capture on the selected channel */
    TIM_MODE_COUNTER,  /*!< external clock counter (slave external1) */
    TIM_MODE_PWMIN,    /*!< PWM input (slave reset, CH1/CH2) */
    TIM_MODE_CPLM,     /*!< complementary PWM + dead time (advanced) */
    TIM_MODE_NPWM      /*!< N-pulse burst (advanced, repetition) */
} tim_mode_t;

/** @brief Output/input active level. */
typedef enum
{
    TIM_POL_HIGH = 0, /*!< active high (rising) */
    TIM_POL_LOW       /*!< active low (falling) */
} tim_polarity_t;

/** @brief Pin pull. */
typedef enum
{
    TIM_PULL_NONE = 0,
    TIM_PULL_UP,
    TIM_PULL_DOWN
} tim_pull_t;

/** @brief Capture channel reported to the capture callback. */
typedef enum
{
    TIM_CAP_CH1 = 0,
    TIM_CAP_CH2,
    TIM_CAP_CH3,
    TIM_CAP_CH4
} tim_cap_ch_t;

/** @brief Capture edge reported to the capture callback. */
typedef enum
{
    TIM_EDGE_RISING = 0,
    TIM_EDGE_FALLING
} tim_edge_t;

/** @brief Register selected for tim_set()/tim_get(). */
typedef enum
{
    TIM_PARAM_CCR = 0, /*!< CCRx (get = captured value in IC mode) */
    TIM_PARAM_COUNT,   /*!< CNT; write clears it */
    TIM_PARAM_FLAG,    /*!< get = pending flag mask; write = clear mask */
    TIM_PARAM_PSC,     /*!< prescaler */
    TIM_PARAM_DTG,     /*!< complementary dead time (CPLM) */
    TIM_PARAM_BURST    /*!< N-pulse burst count (write = trigger) */
} tim_param_t;

/** @brief Pending-flag bits returned by TIM_PARAM_FLAG (own namespace, not HAL). */
#define TIM_PEND_UPDATE (1U << 0U) /*!< update/overflow pending */
#define TIM_PEND_CC     (1U << 1U) /*!< capture/compare pending */

/** @brief Update callback (base/overflow/timeout). */
typedef void (*tim_cb_t)(void);

/** @brief Capture callback; returns the edge to arm next (IC mode only). */
typedef tim_edge_t (*tim_cap_cb_t)(tim_cap_ch_t ch, uint32_t value, tim_edge_t edge);

/** @brief One-shot timer configuration. */
typedef struct
{
    tim_id_t       id;
    tim_mode_t     mode;
    tim_channel_t  channel;      /*!< primary channel */
    tim_polarity_t polarity;     /*!< output/input active level */
    tim_pull_t     pull;         /*!< pin pull (overrides the descriptor) */
    uint8_t        ic_filter;    /*!< input-capture filter (0..15) */
    uint32_t       arr;          /*!< auto-reload (period); 32-bit for TIM2/TIM5 */
    uint16_t       psc;          /*!< prescaler */
    uint8_t        irq_prio;     /*!< NVIC preemption priority */
    uint8_t        irq_sub;      /*!< NVIC subpriority */
    tim_cb_t       update_cb;    /*!< update/overflow/timeout callback */
    tim_cap_cb_t   capture_cb;   /*!< capture callback (IC/PWMIN) */
} tim_cfg_t;

/** @brief TIM6 base timebase, no interrupt, IRQ (1,3). */
#define TIM_CFG_DEFAULT \
    .id = TIM_ID_6, .mode = TIM_MODE_BASE, .channel = TIM_CH1, \
    .polarity = TIM_POL_HIGH, .pull = TIM_PULL_NONE, .ic_filter = 0U, \
    .arr = 0U, .psc = 0U, .irq_prio = 1U, .irq_sub = 3U, \
    .update_cb = 0, .capture_cb = 0

/** @brief  Initialise @p cfg (NULL selects TIM_CFG_DEFAULT) and start it. */
void tim_init(const tim_cfg_t *cfg);

/** @brief  Enable (true) or disable (false) the timer after tim_init(). */
void tim_enable(tim_id_t id, bool on);

/** @brief  Write a register/parameter (TIM_PARAM_x). */
void tim_set(tim_id_t id, tim_channel_t ch, tim_param_t param, uint32_t value);

/** @brief  Read a register/parameter (TIM_PARAM_x). */
uint32_t tim_get(tim_id_t id, tim_channel_t ch, tim_param_t param);

#endif /* BSP_TIM_H */
