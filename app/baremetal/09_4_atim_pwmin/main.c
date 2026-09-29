/**
 * @file    main.c
 * @brief   09_4_atim_pwmin: TIM3_CH4 (PB1) test PWM measured by TIM8 PWM input
 *          (jumper PB1 -> PC6), print frequency.
 */

#include <stdio.h>

#include "bsp.h"
#include "atim.h"
#include "gtim.h"

#define ATIM_PWMIN_TEST_ARR      10U
#define ATIM_PWMIN_TEST_PSC      90U
#define ATIM_PWMIN_TEST_CCR      2U
#define ATIM_PWMIN_TIMER_CLK_MHZ 180U
/* test PWM (TIM3/APB1): 90 MHz/(90*10) = 100 kHz; measured by TIM8 @ 180 MHz */
#define ATIM_PWMIN_US_PER_SECOND 1000000U
#define ATIM_PWMIN_BLINK_COUNT     20U
#define ATIM_PWMIN_LOOP_MS       10U

static volatile uint8_t  g_report_ready;
static volatile uint16_t g_report_psc;
static volatile uint32_t g_report_hval;
static volatile uint32_t g_report_cval;

static volatile uint16_t g_last_psc;
static volatile uint32_t g_last_hval;
static volatile uint32_t g_last_cval;
static volatile uint8_t  g_have_last;

/* Completion callback (ISR): only stash values and raise a flag. */
static void pwmin_done(uint16_t psc, uint32_t hval, uint32_t cval)
{
    g_report_psc   = psc;
    g_report_hval  = hval;
    g_report_cval  = cval;
    g_report_ready = 1U;
}

int main(void)
{
    uint32_t blink = 0;

    bsp_init();
    printf(APP_BANNER "\r\n");

    gtim_timx_pwm_chy_init(ATIM_PWMIN_TEST_ARR - 1U, ATIM_PWMIN_TEST_PSC - 1U);
    gtim_timx_pwm_chy_set(ATIM_PWMIN_TEST_CCR);

    atim_timx_pwmin_chy_init();
    atim_timx_pwmin_chy_register(&pwmin_done);

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
            atim_timx_pwmin_chy_restart();
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
