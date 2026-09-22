/**
 * @file    usmart.c
 * @brief   USMART serial debug console: system commands and typed dispatch.
 *
 * usmart_scan() is polled from the main loop. For each received line it first
 * tries to match a user function from usmart_nametab[]; if that fails it tries
 * the built-in system commands. Dispatch goes through the per-entry exact-typed
 * trampoline stored in struct _m_usmart_nametab.call, so no call is ever made
 * through an incompatible function-pointer type.
 */

#include <string.h>
#include "usmart/usmart.h"
#include "usmart/usmart_str.h"
#include "usmart/usmart_port.h"

/** @brief Indices of the built-in system commands. */
enum
{
    USMART_CMD_HELP = 0,
    USMART_CMD_HELP_LONG,
    USMART_CMD_LIST,
    USMART_CMD_ID,
    USMART_CMD_HEX,
    USMART_CMD_DEC,
    USMART_CMD_RUNTIME
};

static const char *const g_usmart_sys_cmd_tab[] =
{
    "?",
    "help",
    "list",
    "id",
    "hex",
    "dec",
    "runtime",
};

static usmart_status_t usmart_sys_cmd_exe(char *str)
{
    uint8_t                i;
    usmart_cmdname_status_t cs;
    char                   sfname[MAX_FNAME_LEN];
    usmart_aparmtype_t     atype;
    usmart_num_status_t    nstatus;
    uint32_t               value;

    cs = usmart_get_cmdname(str, sfname, &i, MAX_FNAME_LEN);

    if (cs != USMART_CMDNAME_OK)
    {
        return USMART_FUNCERR;
    }

    str += i;

    for (i = 0U; i < (uint8_t)(sizeof(g_usmart_sys_cmd_tab) / sizeof(g_usmart_sys_cmd_tab[0])); i++)
    {
        if (usmart_strcmp(sfname, g_usmart_sys_cmd_tab[i]))
        {
            break;
        }
    }

    switch (i)
    {
        case USMART_CMD_HELP:
        case USMART_CMD_HELP_LONG:
            USMART_PRINTF("\r\n");
#if USMART_USE_HELP
            USMART_PRINTF("------------------------USMART------------------------\r\n");
            USMART_PRINTF("Call any function in the table from the serial terminal.\r\n");
            USMART_PRINTF("Arguments may be decimal, signed or 0X-prefixed hex and\r\n");
            USMART_PRINTF("quoted strings; up to 10 arguments are supported and the\r\n");
            USMART_PRINTF("return value is printed. Seven system commands (lower case):\r\n");
            USMART_PRINTF("?:       show this help\r\n");
            USMART_PRINTF("help:    show this help\r\n");
            USMART_PRINTF("list:    list the available functions\r\n");
            USMART_PRINTF("id:      list the function entry addresses\r\n");
            USMART_PRINTF("hex:     hex argument display, or 'hex 100' to convert\r\n");
            USMART_PRINTF("dec:     decimal argument display, or 'dec 0X64' to convert\r\n");
            USMART_PRINTF("runtime:1 = report run-time, 0 = stop reporting\r\n");
            USMART_PRINTF("Enter a call in the exact source form, e.g. delay_ms(500).\r\n");
#else
            USMART_PRINTF("help disabled\r\n");
#endif
            break;

        case USMART_CMD_LIST:
            USMART_PRINTF("\r\n-------------------------function list-------------------------\r\n");

            for (i = 0U; i < usmart_dev.fnum; i++)
            {
                USMART_PRINTF("%s\r\n", usmart_dev.funs[i].name);
            }

            USMART_PRINTF("\r\n");
            break;

        case USMART_CMD_ID:
            USMART_PRINTF("\r\n-------------------------function id-------------------------\r\n");

            for (i = 0U; i < usmart_dev.fnum; i++)
            {
                uint8_t pnum;
                bool    rval;
                char    idname[MAX_FNAME_LEN];

                (void)usmart_get_fname(usmart_dev.funs[i].name, idname, &pnum, &rval);
                USMART_PRINTF("%s id: 0X%08lX\r\n", idname,
                              (unsigned long)(uintptr_t)usmart_dev.funs[i].func);
            }

            USMART_PRINTF("\r\n");
            break;

        case USMART_CMD_HEX:
        case USMART_CMD_DEC:
        {
            usmart_sptype_t sptype = (i == USMART_CMD_HEX) ? SP_TYPE_HEX : SP_TYPE_DEC;

            USMART_PRINTF("\r\n");
            (void)usmart_get_aparm(str, sfname, MAX_FNAME_LEN, &atype);

            if (atype != USMART_APARM_NUM)
            {
                return USMART_PARMERR;
            }

            nstatus = usmart_str2num(sfname, &value);

            if (nstatus == USMART_NUM_OK)
            {
                if (sptype == SP_TYPE_HEX)
                {
                    USMART_PRINTF("HEX:0X%lX\r\n", (unsigned long)value);
                }
                else
                {
                    USMART_PRINTF("DEC:%lu\r\n", (unsigned long)value);
                }
            }
            else if (nstatus != USMART_NUM_NO_DIGITS)
            {
                return USMART_PARMERR;
            }
            else if (sptype == SP_TYPE_HEX)
            {
                USMART_PRINTF("16-based argument display!\r\n");
                usmart_dev.sptype = SP_TYPE_HEX;
            }
            else
            {
                USMART_PRINTF("10-based argument display!\r\n");
                usmart_dev.sptype = SP_TYPE_DEC;
            }

            USMART_PRINTF("\r\n");
            break;
        }

        case USMART_CMD_RUNTIME:
            USMART_PRINTF("\r\n");
            (void)usmart_get_aparm(str, sfname, MAX_FNAME_LEN, &atype);

            if (atype != USMART_APARM_NUM)
            {
                return USMART_PARMERR;
            }

            nstatus = usmart_str2num(sfname, &value);

            if (nstatus != USMART_NUM_OK)
            {
                return USMART_PARMERR;
            }

#if USMART_ENTIMX_SCAN == 0
            USMART_PRINTF("Run-time needs USMART_ENTIMX_SCAN=1\r\n");
#else
            usmart_dev.runtimeflag = (value != 0U) ? USMART_RUNTIME_ON : USMART_RUNTIME_OFF;

            if (usmart_dev.runtimeflag == USMART_RUNTIME_ON)
            {
                USMART_PRINTF("Run Time Calculation ON\r\n");
            }
            else
            {
                USMART_PRINTF("Run Time Calculation OFF\r\n");
            }
#endif

            USMART_PRINTF("\r\n");
            break;

        default:
            return USMART_FUNCERR;
    }

    return USMART_OK;
}

void usmart_init(uint16_t tclk)
{
    usmart_port_init(tclk);
    usmart_dev.sptype = SP_TYPE_HEX;
}

usmart_status_t usmart_cmd_rec(char *str)
{
    usmart_status_t sta;
    uint8_t         i;
    bool            rval;
    uint8_t         rpnum;
    uint8_t         spnum;
    char            rfname[MAX_FNAME_LEN];
    char            sfname[MAX_FNAME_LEN];

    sta = usmart_get_fname(str, rfname, &rpnum, &rval);

    if (sta != USMART_OK)
    {
        return sta;
    }

    for (i = 0U; i < usmart_dev.fnum; i++)
    {
        sta = usmart_get_fname(usmart_dev.funs[i].name, sfname, &spnum, &rval);

        if (sta != USMART_OK)
        {
            return sta;
        }

        if (usmart_strcmp(sfname, rfname))
        {
            if (spnum > rpnum)
            {
                return USMART_PARMERR;
            }

            usmart_dev.id = i;
            break;
        }
    }

    if (i == usmart_dev.fnum)
    {
        return USMART_NOFUNCFIND;
    }

    sta = usmart_get_fparam(str, &i);

    if (sta != USMART_OK)
    {
        return sta;
    }

    usmart_dev.pnum = i;
    return USMART_OK;
}

void usmart_exe(void)
{
    uint8_t  id;
    uint8_t  i;
    uint8_t  pnum;
    bool     rval;
    uint32_t res = 0U;
    uint32_t temp[MAX_PARM];
    char     sfname[MAX_FNAME_LEN];

    id = usmart_dev.id;

    if (id >= usmart_dev.fnum)
    {
        return;
    }

    (void)usmart_get_fname(usmart_dev.funs[id].name, sfname, &pnum, &rval);
    USMART_PRINTF("\r\n%s(", sfname);

    for (i = 0U; i < pnum; i++)
    {
        if ((usmart_dev.parmtype & (uint16_t)(1U << i)) != 0U)
        {
            USMART_PRINTF("%c", '"');
            USMART_PRINTF("%s", (char *)(usmart_dev.parm + usmart_get_parmpos(i)));
            USMART_PRINTF("%c", '"');
            temp[i] = (uint32_t)(uintptr_t)(usmart_dev.parm + usmart_get_parmpos(i));
        }
        else
        {
            memcpy(&temp[i], usmart_dev.parm + usmart_get_parmpos(i), sizeof(uint32_t));

            if (usmart_dev.sptype == SP_TYPE_DEC)
            {
                USMART_PRINTF("%ld", (long)temp[i]);
            }
            else
            {
                USMART_PRINTF("0X%lX", (unsigned long)temp[i]);
            }
        }

        if (i != (uint8_t)(pnum - 1U))
        {
            USMART_PRINTF("%c", ',');
        }
    }

    USMART_PRINTF(")");

#if USMART_ENTIMX_SCAN == 1
    usmart_timx_reset_time();
#endif

    res = usmart_dev.funs[id].call(usmart_dev.funs[id].func, temp);

#if USMART_ENTIMX_SCAN == 1
    (void)usmart_timx_get_time();
#endif

    if (rval)
    {
        if (usmart_dev.sptype == SP_TYPE_DEC)
        {
            USMART_PRINTF("=%lu;\r\n", (unsigned long)res);
        }
        else
        {
            USMART_PRINTF("=0X%lX;\r\n", (unsigned long)res);
        }
    }
    else
    {
        USMART_PRINTF(";\r\n");
    }

    if (usmart_dev.runtimeflag == USMART_RUNTIME_ON)
    {
        USMART_PRINTF("Function Run Time:%lu us\r\n", (unsigned long)usmart_dev.runtime);
    }
}

void usmart_scan(void)
{
    char           *pbuf;
    usmart_status_t sta;

    pbuf = usmart_get_input_string();

    if (pbuf == 0)
    {
        return;
    }

    sta = usmart_dev.cmd_rec(pbuf);

    if (sta == USMART_OK)
    {
        usmart_dev.exe();
        return;
    }

    {
        usmart_status_t sys = usmart_sys_cmd_exe(pbuf);

        if (sys != USMART_FUNCERR)
        {
            sta = sys;
        }
    }

    switch (sta)
    {
        case USMART_FUNCERR:
            USMART_PRINTF("Function error!\r\n");
            break;

        case USMART_PARMERR:
            USMART_PRINTF("Parameter error!\r\n");
            break;

        case USMART_PARMOVER:
            USMART_PRINTF("Too many parameters!\r\n");
            break;

        case USMART_NOFUNCFIND:
            USMART_PRINTF("Function not found!\r\n");
            break;

        default:
            break;
    }
}

uint32_t read_addr(uint32_t addr)
{
    uint32_t value;

    memcpy(&value, (const void *)(uintptr_t)addr, sizeof(value));
    return value;
}

void write_addr(uint32_t addr, uint32_t val)
{
    memcpy((void *)(uintptr_t)addr, &val, sizeof(val));
}
