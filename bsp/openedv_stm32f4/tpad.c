/**
 * @file    tpad.c
 * @brief   Capacitive touch key driver (TIM2_CH1 / PA5, polling).
 *
 * The pad is discharged, then released to the capture input and charged through
 * an external resistor; input capture times the charge until the threshold.
 * Touching the pad adds capacitance, so the count grows. The driver only
 * returns raw counts; the touch decision/latch lives in the application.
 * Built on the unified tim driver (polling, no interrupts).
 */

#include <stdio.h>

#include "stm32f4xx_hal.h"
#include "tpad.h"
#include "tim.h"
#include "gpio_hw.h"
#include "delay.h"

#define TPAD_GPIO_PORT   GPIOA
#define TPAD_GPIO_PIN    GPIO_PIN_5
#define TPAD_GPIO_AF     GPIO_AF1_TIM2

#define TPAD_ARR_MAX_VAL      0xFFFFFFFFUL
#define TPAD_DISCHARGE_MS     5U
#define TPAD_TIMEOUT_MARGIN   500U
#define TPAD_SAMPLE_PERIOD_MS 10U

#define TPAD_CAL_SAMPLES 10U
#define TPAD_CAL_TRIM_FIRST 2U
#define TPAD_CAL_TRIM_LAST  8U
#define TPAD_CAL_TRIM_COUNT (TPAD_CAL_TRIM_LAST - TPAD_CAL_TRIM_FIRST)

volatile uint16_t g_tpad_default_val;

/* Discharge the pad (drive it low), then release it to the capture input. */
static void tpad_reset(void)
{
    gpio_hw_t out = { TPAD_GPIO_PORT, RCC_AHB1ENR_GPIOAEN, TPAD_GPIO_PIN,
                      GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_HIGH, 0U };
    gpio_hw_t af  = { TPAD_GPIO_PORT, RCC_AHB1ENR_GPIOAEN, TPAD_GPIO_PIN,
                      GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, TPAD_GPIO_AF };

    gpio_hw_setup(&out);
    HAL_GPIO_WritePin(TPAD_GPIO_PORT, TPAD_GPIO_PIN, GPIO_PIN_RESET);
    delay_ms(TPAD_DISCHARGE_MS);

    tim_set(TIM_ID_2, TIM_CH1, TIM_PARAM_FLAG, TIM_PEND_UPDATE | TIM_PEND_CC);
    tim_set(TIM_ID_2, TIM_CH1, TIM_PARAM_COUNT, 0U);

    gpio_hw_setup(&af);
}

/* One charge-time measurement: captured count, or CNT when it times out. */
static uint32_t tpad_get_val(void)
{
    tpad_reset();

    while ((tim_get(TIM_ID_2, TIM_CH1, TIM_PARAM_FLAG) & TIM_PEND_CC) == 0U)
    {
        if (tim_get(TIM_ID_2, TIM_CH1, TIM_PARAM_COUNT) > (TPAD_ARR_MAX_VAL - TPAD_TIMEOUT_MARGIN))
        {
            return tim_get(TIM_ID_2, TIM_CH1, TIM_PARAM_COUNT);
        }
    }

    return tim_get(TIM_ID_2, TIM_CH1, TIM_PARAM_CCR);
}

/* Max of n charge-time measurements (a finger adds capacitance -> larger). */
uint32_t tpad_get_maxval(uint8_t n)
{
    uint32_t maxval = 0;

    while (n-- != 0U)
    {
        uint32_t v = tpad_get_val();
        if (v > maxval)
        {
            maxval = v;
        }
    }

    return maxval;
}

/* TIM2 (APB1): charge time = count * psc / 90 MHz (psc is the counter divider). */
static void tpad_timx_cap_init(uint32_t arr, uint16_t psc)
{
    tim_cfg_t cfg = { TIM_CFG_DEFAULT };

    cfg.id       = TIM_ID_2;
    cfg.mode     = TIM_MODE_IC;
    cfg.channel  = TIM_CH1;
    cfg.polarity = TIM_POL_HIGH;
    cfg.pull     = TIM_PULL_NONE;
    cfg.arr      = arr;
    cfg.psc      = psc;
    tim_init(&cfg);
}

/* Calibrate the no-touch baseline. psc is the counter divider (>= 1). */
tpad_status_t tpad_init(uint16_t psc)
{
    uint16_t buf[TPAD_CAL_SAMPLES];
    uint32_t sum = 0;
    uint32_t i;
    uint32_t j;

    if (psc == 0U)
    {
        return TPAD_ERROR;
    }

    /* The timer register holds psc-1, so the effective divider is psc. */
    tpad_timx_cap_init(TPAD_ARR_MAX_VAL, (uint16_t)(psc - 1U));

    /* Sample the untouched pad. */
    for (i = 0; i < TPAD_CAL_SAMPLES; i++)
    {
        buf[i] = tpad_get_val();
        delay_ms(TPAD_SAMPLE_PERIOD_MS);
    }

    /* Sort, then average the middle samples (drop the extremes). */
    for (i = 0; i < TPAD_CAL_SAMPLES - 1U; i++)
    {
        for (j = i + 1U; j < TPAD_CAL_SAMPLES; j++)
        {
            if (buf[i] > buf[j])
            {
                uint16_t tmp = buf[i];
                buf[i] = buf[j];
                buf[j] = tmp;
            }
        }
    }

    for (i = TPAD_CAL_TRIM_FIRST; i < TPAD_CAL_TRIM_LAST; i++)
    {
        sum += buf[i];
    }

    g_tpad_default_val = (uint16_t)(sum / TPAD_CAL_TRIM_COUNT);
    printf("g_tpad_default_val:%d\r\n", (int)g_tpad_default_val);

    /* A baseline above half the 16-bit range means the pad is stuck. */
    if (g_tpad_default_val > ((uint16_t)TPAD_ARR_MAX_VAL / 2U))
    {
        return TPAD_ERROR;
    }

    return TPAD_OK;
}

/* Calibrated no-touch baseline (raw count). */
uint32_t tpad_baseline(void)
{
    return g_tpad_default_val;
}
