/**
 * @file    rng.h
 * @brief   Hardware random number generator (RNG) interface.
 */

#ifndef BSP_RNG_H
#define BSP_RNG_H

#include <stdint.h>

/** @brief  RNG status. */
typedef enum
{
    RNG_OK = 0,
    RNG_ERROR = 1
} rng_status_t;

/** @brief  Initialise the RNG and wait (bounded) until it is ready. */
rng_status_t rng_init(void);

/** @brief  Read a 32-bit random number. */
rng_status_t rng_get(uint32_t *value);

/** @brief  Read a random number within [min, max] (both inclusive). */
rng_status_t rng_get_range(int min, int max, int *value);

#endif /* BSP_RNG_H */
