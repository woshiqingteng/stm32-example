/**
 * @file    usmart_str.c
 * @brief   USMART string and number parsing helpers.
 *
 * A function signature has the form "<return> name(<type> arg, ...)". The name
 * is used for dispatch and the argument count is derived from the text. Numeric
 * arguments accept decimal, signed and 0X-prefixed hexadecimal values; string
 * arguments are quoted and may use a backslash escape.
 */

#include <string.h>
#include "usmart.h"
#include "usmart_str.h"
#include "sys.h"

#define USMART_BASE_DEC        10U  /*!< decimal radix */
#define USMART_BASE_HEX        16U  /*!< hexadecimal radix */
#define USMART_HEX_PREFIX_LEN  2U   /*!< length of the "0X" prefix */
#define USMART_TYPE_MAX_CHARS  5U   /*!< longest return/type word kept while scanning */
#define USMART_TYPE_BUF_LEN    6U   /*!< USMART_TYPE_MAX_CHARS plus NUL */
#define USMART_UPPER_OFFSET    0x20U/*!< lowercase to uppercase delta */
#define USMART_FNAME_FLAG      0x80U/*!< "a return type has started" marker bit */
#define USMART_FNAME_MASK      0x7FU/*!< character count mask for the marker byte */

/** @brief Sign prefix found while scanning a numeric token. */
typedef enum
{
    USMART_SIGN_NONE = 0,
    USMART_SIGN_PLUS,
    USMART_SIGN_MINUS
} usmart_sign_t;

static uint8_t usmart_search_nextc(const char *str)
{
    str++;

    while (*str == ' ')
    {
        str++;
    }

    return (uint8_t)*str;
}

static void usmart_strcopy(const char *src, char *dst)
{
    while (1)
    {
        *dst = *src;

        if (*src == '\0')
        {
            break;
        }

        src++;
        dst++;
    }
}

static uint8_t usmart_strlen(const char *str)
{
    uint8_t len = 0U;

    while (*str != '\0')
    {
        len++;
        str++;
    }

    return len;
}

bool usmart_strcmp(const char *str1, const char *str2)
{
    while (1)
    {
        if (*str1 != *str2)
        {
            return false;
        }

        if (*str1 == '\0')
        {
            break;
        }

        str1++;
        str2++;
    }

    return true;
}

usmart_num_status_t usmart_str2num(const char *str, uint32_t *res)
{
    uint32_t      t;
    int32_t       tnum;
    uint8_t       bnum = 0U;   /* number of data digits */
    const char   *p;
    uint8_t       hexdec = USMART_BASE_DEC;
    usmart_sign_t sign = USMART_SIGN_NONE;

    p    = str;
    *res = 0U;

    /* Validate characters and detect the base. */
    while (1)
    {
        if (((*p <= '9') && (*p >= '0')) ||
            (((*str == '-') || (*str == '+')) && (bnum == 0U)) ||
            ((*p <= 'F') && (*p >= 'A')) ||
            ((*p == 'X') && (bnum == (USMART_HEX_PREFIX_LEN - 1U))))
        {
            if (*p >= 'A')
            {
                hexdec = USMART_BASE_HEX;
            }

            if (*str == '-')
            {
                sign = USMART_SIGN_MINUS;
                str++;
            }
            else if (*str == '+')
            {
                sign = USMART_SIGN_PLUS;
                str++;
            }
            else
            {
                bnum++;
            }
        }
        else if (*p == '\0')
        {
            break;
        }
        else
        {
            return USMART_NUM_BADCHAR;
        }

        p++;
    }

    p = str;

    if (hexdec == USMART_BASE_HEX)
    {
        if (bnum < (USMART_HEX_PREFIX_LEN + 1U))
        {
            return USMART_NUM_HEX_TOO_SHORT;
        }

        if ((*p == '0') && (*(p + 1) == 'X'))
        {
            p    += USMART_HEX_PREFIX_LEN;
            bnum -= USMART_HEX_PREFIX_LEN;
        }
        else
        {
            return USMART_NUM_HEX_NO_PREFIX;
        }
    }
    else if (bnum == 0U)
    {
        return USMART_NUM_NO_DIGITS;
    }

    while (1)
    {
        if (bnum != 0U)
        {
            bnum--;
        }

        if ((*p <= '9') && (*p >= '0'))
        {
            t = (uint32_t)(*p - '0');
        }
        else
        {
            t = (uint32_t)(*p - 'A' + USMART_BASE_DEC);
        }

        *res += t * bsp_pow(hexdec, bnum);
        p++;

        if (*p == '\0')
        {
            break;
        }
    }

    if (sign == USMART_SIGN_MINUS)
    {
        tnum = -(int32_t)(*res);
        *res = (uint32_t)tnum;
    }

    return USMART_NUM_OK;
}

usmart_cmdname_status_t usmart_get_cmdname(const char *str, char *cmdname, uint8_t *nlen, uint8_t maxlen)
{
    *nlen = 0U;

    while ((*str != ' ') && (*str != '\0'))
    {
        *cmdname++ = *str++;
        (*nlen)++;

        if (*nlen >= maxlen)
        {
            return USMART_CMDNAME_OVERFLOW;
        }
    }

    *cmdname = '\0';
    return USMART_CMDNAME_OK;
}

usmart_status_t usmart_get_fname(const char *str, char *fname, uint8_t *pnum, bool *rval)
{
    usmart_status_t res = USMART_OK;
    uint8_t         paren_depth = 0U; /* parenthesis depth */
    const char     *strtemp;
    uint8_t         offset = 0U;
    uint8_t         parmnum = 0U;
    uint8_t         parm_chars = 1U;  /* characters seen for the current parameter */
    char            fpname[USMART_TYPE_BUF_LEN];
    uint8_t         fplcnt = 0U;      /* first parameter length counter */
    uint8_t         pcnt = 0U;        /* return type character counter with marker bit */
    uint8_t         pos = 0U;
    uint8_t         next_char = 0U;
    bool            in_string = false; /* true while inside a quoted literal */

    /* A return value exists unless the first token is exactly "void". */
    strtemp = str;

    while (*strtemp != '\0')
    {
        if ((*strtemp != ' ') && ((pcnt & USMART_FNAME_MASK) < USMART_TYPE_MAX_CHARS))
        {
            if (pcnt == 0U)
            {
                pcnt |= USMART_FNAME_FLAG;
            }

            if (((pcnt & USMART_FNAME_MASK) == (USMART_TYPE_MAX_CHARS - 1U)) && (*strtemp != '*'))
            {
                break;
            }

            fpname[pcnt & USMART_FNAME_MASK] = *strtemp;
            pcnt++;
        }
        else if (pcnt == (USMART_FNAME_FLAG | USMART_TYPE_MAX_CHARS))
        {
            break;
        }

        strtemp++;
    }

    if (pcnt != 0U)
    {
        fpname[pcnt & USMART_FNAME_MASK] = '\0';
        *rval = !usmart_strcmp(fpname, "void");
        pcnt = 0U;
    }

    /* Skip the return type, spaces and '*' to find the function name. */
    strtemp = str;

    while ((*strtemp != '(') && (*strtemp != '\0'))
    {
        strtemp++;
        pos++;

        if ((*strtemp == ' ') || (*strtemp == '*'))
        {
            next_char = usmart_search_nextc(strtemp);

            if ((next_char != '(') && (next_char != '*'))
            {
                offset = pos;
            }
        }
    }

    strtemp = str;

    if (offset != 0U)
    {
        strtemp += offset + 1U;
    }

    in_string = false;

    while (1)
    {
        if (*strtemp == '\0')
        {
            res = USMART_FUNCERR;
            break;
        }
        else if ((*strtemp == '(') && !in_string)
        {
            paren_depth++;
        }
        else if ((*strtemp == ')') && !in_string)
        {
            if (paren_depth != 0U)
            {
                paren_depth--;
            }
            else
            {
                res = USMART_FUNCERR;
            }

            if (paren_depth == 0U)
            {
                break;
            }
        }
        else if (*strtemp == '"')
        {
            in_string = !in_string;
        }

        if (paren_depth == 0U)
        {
            if (*strtemp != ' ')
            {
                *fname++ = *strtemp;
            }
        }
        else
        {
            if (*strtemp == ',')
            {
                parm_chars = 1U;
                pcnt++;
            }
            else if ((*strtemp != ' ') && (*strtemp != '('))
            {
                if ((pcnt == 0U) && (fplcnt < USMART_TYPE_MAX_CHARS))
                {
                    fpname[fplcnt] = *strtemp;
                    fplcnt++;
                }

                parm_chars++;
            }

            if ((paren_depth == 1U) && (parm_chars == 2U))
            {
                parm_chars++;
                parmnum++;
            }
        }

        strtemp++;
    }

    if (parmnum == 1U)
    {
        fpname[fplcnt] = '\0';

        if (usmart_strcmp(fpname, "void"))
        {
            parmnum = 0U;
        }
    }

    *pnum  = parmnum;
    *fname = '\0';
    return res;
}

/*
 * Bounded copy policy: fparm_size is the capacity of fparm including its NUL
 * terminator, so at most fparm_size-1 characters are emitted. The string branch
 * (whose data eventually lands in usmart_dev.parm[], sized PARM_LEN at offset
 * usmart_get_parmpos(n)) is bounded here; if a literal does not fit the
 * argument is reported as USMART_APARM_ERR instead of overflowing.
 */
uint8_t usmart_get_aparm(const char *str, char *fparm, uint8_t fparm_size, usmart_aparmtype_t *ptype)
{
    uint8_t            i = 0U;
    uint8_t            out = 0U;
    bool               end_of_arg = false;
    bool               in_string = false;
    usmart_aparmtype_t type = USMART_APARM_NUM;

    if (fparm_size == 0U)
    {
        *ptype = USMART_APARM_ERR;
        return 0U;
    }

    while (1)
    {
        if ((*str == ',') && !in_string)
        {
            end_of_arg = true;
        }

        if (((*str == ')') || (*str == '\0')) && !in_string)
        {
            break;
        }

        if (type == USMART_APARM_NUM)
        {
            if (((*str >= '0') && (*str <= '9')) || (*str == '-') || (*str == '+') ||
                ((*str >= 'a') && (*str <= 'f')) || ((*str >= 'A') && (*str <= 'F')) ||
                (*str == 'X') || (*str == 'x'))
            {
                if (end_of_arg)
                {
                    break;
                }

                if (out >= (uint8_t)(fparm_size - 1U))
                {
                    type = USMART_APARM_ERR;
                    break;
                }

                if (*str >= 'a')
                {
                    *fparm = (char)(*str - USMART_UPPER_OFFSET);
                }
                else
                {
                    *fparm = *str;
                }

                fparm++;
                out++;
            }
            else if (*str == '"')
            {
                if (end_of_arg)
                {
                    break;
                }

                type      = USMART_APARM_STR;
                in_string = true;
            }
            else if ((*str != ' ') && (*str != ','))
            {
                type = USMART_APARM_ERR;
                break;
            }
        }
        else
        {
            if (*str == '"')
            {
                in_string = false;
            }

            if (end_of_arg)
            {
                break;
            }

            if (in_string)
            {
                if (*str == '\\')
                {
                    str++;
                    i++;
                }

                if (out >= (uint8_t)(fparm_size - 1U))
                {
                    type = USMART_APARM_ERR;
                    break;
                }

                *fparm = *str;
                fparm++;
                out++;
            }
        }

        i++;
        str++;
    }

    *fparm = '\0';
    *ptype = type;
    return i;
}

uint8_t usmart_get_parmpos(uint8_t num)
{
    uint8_t temp = 0U;
    uint8_t i;

    for (i = 0U; i < num; i++)
    {
        temp += usmart_dev.plentbl[i];
    }

    return temp;
}

usmart_status_t usmart_get_fparam(const char *str, uint8_t *parn)
{
    uint8_t             i;
    uint8_t             len;
    usmart_aparmtype_t  type;
    usmart_num_status_t nstatus;
    uint32_t            res;
    uint8_t             n = 0U;
    char                tstr[PARM_LEN + 1U];

    for (i = 0U; i < MAX_PARM; i++)
    {
        usmart_dev.plentbl[i] = 0U;
    }

    while (*str != '(')
    {
        if (*str == '\0')
        {
            return USMART_FUNCERR;
        }

        str++;
    }

    str++;

    while (1)
    {
        i = usmart_get_aparm(str, tstr, (uint8_t)sizeof(tstr), &type);
        str += i;

        switch (type)
        {
            case USMART_APARM_NUM:
                if (tstr[0] != '\0')
                {
                    nstatus = usmart_str2num(tstr, &res);

                    if (nstatus != USMART_NUM_OK)
                    {
                        return USMART_PARMERR;
                    }

                    if (n >= MAX_PARM)
                    {
                        return USMART_PARMOVER;
                    }

                    memcpy(usmart_dev.parm + usmart_get_parmpos(n), &res, sizeof(res));
                    usmart_dev.parmtype &= (uint16_t)~(1U << n);
                    usmart_dev.plentbl[n]  = (uint8_t)sizeof(res);
                    n++;
                }
                break;

            case USMART_APARM_STR:
            {
                uint8_t pos;

                if (n >= MAX_PARM)
                {
                    return USMART_PARMOVER;
                }

                len = (uint8_t)(usmart_strlen(tstr) + 1U);
                pos = usmart_get_parmpos(n);

                /* usmart_dev.parm[] holds PARM_LEN bytes in total and this
                 * argument starts at usmart_get_parmpos(n): reject a string
                 * that would not fit rather than overflowing the packed area. */
                if (((uint32_t)pos + (uint32_t)len) > PARM_LEN)
                {
                    return USMART_PARMERR;
                }

                usmart_strcopy(tstr, (char *)&usmart_dev.parm[pos]);
                usmart_dev.parmtype |= (uint16_t)(1U << n);
                usmart_dev.plentbl[n]  = len;
                n++;
                break;
            }

            case USMART_APARM_ERR:
            default:
                return USMART_PARMERR;
        }

        if ((*str == ')') || (*str == '\0'))
        {
            break;
        }
    }

    *parn = n;
    return USMART_OK;
}
