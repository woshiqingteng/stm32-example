/**
 * @file    tpad.c
 * @brief   Capacitive touch key driver (TIM2_CH1 / PA5, polling).
 *
 * The pad is discharged, then released to the capture input and charged through
 * an external resistor; input capture times the charge until the threshold.
 * Touching the pad adds capacitance, so the count grows. The driver only
 * returns raw counts; the touch decision/latch lives in the application.
 */

#include <stdio.h>

#include "stm32f4xx_hal.h"
#include "tpad.h"
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

static TIM_HandleTypeDef g_tpad_handle;

/* Discharge the pad (drive it low), then release it to the capture input. */
static void tpad_reset(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin   = TPAD_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLDOWN;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(TPAD_GPIO_PORT, &gpio_init);

    HAL_GPIO_WritePin(TPAD_GPIO_PORT, TPAD_GPIO_PIN, GPIO_PIN_RESET);
    delay_ms(TPAD_DISCHARGE_MS);

    __HAL_TIM_CLEAR_FLAG(&g_tpad_handle, TIM_FLAG_UPDATE | TIM_FLAG_CC1);
    __HAL_TIM_SET_COUNTER(&g_tpad_handle, 0U);

    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Alternate = TPAD_GPIO_AF;
    HAL_GPIO_Init(TPAD_GPIO_PORT, &gpio_init);
}

/* One charge-time measurement: captured count, or CNT when it times out. */
static uint32_t tpad_get_val(void)
{
    tpad_reset();

    while (__HAL_TIM_GET_FLAG(&g_tpad_handle, TIM_FLAG_CC1) == RESET)
    {
        if (__HAL_TIM_GET_COUNTER(&g_tpad_handle) > (TPAD_ARR_MAX_VAL - TPAD_TIMEOUT_MARGIN))
        {
            return __HAL_TIM_GET_COUNTER(&g_tpad_handle);
        }
    }

    return __HAL_TIM_GET_COMPARE(&g_tpad_handle, TIM_CHANNEL_1);
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
    GPIO_InitTypeDef gpio_init = {0};
    TIM_IC_InitTypeDef ic      = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    gpio_init.Pin       = TPAD_GPIO_PIN;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = TPAD_GPIO_AF;
    HAL_GPIO_Init(TPAD_GPIO_PORT, &gpio_init);

    g_tpad_handle.Instance               = TIM2;
    g_tpad_handle.Init.Prescaler         = psc;
    g_tpad_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_tpad_handle.Init.Period            = arr;
    g_tpad_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_IC_Init(&g_tpad_handle);

    ic.ICPolarity  = TIM_ICPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter    = 0;
    HAL_TIM_IC_ConfigChannel(&g_tpad_handle, &ic, TIM_CHANNEL_1);
    HAL_TIM_IC_Start(&g_tpad_handle, TIM_CHANNEL_1);
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
