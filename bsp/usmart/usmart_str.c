/**
 * @file    usmart_str.c
 * @brief   USMART string and number parsing helpers.
 *
 * The parser understands function signatures of the form
 * "<return> name(<type> arg, ...)" so the function table only needs a textual
 * signature: the name is used for dispatch and the argument count is derived
 * from it. Numeric arguments accept decimal, negative and 0X-prefixed hex.
 */

#include <string.h>
#include "usmart/usmart.h"
#include "usmart/usmart_str.h"

/** @brief  Return the first non-space byte following str[0]. */
static uint8_t usmart_search_nextc(const char *str)
{
    str++;
    while (*str == ' ')
    {
        str++;
    }
    return (uint8_t)*str;
}

uint8_t usmart_strcmp(const char *str1, const char *str2)
{
    while (1)
    {
        if (*str1 != *str2)
        {
            return 1U;
        }
        if (*str1 == '\0')
        {
            break;
        }
        str1++;
        str2++;
    }
    return 0U;
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

uint32_t usmart_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1U;

    while (n-- != 0U)
    {
        result *= m;
    }
    return result;
}

uint8_t usmart_str2num(const char *str, uint32_t *res)
{
    uint32_t    t;
    int32_t     tnum;
    uint8_t     bnum = 0U;   /* number of data digits */
    const char *p;
    uint8_t     hexdec = 10U; /* 10 or 16 */
    uint8_t     flag = 0U;   /* 0: none, 1: '+', 2: '-' */

    p = str;
    *res = 0U;

    /* Validate characters and detect the base. */
    while (1)
    {
        if (((*p <= '9') && (*p >= '0')) ||
            (((*str == '-') || (*str == '+')) && (bnum == 0U)) ||
            ((*p <= 'F') && (*p >= 'A')) ||
            ((*p == 'X') && (bnum == 1U)))
        {
            if (*p >= 'A')
            {
                hexdec = 16U;
            }

            if (*str == '-')
            {
                flag = 2U;
                str++;
            }
            else if (*str == '+')
            {
                flag = 1U;
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
            return 1U;
        }
        p++;
    }

    p = str;

    if (hexdec == 16U)
    {
        if (bnum < 3U)
        {
            return 2U; /* too short to be 0Xnn */
        }
        if ((*p == '0') && (*(p + 1) == 'X'))
        {
            p += 2;
            bnum -= 2U;
        }
        else
        {
            return 3U; /* hex must start with 0X */
        }
    }
    else if (bnum == 0U)
    {
        return 4U; /* no digits */
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
            t = (uint32_t)(*p - 'A' + 10);
        }
        *res += t * usmart_pow(hexdec, bnum);
        p++;
        if (*p == '\0')
        {
            break;
        }
    }

    if (flag == 2U)
    {
        tnum = -(int32_t)(*res);
        *res = (uint32_t)tnum;
    }
    return 0U;
}

uint8_t usmart_get_cmdname(const char *str, char *cmdname, uint8_t *nlen, uint8_t maxlen)
{
    *nlen = 0U;

    while ((*str != ' ') && (*str != '\0'))
    {
        *cmdname++ = *str++;
        (*nlen)++;
        if (*nlen >= maxlen)
        {
            return 1U;
        }
    }
    *cmdname = '\0';
    return 0U;
}

uint8_t usmart_get_fname(const char *str, char *fname, uint8_t *pnum, uint8_t *rval)
{
    uint8_t     res = USMART_RES_OK;
    uint8_t     fover = 0U;          /* parenthesis depth */
    uint8_t     parmnum = 0U;
    uint8_t     parm_chars = 1U;     /* non-space chars seen for current param */
    uint8_t     parm_idx = 0U;       /* comma counter */
    uint8_t     first_parm_len = 0U;
    char        first_parm[6];       /* "void" + NUL */
    char        rtname[6];           /* return type word (max 5 chars) */
    uint8_t     rtlen = 0U;
    uint8_t     in_string = 0U;
    uint8_t     offset = 0U;
    uint8_t     pos = 0U;
    const char *p;

    /* Return value exists unless the first token is exactly "void". */
    p = str;
    while ((*p != '\0') && (*p != ' ') && (rtlen < 5U))
    {
        rtname[rtlen++] = *p++;
    }
    rtname[rtlen] = '\0';
    *rval = (usmart_strcmp(rtname, "void") == 0U) ? 0U : 1U;

    /* Find where the function name starts (skip return type, spaces, '*'). */
    p = str;
    while ((*p != '(') && (*p != '\0'))
    {
        p++;
        pos++;
        if ((*p == ' ') || (*p == '*'))
        {
            uint8_t nchar = usmart_search_nextc(p);
            if ((nchar != '(') && (nchar != '*'))
            {
                offset = pos;
            }
        }
    }
    p = (offset != 0U) ? (str + offset + 1U) : str;

    while (1)
    {
        if (*p == '\0')
        {
            res = USMART_RES_FUNCERR;
            break;
        }
        else if ((*p == '(') && (in_string == 0U))
        {
            fover++;
        }
        else if ((*p == ')') && (in_string == 0U))
        {
            if (fover != 0U)
            {
                fover--;
            }
            else
            {
                res = USMART_RES_FUNCERR;
            }
            if (fover == 0U)
            {
                break;
            }
        }
        else if (*p == '"')
        {
            in_string = (uint8_t)(!in_string);
        }

        if (fover == 0U)
        {
            if (*p != ' ')
            {
                *fname++ = *p;
            }
        }
        else
        {
            if (*p == ',')
            {
                parm_chars = 1U;
                parm_idx++;
            }
            else if ((*p != ' ') && (*p != '('))
            {
                if ((parm_idx == 0U) && (first_parm_len < 5U))
                {
                    first_parm[first_parm_len++] = *p;
                }
                parm_chars++;
            }

            if ((fover == 1U) && (parm_chars == 2U))
            {
                parm_chars++;
                parmnum++;
            }
        }
        p++;
    }

    if (parmnum == 1U)
    {
        first_parm[first_parm_len] = '\0';
        if (usmart_strcmp(first_parm, "void") == 0U)
        {
            parmnum = 0U;
        }
    }

    *pnum = parmnum;
    *fname = '\0';
    return res;
}

uint8_t usmart_get_aparm(const char *str, char *fparm, usmart_aparmtype_t *ptype)
{
    uint8_t            i = 0U;
    uint8_t            enout = 0U;
    usmart_aparmtype_t type = USMART_APARM_NUM;
    uint8_t            in_string = 0U;

    while (1)
    {
        if ((*str == ',') && (in_string == 0U))
        {
            enout = 1U;
        }
        if (((*str == ')') || (*str == '\0')) && (in_string == 0U))
        {
            break;
        }

        if (type == USMART_APARM_NUM)
        {
            if (((*str >= '0') && (*str <= '9')) || (*str == '-') || (*str == '+') ||
                ((*str >= 'a') && (*str <= 'f')) || ((*str >= 'A') && (*str <= 'F')) ||
                (*str == 'X') || (*str == 'x'))
            {
                if (enout)
                {
                    break;
                }
                *fparm = (*str >= 'a') ? (char)(*str - 0x20) : *str;
                fparm++;
            }
            else if (*str == '"')
            {
                if (enout)
                {
                    break;
                }
                type = USMART_APARM_STR;
                in_string = 1U;
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
                in_string = 0U;
            }
            if (enout)
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
                *fparm = *str;
                fparm++;
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

uint8_t usmart_get_fparam(const char *str, uint8_t *parn)
{
    uint8_t            i;
    usmart_aparmtype_t type;
    uint32_t           res;
    uint8_t            n = 0U;
    uint8_t            len;
    char               tstr[USMART_PARM_LEN + 1U];

    for (i = 0U; i < USMART_MAX_PARM; i++)
    {
        usmart_dev.plentbl[i] = 0U;
    }

    while (*str != '(')
    {
        str++;
        if (*str == '\0')
        {
            return USMART_RES_FUNCERR;
        }
    }
    str++;

    while (1)
    {
        i = usmart_get_aparm(str, tstr, &type);
        str += i;

        switch (type)
        {
            case USMART_APARM_NUM:
                if (tstr[0] != '\0')
                {
                    i = usmart_str2num(tstr, &res);
                    if (i != 0U)
                    {
                        return USMART_RES_PARMERR;
                    }
                    memcpy(usmart_dev.parm + usmart_get_parmpos(n), &res, sizeof(res));
                    usmart_dev.parmtype[n] = USMART_PARM_NUM;
                    usmart_dev.plentbl[n] = 4U;
                    n++;
                    if (n > USMART_MAX_PARM)
                    {
                        return USMART_RES_PARMOVER;
                    }
                }
                break;

            case USMART_APARM_STR:
                len = (uint8_t)(usmart_strlen(tstr) + 1U);
                usmart_strcopy(tstr, (char *)&usmart_dev.parm[usmart_get_parmpos(n)]);
                usmart_dev.parmtype[n] = USMART_PARM_STR;
                usmart_dev.plentbl[n] = len;
                n++;
                if (n > USMART_MAX_PARM)
                {
                    return USMART_RES_PARMOVER;
                }
                break;

            case USMART_APARM_ERR:
            default:
                return USMART_RES_PARMERR;
        }

        if ((*str == ')') || (*str == '\0'))
        {
            break;
        }
    }

    *parn = n;
    return USMART_RES_OK;
}
