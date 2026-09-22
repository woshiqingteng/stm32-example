/**
 * @file    usmart_str.h
 * @brief   USMART string and number parsing helpers.
 */

#ifndef BSP_USMART_USMART_STR_H
#define BSP_USMART_USMART_STR_H

#include <stdint.h>
#include "usmart/usmart.h"

/** @brief Kind of a parsed argument. */
typedef enum
{
    USMART_APARM_NUM = 0,   /*!< numeric argument */
    USMART_APARM_STR = 1,   /*!< quoted string argument */
    USMART_APARM_ERR = 0xFF /*!< malformed argument */
} usmart_aparmtype_t;

/** @brief Outcome of usmart_str2num(). */
typedef enum
{
    USMART_NUM_OK = 0,        /*!< converted successfully */
    USMART_NUM_BADCHAR,       /*!< illegal character in the token */
    USMART_NUM_HEX_TOO_SHORT, /*!< too short to be "0Xnn" */
    USMART_NUM_HEX_NO_PREFIX, /*!< hex digits without a "0X" prefix */
    USMART_NUM_NO_DIGITS      /*!< empty token */
} usmart_num_status_t;

/** @brief Outcome of usmart_get_cmdname(). */
typedef enum
{
    USMART_CMDNAME_OK = 0,    /*!< command name copied */
    USMART_CMDNAME_OVERFLOW   /*!< name longer than the caller's buffer */
} usmart_cmdname_status_t;

uint8_t  usmart_get_parmpos(uint8_t num);
bool     usmart_strcmp(const char *str1, const char *str2);
usmart_num_status_t usmart_str2num(const char *str, uint32_t *res);
usmart_cmdname_status_t usmart_get_cmdname(const char *str, char *cmdname, uint8_t *nlen, uint8_t maxlen);
usmart_status_t usmart_get_fname(const char *str, char *fname, uint8_t *pnum, bool *rval);

/**
 * @brief  Parse one argument at the start of @p str.
 * @param  fparm      destination buffer (NUL terminated on return)
 * @param  fparm_size capacity of @p fparm including the NUL terminator
 * @param  ptype      receives the argument kind (USMART_APARM_ERR on failure)
 * @return number of input characters consumed
 */
uint8_t  usmart_get_aparm(const char *str, char *fparm, uint8_t fparm_size, usmart_aparmtype_t *ptype);
usmart_status_t usmart_get_fparam(const char *str, uint8_t *parn);

#endif /* BSP_USMART_USMART_STR_H */
