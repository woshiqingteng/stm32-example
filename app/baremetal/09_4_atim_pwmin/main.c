/**
 * @file    main.c
 * @brief   09_4_atim_pwmin: TIM3_CH4 (PB1) test PWM measured by TIM8 PWM input
 *          (jumper PB1 -> PC6), print frequency.
 *
 * The tim driver only reports raw captures (CH1 rising / CH2 falling) and
 * update timeouts; the measurement state machine and the auto-ranging live here.
 */

#include <stdio.h>

#include "bsp.h"
#include "tim.h"

#define ATIM_PWMIN_TEST_ARR      10U
#define ATIM_PWMIN_TEST_PSC      90U
#define ATIM_PWMIN_TEST_CCR      2U
#define ATIM_PWMIN_TIMER_CLK_MHZ 180U
/* test PWM (TIM3/APB1): 90 MHz/(90*10) = 100 kHz; measured by TIM8 @ 180 MHz */
#define ATIM_PWMIN_US_PER_SECOND 1000000U
#define ATIM_PWMIN_BLINK_COUNT     20U
#define ATIM_PWMIN_LOOP_MS       10U

#define PWMIN_ARR            0xFFFFU
#define PWMIN_PSC_MAX        0xFFFFU
#define PWMIN_PSC_DOUBLE_LIMIT 0x7FFFU

typedef enum
{
    PWMIN_IDLE = 0,   /*!< waiting for the first rising edge */
    PWMIN_ARMED,      /*!< measuring */
    PWMIN_DONE        /*!< measurement ready */
} pwmin_state_t;

static volatile uint8_t  g_report_ready;
static volatile uint16_t g_report_psc;
static volatile uint32_t g_report_hval;
static volatile uint32_t g_report_cval;

static volatile uint16_t g_last_psc;
static volatile uint32_t g_last_hval;
static volatile uint32_t g_last_cval;
static volatile uint8_t  g_have_last;

static volatile pwmin_state_t g_state;
static uint16_t g_psc;   /* applied ranging prescaler */
static uint32_t g_hval;  /* high time (ticks) captured on CH2 */

/* Raw capture (ISR): assemble one CH1(CH2) measurement. */
static tim_edge_t pwmin_capture(tim_cap_ch_t ch, uint32_t value, tim_edge_t edge)
{
    (void)edge;

    if (g_state == PWMIN_IDLE)
    {
        if (ch == TIM_CAP_CH1)
        {
            g_state = PWMIN_ARMED;    /* discard the first edge */
        }
    }
    else if (g_state == PWMIN_ARMED)
    {
        if (ch == TIM_CAP_CH2)
        {
            g_hval = value + 1U;
        }
        else if (ch == TIM_CAP_CH1)
        {
            uint32_t cval = value + 1U;

            if (g_hval < cval)
            {
                g_report_psc   = g_psc;
                g_report_hval  = g_hval;
                g_report_cval  = cval;
                g_report_ready = 1U;
                g_state        = PWMIN_DONE;
            }
        }
    }

    return TIM_EDGE_RISING;   /* ignored in PWM-input mode */
}

/* Update/timeout (ISR): widen the range (auto-ranging moved out of the driver). */
static void pwmin_timeout(void)
{
    if (g_state == PWMIN_DONE)
    {
        return;
    }

    if (g_psc == 0U)
    {
        g_psc = 1U;
    }
    else if (g_psc == PWMIN_PSC_MAX)
    {
        g_psc = 0U;
    }
    else if (g_psc > PWMIN_PSC_DOUBLE_LIMIT)
    {
        g_psc = PWMIN_PSC_MAX;
    }
    else
    {
        g_psc = (uint16_t)(g_psc * 2U);
    }

    tim_set(TIM_ID_8, TIM_CH1, TIM_PARAM_PSC, g_psc);
    tim_set(TIM_ID_8, TIM_CH1, TIM_PARAM_COUNT, 0U);
}

/* Re-arm after a completed measurement. */
static void pwmin_rearm(void)
{
    g_state = PWMIN_IDLE;
    g_psc   = 0U;
    g_hval  = 0U;
    tim_set(TIM_ID_8, TIM_CH1, TIM_PARAM_PSC, 0U);
    tim_set(TIM_ID_8, TIM_CH1, TIM_PARAM_COUNT, 0U);
    tim_set(TIM_ID_8, TIM_CH1, TIM_PARAM_FLAG, TIM_PEND_UPDATE | TIM_PEND_CC);
}

int main(void)
{
    tim_cfg_t pwm_cfg  = { TIM_CFG_DEFAULT };
    tim_cfg_t pin_cfg  = { TIM_CFG_DEFAULT };
    uint32_t  blink = 0;

    bsp_init();
    printf(APP_BANNER "\r\n");

    /* Test PWM: TIM3_CH4 (PB1), 100 kHz. */
    pwm_cfg.id       = TIM_ID_3;
    pwm_cfg.mode     = TIM_MODE_PWM;
    pwm_cfg.channel  = TIM_CH4;
    pwm_cfg.polarity = TIM_POL_LOW;
    pwm_cfg.pull     = TIM_PULL_UP;
    pwm_cfg.arr      = ATIM_PWMIN_TEST_ARR - 1U;
    pwm_cfg.psc      = ATIM_PWMIN_TEST_PSC - 1U;
    tim_init(&pwm_cfg);
    tim_set(TIM_ID_3, TIM_CH4, TIM_PARAM_CCR, ATIM_PWMIN_TEST_CCR);

    /* PWM input: TIM8_CH1 (PC6). */
    pin_cfg.id         = TIM_ID_8;
    pin_cfg.mode       = TIM_MODE_PWMIN;
    pin_cfg.channel    = TIM_CH1;
    pin_cfg.pull       = TIM_PULL_DOWN;
    pin_cfg.arr        = PWMIN_ARR;
    pin_cfg.psc        = 0U;
    pin_cfg.update_cb  = &pwmin_timeout;
    pin_cfg.capture_cb = &pwmin_capture;
    tim_init(&pin_cfg);

    g_state = PWMIN_IDLE;
    g_psc   = 0U;
    g_hval  = 0U;

    for (;;)
    {
        /* Consume every loop so a new measurement is never overwritten. */
        if (g_report_ready)
        {
            g_report_ready = 0U;
            g_last_psc  = g_report_psc;
            g_last_hval = g_report_hval;
            g_last_cval = g_report_cval;
            g_have_last = 1U;
            pwmin_rearm();
        }

        /* Throttled report: print the latest measurement every 200 ms. */
        if ((blink % ATIM_PWMIN_BLINK_COUNT) == 0U)
        {
            if (g_have_last)
            {
                uint16_t psc   = g_last_psc;
                uint32_t hval  = g_last_hval;
                uint32_t cval  = g_last_cval;
                uint64_t scale = (uint64_t)psc + 1U;
                uint64_t htime = ((uint64_t)hval * scale) / ATIM_PWMIN_TIMER_CLK_MHZ;
                uint64_t ctime = ((uint64_t)cval * scale) / ATIM_PWMIN_TIMER_CLK_MHZ;
                uint64_t freq  = (ctime != 0U) ? (ATIM_PWMIN_US_PER_SECOND / ctime) : 0U;

                printf("psc:%u hval:%lu cval:%lu high:%luus cycle:%luus freq:%luHz\r\n",
                       (unsigned)psc, (unsigned long)hval, (unsigned long)cval,
                       (unsigned long)htime, (unsigned long)ctime, (unsigned long)freq);
            }

            led_toggle(LED1);
        }

        blink++;
        delay_ms(ATIM_PWMIN_LOOP_MS);
    }
}
