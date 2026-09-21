/**
 * @file    usmart.c
 * @brief   USMART serial debug console: command parser and function dispatch.
 *
 * usmart_init() wires the console to the USART driver receive hook and starts
 * TIM4 as a 1 us time base. usmart_scan() is polled from the main loop and
 * executes a command once a full line has been received.
 */

#include <stdio.h>
#include <string.h>
#include "usmart/usmart.h"
#include "usmart/usmart_str.h"
#include "usart.h"
#include "delay.h"
#include "led.h"

#define USMART_TIMX              TIM4
#define USMART_TIMX_IRQn         TIM4_IRQn
#define USMART_TIMX_CLK_ENABLE() do { __HAL_RCC_TIM4_CLK_ENABLE(); } while (0)
#define USMART_TIMX_TICK_HZ      1000000U
#define USMART_TIMX_PERIOD       0xFFFFU

static TIM_HandleTypeDef g_usmart_timx;
static uint8_t           g_usmart_rx_buf[USMART_RX_BUF_LEN];
static uint16_t          g_usmart_rx_len;
static usmart_rx_state_t g_usmart_rx_state;

/** @brief Entry point that executes one system command and returns a result code. */
typedef usmart_result_t (*usmart_sys_cmd_handler_t)(const char *str);

/** @brief A system command name paired with its handler. */
typedef struct
{
    const char              *name;
    usmart_sys_cmd_handler_t handler;
} usmart_sys_cmd_ent_t;

/** @brief Thunk that calls a function table entry with a fixed argument count. */
typedef uint32_t (*usmart_func_call_t)(void *func, const uint32_t *args);

static void     usmart_rx_byte_hook(uint8_t byte);
static char    *usmart_get_input_string(void);
static void     usmart_timx_init(void);
static void     usmart_timx_reset_time(void);
static uint32_t usmart_timx_get_time(void);
static usmart_result_t usmart_sys_cmd_exe(const char *str);
static usmart_result_t usmart_cmd_rec(const char *str);
static void     usmart_exe(void);

static usmart_result_t usmart_sys_cmd_help(const char *str);
static usmart_result_t usmart_sys_cmd_list(const char *str);
static usmart_result_t usmart_sys_cmd_id(const char *str);
static usmart_result_t usmart_sys_cmd_hex(const char *str);
static usmart_result_t usmart_sys_cmd_dec(const char *str);
static usmart_result_t usmart_sys_cmd_runtime(const char *str);

/* Function table: only the functions useful without a display are exposed. */
static const usmart_nametab_t g_usmart_nametab[] =
{
    { (void *)read_addr,  "uint32_t read_addr(uint32_t addr)" },
    { (void *)write_addr, "void write_addr(uint32_t addr, uint32_t val)" },
    { (void *)delay_ms,   "void delay_ms(uint16_t nms)" },
    { (void *)delay_us,   "void delay_us(uint32_t nus)" },
    { (void *)led_on,     "void led_on(led_id_t id)" },
    { (void *)led_off,    "void led_off(led_id_t id)" },
};

usmart_dev_t usmart_dev =
{
    .funs        = g_usmart_nametab,
    .fnum        = (uint8_t)(sizeof(g_usmart_nametab) / sizeof(g_usmart_nametab[0])),
    .pnum        = 0U,
    .id          = 0U,
    .sptype      = USMART_SP_HEX,
    .parmtype    = {0},
    .plentbl     = {0},
    .parm        = {0},
    .runtimeflag = false,
    .runtime     = 0U,
};

uint32_t read_addr(uint32_t addr)
{
    return *(uint32_t *)addr;
}

void write_addr(uint32_t addr, uint32_t val)
{
    *(uint32_t *)addr = val;
}

static void usmart_rx_byte_hook(uint8_t byte)
{
    if ((byte == '\r') || (byte == '\n'))
    {
        if (g_usmart_rx_len != 0U)
        {
            g_usmart_rx_buf[g_usmart_rx_len] = '\0';
            g_usmart_rx_state = USMART_RX_READY;
        }
    }
    else if (g_usmart_rx_state != USMART_RX_READY)
    {
        if (g_usmart_rx_len < (USMART_RX_BUF_LEN - 1U))
        {
            g_usmart_rx_buf[g_usmart_rx_len++] = byte;
            g_usmart_rx_state = USMART_RX_RECEIVING;
        }
        else
        {
            g_usmart_rx_len   = 0U;
            g_usmart_rx_state = USMART_RX_IDLE;
        }
    }
}

static char *usmart_get_input_string(void)
{
    char *pbuf = 0;

    if (g_usmart_rx_state == USMART_RX_READY)
    {
        g_usmart_rx_buf[g_usmart_rx_len] = '\0';
        pbuf              = (char *)g_usmart_rx_buf;
        g_usmart_rx_len   = 0U;
        g_usmart_rx_state = USMART_RX_IDLE;
    }
    return pbuf;
}

static uint32_t usmart_timx_clk_hz(void)
{
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();

    return ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1) ? pclk1 : (pclk1 * 2U);
}

static void usmart_timx_init(void)
{
    uint32_t tim_clk;

    /* ---- MSP begin: TIM4 clock + base ---- */
    USMART_TIMX_CLK_ENABLE();
    tim_clk = usmart_timx_clk_hz();
    /* ---- MSP end ---- */

    g_usmart_timx.Instance           = USMART_TIMX;
    g_usmart_timx.Init.Prescaler     = (tim_clk / USMART_TIMX_TICK_HZ) - 1U;
    g_usmart_timx.Init.CounterMode   = TIM_COUNTERMODE_UP;
    g_usmart_timx.Init.Period        = USMART_TIMX_PERIOD;
    g_usmart_timx.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    (void)HAL_TIM_Base_Init(&g_usmart_timx);
    (void)HAL_TIM_Base_Start(&g_usmart_timx);
}

static void usmart_timx_reset_time(void)
{
    __HAL_TIM_CLEAR_FLAG(&g_usmart_timx, TIM_FLAG_UPDATE);
    __HAL_TIM_SET_COUNTER(&g_usmart_timx, 0U);
    usmart_dev.runtime = 0U;
}

static uint32_t usmart_timx_get_time(void)
{
    if (__HAL_TIM_GET_FLAG(&g_usmart_timx, TIM_FLAG_UPDATE) == SET)
    {
        usmart_dev.runtime += (USMART_TIMX_PERIOD + 1U);
    }
    usmart_dev.runtime += __HAL_TIM_GET_COUNTER(&g_usmart_timx);
    return usmart_dev.runtime;
}

void usmart_init(void)
{
    usmart_timx_init();
    usart_register_rx_byte_hook(usmart_rx_byte_hook);
    usmart_dev.sptype = USMART_SP_HEX;
}

static usmart_result_t usmart_sys_cmd_help(const char *str)
{
    (void)str;
    printf("\r\nUSMART console. System commands (lower case):\r\n");
    printf("  ?|help  : this help\r\n");
    printf("  list    : list usable functions\r\n");
    printf("  id      : list function entry addresses\r\n");
    printf("  hex     : hex parameter display, or 'hex 100' to convert\r\n");
    printf("  dec     : dec parameter display, or 'dec 0X64' to convert\r\n");
    printf("  runtime : 1=enable / 0=disable run-time report\r\n");
    printf("Example call: delay_ms(500)\r\n");
    return USMART_RES_OK;
}

static usmart_result_t usmart_sys_cmd_list(const char *str)
{
    uint8_t i;

    (void)str;
    printf("\r\n--- function list ---\r\n");
    for (i = 0U; i < usmart_dev.fnum; i++)
    {
        printf("%s\r\n", usmart_dev.funs[i].name);
    }
    printf("\r\n");
    return USMART_RES_OK;
}

static usmart_result_t usmart_sys_cmd_id(const char *str)
{
    uint8_t i;

    (void)str;
    printf("\r\n--- function id ---\r\n");
    for (i = 0U; i < usmart_dev.fnum; i++)
    {
        uint8_t pnum;
        uint8_t rval;
        char    sfname[USMART_MAX_FNAME_LEN];

        (void)usmart_get_fname(usmart_dev.funs[i].name, sfname, &pnum, &rval);
        printf("%s id: 0X%08lX\r\n", sfname,
               (unsigned long)(uintptr_t)usmart_dev.funs[i].func);
    }
    printf("\r\n");
    return USMART_RES_OK;
}

static usmart_result_t usmart_sys_cmd_hexdec(const char *str, usmart_sptype_t sptype)
{
    usmart_aparmtype_t  atype;
    usmart_num_status_t nstatus;
    uint32_t            res;
    char                sfname[USMART_MAX_FNAME_LEN];

    printf("\r\n");
    (void)usmart_get_aparm(str, sfname, &atype);

    if (atype != USMART_APARM_NUM)
    {
        return USMART_RES_PARMERR;
    }

    nstatus = usmart_str2num(sfname, &res);
    if (nstatus == USMART_NUM_OK)
    {
        if (sptype == USMART_SP_HEX)
        {
            printf("HEX:0X%lX\r\n", (unsigned long)res);
        }
        else
        {
            printf("DEC:%lu\r\n", (unsigned long)res);
        }
    }
    else if (nstatus != USMART_NUM_NO_DIGITS)
    {
        return USMART_RES_PARMERR;
    }
    else if (sptype == USMART_SP_HEX)
    {
        printf("Hex parameter display\r\n");
        usmart_dev.sptype = USMART_SP_HEX;
    }
    else
    {
        printf("Decimal parameter display\r\n");
        usmart_dev.sptype = USMART_SP_DEC;
    }
    printf("\r\n");
    return USMART_RES_OK;
}

static usmart_result_t usmart_sys_cmd_hex(const char *str)
{
    return usmart_sys_cmd_hexdec(str, USMART_SP_HEX);
}

static usmart_result_t usmart_sys_cmd_dec(const char *str)
{
    return usmart_sys_cmd_hexdec(str, USMART_SP_DEC);
}

static usmart_result_t usmart_sys_cmd_runtime(const char *str)
{
    usmart_aparmtype_t  atype;
    usmart_num_status_t nstatus;
    uint32_t            res;
    char                sfname[USMART_MAX_FNAME_LEN];

    printf("\r\n");
    (void)usmart_get_aparm(str, sfname, &atype);

    if (atype != USMART_APARM_NUM)
    {
        return USMART_RES_PARMERR;
    }

    nstatus = usmart_str2num(sfname, &res);
    if (nstatus != USMART_NUM_OK)
    {
        return USMART_RES_PARMERR;
    }

    usmart_dev.runtimeflag = (res != 0U);
    printf("Run time report %s\r\n", usmart_dev.runtimeflag ? "ON" : "OFF");
    printf("\r\n");
    return USMART_RES_OK;
}

static const usmart_sys_cmd_ent_t g_usmart_sys_cmd[] =
{
    { "?",       usmart_sys_cmd_help },
    { "help",    usmart_sys_cmd_help },
    { "list",    usmart_sys_cmd_list },
    { "id",      usmart_sys_cmd_id },
    { "hex",     usmart_sys_cmd_hex },
    { "dec",     usmart_sys_cmd_dec },
    { "runtime", usmart_sys_cmd_runtime },
};

static usmart_result_t usmart_sys_cmd_exe(const char *str)
{
    uint8_t i;
    uint8_t name_len;
    char    sfname[USMART_MAX_FNAME_LEN];

    if (usmart_get_cmdname(str, sfname, &name_len, USMART_MAX_FNAME_LEN) != USMART_CMDNAME_OK)
    {
        return USMART_RES_FUNCERR;
    }
    str += name_len;

    for (i = 0U; i < (uint8_t)(sizeof(g_usmart_sys_cmd) / sizeof(g_usmart_sys_cmd[0])); i++)
    {
        if (usmart_strcmp(sfname, g_usmart_sys_cmd[i].name) == 0U)
        {
            return g_usmart_sys_cmd[i].handler(str);
        }
    }

    return USMART_RES_FUNCERR;
}

static usmart_result_t usmart_cmd_rec(const char *str)
{
    usmart_result_t sta;
    uint8_t         i;
    uint8_t         rval;
    uint8_t         rpnum;
    uint8_t         spnum;
    char            rfname[USMART_MAX_FNAME_LEN];
    char            sfname[USMART_MAX_FNAME_LEN];

    sta = usmart_get_fname(str, rfname, &rpnum, &rval);
    if (sta != USMART_RES_OK)
    {
        return sta;
    }

    for (i = 0U; i < usmart_dev.fnum; i++)
    {
        sta = usmart_get_fname(usmart_dev.funs[i].name, sfname, &spnum, &rval);
        if (sta != USMART_RES_OK)
        {
            return sta;
        }

        if (usmart_strcmp(sfname, rfname) == 0U)
        {
            if (spnum > rpnum)
            {
                return USMART_RES_PARMERR;
            }
            usmart_dev.id = i;
            break;
        }
    }

    if (i == usmart_dev.fnum)
    {
        return USMART_RES_NOFUNCFIND;
    }

    sta = usmart_get_fparam(str, &i);
    if (sta != USMART_RES_OK)
    {
        return sta;
    }
    usmart_dev.pnum = i;
    return USMART_RES_OK;
}

/* One thunk per supported argument count; all share the same signature so the
 * call site can select one by parameter count without positional magic. */
static uint32_t usmart_call0(void *func, const uint32_t *args)
{
    (void)args;
    return (*(uint32_t (*)(void))func)();
}

static uint32_t usmart_call1(void *func, const uint32_t *args)
{
    return (*(uint32_t (*)(uint32_t))func)(args[0]);
}

static uint32_t usmart_call2(void *func, const uint32_t *args)
{
    return (*(uint32_t (*)(uint32_t, uint32_t))func)(args[0], args[1]);
}

static uint32_t usmart_call3(void *func, const uint32_t *args)
{
    return (*(uint32_t (*)(uint32_t, uint32_t, uint32_t))func)(args[0], args[1], args[2]);
}

static uint32_t usmart_call4(void *func, const uint32_t *args)
{
    return (*(uint32_t (*)(uint32_t, uint32_t, uint32_t, uint32_t))func)(
        args[0], args[1], args[2], args[3]);
}

static const usmart_func_call_t g_usmart_call_tab[] =
{
    usmart_call0,
    usmart_call1,
    usmart_call2,
    usmart_call3,
    usmart_call4,
};

static void usmart_exe(void)
{
    uint8_t  id;
    uint8_t  i;
    uint8_t  pnum;
    uint8_t  rval;
    uint32_t res = 0U;
    uint32_t temp[USMART_MAX_PARM];
    char     sfname[USMART_MAX_FNAME_LEN];

    id = usmart_dev.id;
    if (id >= usmart_dev.fnum)
    {
        return;
    }

    (void)usmart_get_fname(usmart_dev.funs[id].name, sfname, &pnum, &rval);
    printf("\r\n%s(", sfname);

    for (i = 0U; i < pnum; i++)
    {
        if (usmart_dev.parmtype[i] == USMART_PARM_STR)
        {
            printf("\"%s\"", (char *)(usmart_dev.parm + usmart_get_parmpos(i)));
            temp[i] = (uint32_t)(uintptr_t)(usmart_dev.parm + usmart_get_parmpos(i));
        }
        else
        {
            memcpy(&temp[i], usmart_dev.parm + usmart_get_parmpos(i), sizeof(uint32_t));
            if (usmart_dev.sptype == USMART_SP_DEC)
            {
                printf("%ld", (long)temp[i]);
            }
            else
            {
                printf("0X%lX", (unsigned long)temp[i]);
            }
        }

        if (i != (uint8_t)(pnum - 1U))
        {
            printf(",");
        }
    }
    printf(")");

    usmart_timx_reset_time();

    if (usmart_dev.pnum <= USMART_MAX_ARG_COUNT)
    {
        res = g_usmart_call_tab[usmart_dev.pnum](usmart_dev.funs[id].func, temp);
    }

    (void)usmart_timx_get_time();

    if (rval != 0U)
    {
        if (usmart_dev.sptype == USMART_SP_DEC)
        {
            printf("=%ld;\r\n", (long)res);
        }
        else
        {
            printf("=0X%lX;\r\n", (unsigned long)res);
        }
    }
    else
    {
        printf(";\r\n");
    }

    if (usmart_dev.runtimeflag)
    {
        printf("Run time: %lu us\r\n", (unsigned long)usmart_dev.runtime);
    }
}

void usmart_scan(void)
{
    char           *pbuf;
    usmart_result_t sta;
    usmart_result_t r;

    pbuf = usmart_get_input_string();
    if (pbuf == 0)
    {
        return;
    }

    sta = usmart_cmd_rec(pbuf);
    if (sta == USMART_RES_OK)
    {
        usmart_exe();
        return;
    }

    r = usmart_sys_cmd_exe(pbuf);
    if (r != USMART_RES_FUNCERR)
    {
        sta = r;
    }

    switch (sta)
    {
        case USMART_RES_FUNCERR:
            printf("Function error!\r\n");
            break;
        case USMART_RES_PARMERR:
            printf("Parameter error!\r\n");
            break;
        case USMART_RES_PARMOVER:
            printf("Too many parameters!\r\n");
            break;
        case USMART_RES_NOFUNCFIND:
            printf("Function not found!\r\n");
            break;
        default:
            break;
    }
}
