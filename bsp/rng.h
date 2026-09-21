/**
 * @file    rng.h
 * @brief   Hardware random number generator (RNG) interface.
 */

#ifndef BSP_RNG_H
#define BSP_RNG_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief  RNG readiness state. */
typedef enum
{
    RNG_NOT_READY = 0,
    RNG_READY     = 1
} rng_status_t;

/** @brief  Initialise the RNG and wait (bounded) until it is ready. */
void rng_init(void);

rng_status_t rng_is_ready(void);

uint32_t rng_get_random_num(void);

/** @brief  Read a random number within [min, max] (both inclusive). */
int rng_get_random_range(int min, int max);

#endif /* BSP_RNG_H */
