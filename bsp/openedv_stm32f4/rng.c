/**
 * @file    rng.c
 * @brief   Hardware random number generator driver. MSP content is inlined.
 */

#include "stm32f4xx_hal.h"
#include "delay.h"
#include "rng.h"

#define RNG_READY_RETRY_COUNT  10000U
#define RNG_POLL_US            10U

static RNG_HandleTypeDef g_rng_handle;

rng_status_t rng_init(void)
{
    uint16_t retry = 0U;

    g_rng_handle.Instance = RNG;

    /* ---- MSP begin: RNG peripheral clock ---- */
    __HAL_RCC_RNG_CLK_ENABLE();
    /* ---- MSP end ---- */

    if (HAL_RNG_Init(&g_rng_handle) != HAL_OK)
    {
        return RNG_ERROR;
    }

    /* Discard any stale error flags latched before init. */
    __HAL_RNG_CLEAR_IT(&g_rng_handle, RNG_IT_CEI | RNG_IT_SEI);

    while ((__HAL_RNG_GET_FLAG(&g_rng_handle, RNG_FLAG_DRDY) == RESET) &&
           (retry < RNG_READY_RETRY_COUNT))
    {
        retry++;
        delay_us(RNG_POLL_US);
    }

    return (__HAL_RNG_GET_FLAG(&g_rng_handle, RNG_FLAG_DRDY) != RESET) ? RNG_OK : RNG_ERROR;
}

rng_status_t rng_get(uint32_t *value)
{
    if (value == NULL)
    {
        return RNG_ERROR;
    }

    return (HAL_RNG_GenerateRandomNumber(&g_rng_handle, value) == HAL_OK) ? RNG_OK : RNG_ERROR;
}

rng_status_t rng_get_range(int min, int max, int *value)
{
    uint32_t raw;
    uint32_t span;
    uint32_t limit;

    /* Unsigned subtraction avoids signed overflow for extreme ranges. */
    span = (uint32_t)max - (uint32_t)min + 1U;

    if ((value == NULL) || (max < min) || (span == 0U))
    {
        return RNG_ERROR;
    }

    /* Reject the top values so the modulo has no bias. */
    limit = (UINT32_MAX / span) * span;

    do
    {
        if (rng_get(&raw) != RNG_OK)
        {
            return RNG_ERROR;
        }
    } while (raw >= limit);

    *value = (int)(raw % span) + min;

    return RNG_OK;
}
