/**
 * @file    rng.c
 * @brief   Hardware random number generator driver. MSP content is inlined.
 */

#include "stm32f4xx_hal.h"
#include "delay.h"
#include "rng.h"

#define RNG_READY_RETRY        10000U
#define RNG_POLL_US            10U
#define RNG_RANGE_SPAN_OFFSET  1U

static RNG_HandleTypeDef g_rng_handle;
static rng_status_t      g_rng_ready = RNG_NOT_READY;

void rng_init(void)
{
    uint16_t retry = 0U;

    g_rng_handle.Instance = RNG;

    /* ---- MSP begin: RNG peripheral clock ---- */
    __HAL_RCC_RNG_CLK_ENABLE();
    /* ---- MSP end ---- */

    (void)HAL_RNG_Init(&g_rng_handle);

    while ((__HAL_RNG_GET_FLAG(&g_rng_handle, RNG_FLAG_DRDY) == RESET) &&
           (retry < RNG_READY_RETRY))
    {
        retry++;
        delay_us(RNG_POLL_US);
    }

    g_rng_ready = (retry < RNG_READY_RETRY) ? RNG_READY : RNG_NOT_READY;
}

rng_status_t rng_is_ready(void)
{
    return g_rng_ready;
}

uint32_t rng_get_random_num(void)
{
    uint32_t value = 0U;

    (void)HAL_RNG_GenerateRandomNumber(&g_rng_handle, &value);

    return value;
}

int rng_get_random_range(int min, int max)
{
    uint32_t value = 0U;
    uint32_t span;

    (void)HAL_RNG_GenerateRandomNumber(&g_rng_handle, &value);

    if (max < min)
    {
        return min;
    }

    span = (uint32_t)(max - min) + 1U;

    if (span == 0U)
    {
        return min;
    }

    return (int)(value % span) + min;
}
