/**
 * @file    usmart_str.h
 * @brief   USMART string/number parsing helpers.
 */

#ifndef BSP_USMART_USMART_STR_H
#define BSP_USMART_USMART_STR_H

#include <stdint.h>
#include "usmart/usmart.h"

/** @brief One parsed parameter: number, string literal, or malformed. */
typedef enum
{
    USMART_APARM_NUM = 0,
    USMART_APARM_STR,
    USMART_APARM_ERR = 0xFF
} usmart_aparmtype_t;

uint8_t  usmart_get_parmpos(uint8_t num);
uint8_t  usmart_strcmp(const char *str1, const char *str2);
uint32_t usmart_pow(uint8_t m, uint8_t n);
uint8_t  usmart_str2num(const char *str, uint32_t *res);
uint8_t  usmart_get_cmdname(const char *str, char *cmdname, uint8_t *nlen, uint8_t maxlen);
uint8_t  usmart_get_fname(const char *str, char *fname, uint8_t *pnum, uint8_t *rval);
uint8_t  usmart_get_aparm(const char *str, char *fparm, usmart_aparmtype_t *ptype);
uint8_t  usmart_get_fparam(const char *str, uint8_t *parn);

#endif /* BSP_USMART_USMART_STR_H */
