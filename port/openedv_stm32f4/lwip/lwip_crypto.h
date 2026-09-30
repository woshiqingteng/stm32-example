/**
 * @file    lwip_crypto.h
 * @brief   Crypto helpers for the lwIP cloud examples (mbedTLS backed).
 *
 * Thin wrappers around mbedTLS MD/HMAC-SHA1 and Base64, used by the Aliyun and
 * OneNET MQTT examples to derive their login tokens.
 */

#ifndef PORT_LWIP_CRYPTO_H
#define PORT_LWIP_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

#define LWIP_HMAC_SHA1_LEN  20U

/** @brief  HMAC-SHA1. @return 0 on success, <0 on error. */
int lwip_hmac_sha1(const uint8_t *key, size_t key_len,
                   const uint8_t *data, size_t data_len,
                   uint8_t out[LWIP_HMAC_SHA1_LEN]);

/** @brief  Base64 encode/decode (see mbedtls_base64_*). */
int lwip_base64_encode(uint8_t *dst, size_t dst_len, size_t *olen,
                       const uint8_t *src, size_t src_len);

int lwip_base64_decode(uint8_t *dst, size_t dst_len, size_t *olen,
                       const uint8_t *src, size_t src_len);

#endif /* PORT_LWIP_CRYPTO_H */
