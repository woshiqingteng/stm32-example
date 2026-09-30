/**
 * @file    mbedtls_config.h
 * @brief   Minimal mbedTLS 2.16.2 configuration for the bare-metal STM32F4.
 *
 * Only the primitives used by the lwIP examples (MD/HMAC + SHA-1 + Base64) are
 * enabled; TLS/X509/PK/ciphers and the POSIX layers are compiled out. This is a
 * full replacement for mbedtls/config.h, so it ends with check_config.h.
 */

#ifndef PROJECT_MBEDTLS_CONFIG_H
#define PROJECT_MBEDTLS_CONFIG_H

#define MBEDTLS_PLATFORM_C
#define MBEDTLS_MD_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_BASE64_C

#include "mbedtls/check_config.h"

#endif /* PROJECT_MBEDTLS_CONFIG_H */
