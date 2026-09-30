/**
 * @file    mbedtls_config.h
 * @brief   mbedTLS 2.16.2 configuration for the bare-metal STM32F4 target.
 *
 * Starts from the upstream default configuration and drops the layers that
 * assume a POSIX host (sockets, wall clock, stdio file access). The library is
 * built and linked so it is available to future TLS apps, but the current
 * example does not call into it.
 */

#ifndef PROJECT_MBEDTLS_CONFIG_H
#define PROJECT_MBEDTLS_CONFIG_H

#include "mbedtls/config.h"

#undef MBEDTLS_NET_C
#undef MBEDTLS_TIMING_C
#undef MBEDTLS_FS_IO
#undef MBEDTLS_HAVE_TIME
#undef MBEDTLS_HAVE_TIME_DATE

/* No POSIX entropy source; the port provides mbedtls_hardware_poll() backed by
 * the STM32 RNG (see port/openedv_stm32f4/lwip/mbedtls_rng.c). */
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT

#endif /* PROJECT_MBEDTLS_CONFIG_H */
