/**
 * @file    rng.h
 * @brief   Hardware random number generator (RNG) interface.
 */

#ifndef BSP_RNG_H
#define BSP_RNG_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief  Initialise the RNG and wait (bounded) until it is ready. */
void rng_init(void);

/** @brief  Non-zero once the RNG produced at least one valid value. */
uint8_t rng_is_ready(void);

/** @brief  Read a 32-bit random number. */
uint32_t rng_get_random_num(void);

/**
 * @brief  Read a random number within [min, max].
 * @param  min Lower bound (inclusive).
 * @param  max Upper bound (inclusive).
 * @return A value v with min <= v <= max.
 */
int rng_get_random_range(int min, int max);

#endif /* BSP_RNG_H */
