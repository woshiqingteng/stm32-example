/**
 * @file    ir.c
 * @brief   NEC infrared ir receiver. TIM1_CH1 (PA8) input capture with 1 us
 *          resolution, driven by the unified tim driver (raw edges + timeout);
 *          the NEC decode state machine lives here.
 */

#include <stdbool.h>

#include "ir.h"
#include "tim.h"

#define IR_PRESCALER        (180U - 1U) /* 1 tick = 1 us at 180 MHz */
#define IR_PERIOD_TICK           10000U
#define IR_IC_FILTER             0x03U

#define IR_REPEAT_MAX_COUNT         14U

/** @brief  NEC decoder state. */
typedef enum
{
    IR_STATE_IDLE = 0,   /*!< waiting for a leader */
    IR_STATE_FRAME = 1   /*!< leader seen, decoding the 32 bits */
} ir_state_t;

#define IR_BIT0_MIN_US          300U
#define IR_BIT0_MAX_US          800U
#define IR_BIT1_MIN_US         1400U
#define IR_BIT1_MAX_US         1800U
#define IR_REPEAT_MIN_US       2000U
#define IR_REPEAT_MAX_VAL_US   3000U
#define IR_LEAD_MIN_US         4200U
#define IR_LEAD_MAX_US         4700U

static ir_state_t  g_ir_state;
static bool        g_ir_in_high;      /* a high-level pulse is in progress */
static bool        g_ir_key_pending;  /* a decoded key waits in ir_scan() */
static uint32_t    g_ir_data;
static uint8_t     g_ir_timeout;      /* update ticks since the last capture */
static uint8_t     g_ir_cnt;          /* repeat-frame counter */

/* Update ISR: frame timeout / repeat handling. */
static void ir_update(void)
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

/* Capture ISR: rising restarts timing, falling carries the bit value. */
static tim_edge_t ir_capture(tim_cap_ch_t ch, uint32_t value, tim_edge_t edge)
{
    uint16_t dval = (uint16_t)value;

    (void)ch;

    if (edge == TIM_EDGE_RISING)
    {
        g_ir_in_high = true;
        return TIM_EDGE_FALLING;
    }

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
    return TIM_EDGE_RISING;
}

void ir_init(void)
{
    tim_cfg_t cfg = { TIM_CFG_DEFAULT };

    g_ir_state       = IR_STATE_IDLE;
    g_ir_in_high     = false;
    g_ir_key_pending = false;
    g_ir_data        = 0U;
    g_ir_timeout     = 0U;
    g_ir_cnt         = 0U;

    cfg.id         = TIM_ID_1;
    cfg.mode       = TIM_MODE_IC;
    cfg.channel    = TIM_CH1;
    cfg.polarity   = TIM_POL_HIGH;
    cfg.pull       = TIM_PULL_UP;
    cfg.ic_filter  = IR_IC_FILTER;
    cfg.arr        = IR_PERIOD_TICK;
    cfg.psc        = IR_PRESCALER;
    cfg.update_cb  = &ir_update;
    cfg.capture_cb = &ir_capture;
    tim_init(&cfg);
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
