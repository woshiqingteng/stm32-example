/**
 * @file    tpad.c
 * @brief   Capacitive touch key driver (TIM2_CH1 / PA5, polling).
 */

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "tpad.h"
#include "delay.h"

#define TPAD_GPIO_PORT   GPIOA
#define TPAD_GPIO_PIN    GPIO_PIN_5
#define TPAD_GPIO_AF     GPIO_AF1_TIM2

#define TPAD_GATE_VAL    50U
#define TPAD_ARR_MAX_VAL 0xFFFFFFFFUL
#define TPAD_CAL_SAMPLES 10U
#define TPAD_SCAN_SAMPLE 3U
#define TPAD_SCAN_SAMPLE_CONT 6U
#define TPAD_LOCK_COUNT  3U

volatile uint16_t g_tpad_default_val;

static TIM_HandleTypeDef g_tpad_handle;

static void tpad_reset(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin   = TPAD_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLDOWN;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(TPAD_GPIO_PORT, &gpio_init);

    HAL_GPIO_WritePin(TPAD_GPIO_PORT, TPAD_GPIO_PIN, GPIO_PIN_RESET);
    delay_ms(5);

    TIM2->SR  = 0;
    TIM2->CNT = 0;

    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Alternate = TPAD_GPIO_AF;
    HAL_GPIO_Init(TPAD_GPIO_PORT, &gpio_init);
}

static uint32_t tpad_get_val(void)
{
    tpad_reset();

    while (__HAL_TIM_GET_FLAG(&g_tpad_handle, TIM_FLAG_CC1) == RESET)
    {
        if (__HAL_TIM_GET_COUNTER(&g_tpad_handle) > (TPAD_ARR_MAX_VAL - 500U))
        {
            return __HAL_TIM_GET_COUNTER(&g_tpad_handle);
        }
    }

    return __HAL_TIM_GET_COMPARE(&g_tpad_handle, TIM_CHANNEL_1);
}

static uint32_t tpad_get_maxval(uint8_t n)
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

uint8_t tpad_init(uint16_t psc)
{
    uint16_t buf[TPAD_CAL_SAMPLES];
    uint32_t sum = 0;
    uint32_t i;
    uint32_t j;

    tpad_timx_cap_init(TPAD_ARR_MAX_VAL, (uint16_t)(psc - 1U));

    for (i = 0; i < TPAD_CAL_SAMPLES; i++)
    {
        buf[i] = tpad_get_val();
        delay_ms(10);
    }

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

    for (i = 2; i < 8U; i++)
    {
        sum += buf[i];
    }

    g_tpad_default_val = (uint16_t)(sum / 6U);
    printf("g_tpad_default_val:%d\r\n", (int)g_tpad_default_val);

    if ((uint32_t)g_tpad_default_val > (TPAD_ARR_MAX_VAL / 2U))
    {
        return 1U;
    }

    return 0U;
}

uint8_t tpad_scan(bool continuous)
{
    static uint8_t keyen = 0;
    uint8_t res = 0;
    uint8_t sample = TPAD_SCAN_SAMPLE;
    uint32_t rval;

    if (continuous)
    {
        sample = TPAD_SCAN_SAMPLE_CONT;
        keyen = 0;
    }

    rval = tpad_get_maxval(sample);

    if (rval > (uint16_t)(g_tpad_default_val + TPAD_GATE_VAL))
    {
        if (keyen == 0U)
        {
            res = 1U;
        }
        keyen = TPAD_LOCK_COUNT;
    }

    if (keyen != 0U)
    {
        keyen--;
    }

    return res;
}
