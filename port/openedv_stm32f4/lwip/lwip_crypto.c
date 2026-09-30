/**
 * @file    lwip_crypto.c
 * @brief   mbedTLS backed HMAC-SHA1 / Base64 helpers.
 */

#include "mbedtls/md.h"
#include "mbedtls/base64.h"

#include "lwip_crypto.h"

int lwip_hmac_sha1(const uint8_t *key, size_t key_len,
                   const uint8_t *data, size_t data_len,
                   uint8_t out[LWIP_HMAC_SHA1_LEN])
{
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);

    if (info == NULL)
    {
        return -1;
    }

    return mbedtls_md_hmac(info, key, key_len, data, data_len, out);
}

int lwip_base64_encode(uint8_t *dst, size_t dst_len, size_t *olen,
                       const uint8_t *src, size_t src_len)
{
    return mbedtls_base64_encode(dst, dst_len, olen, src, src_len);
}

int lwip_base64_decode(uint8_t *dst, size_t dst_len, size_t *olen,
                       const uint8_t *src, size_t src_len)
{
    return mbedtls_base64_decode(dst, dst_len, olen, src, src_len);
}
