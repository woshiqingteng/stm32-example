/**
 * @file    mbedtls_rng.c
 * @brief   mbedTLS hardware entropy source backed by the STM32 RNG.
 *
 * The library is built and linked for future TLS apps; the current example
 * does not consume it, so this hook is provided but not exercised.
 */

#include <stddef.h>

#include "mbedtls/entropy.h"
#include "mbedtls/entropy_poll.h"
#include "rng.h"

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    size_t i;

    (void)data;

    if (output == NULL || olen == NULL)
    {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    if (rng_is_ready() != RNG_READY)
    {
        rng_init();
    }

    for (i = 0; i < len; i++)
    {
        output[i] = (unsigned char)(rng_get_random_num() & 0xFFU);
    }

    *olen = len;
    return 0;
}
