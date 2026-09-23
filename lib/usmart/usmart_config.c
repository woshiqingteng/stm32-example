/**
 * @file    usmart_config.c
 * @brief   USMART user function table and control block.
 *
 * Add functions to usmart_nametab[] to make them callable from the serial
 * console. The name string is the exact source signature; the parser derives
 * the parameter count from it. The third field is an exact-typed trampoline:
 * calling a function through a pointer of a different signature is undefined
 * behaviour, so every entry carries a wrapper whose prototype matches the
 * target. usmart_exe() invokes entry.call(entry.func, args).
 */

#include "usmart.h"
#include "delay.h"
#include "led.h"
#include "sdram.h"

/* ---- Exact-typed trampolines (one per distinct target signature) ---- */

static uint32_t usmart_call_u32_u32(void *func, const uint32_t *args)
{
    return (*(uint32_t (*)(uint32_t))func)(args[0]);
}

static uint32_t usmart_call_void(void *func, const uint32_t *args)
{
    (void)args;
    (*(void (*)(void))func)();
    return 0U;
}

static uint32_t usmart_call_void_u16(void *func, const uint32_t *args)
{
    (*(void (*)(uint16_t))func)((uint16_t)args[0]);
    return 0U;
}

static uint32_t usmart_call_void_u32(void *func, const uint32_t *args)
{
    (*(void (*)(uint32_t))func)(args[0]);
    return 0U;
}

static uint32_t usmart_call_void_ledid(void *func, const uint32_t *args)
{
    (*(void (*)(led_id_t))func)((led_id_t)args[0]);
    return 0U;
}

static uint32_t usmart_call_void_u32_u32(void *func, const uint32_t *args)
{
    (*(void (*)(uint32_t, uint32_t))func)(args[0], args[1]);
    return 0U;
}

/* ---- User function table ---- */

struct _m_usmart_nametab usmart_nametab[] =
{
#if USMART_USE_WRFUNS == 1
    { (void *)read_addr,  "uint32_t read_addr(uint32_t addr)",              usmart_call_u32_u32 },
    { (void *)write_addr, "void write_addr(uint32_t addr, uint32_t val)",   usmart_call_void_u32_u32 },
#endif

    { (void *)delay_ms,   "void delay_ms(uint16_t nms)",                    usmart_call_void_u16 },
    { (void *)delay_us,   "void delay_us(uint32_t nus)",                    usmart_call_void_u32 },

    { (void *)led_on,     "void led_on(led_id_t id)",                       usmart_call_void_ledid },
    { (void *)led_off,    "void led_off(led_id_t id)",                      usmart_call_void_ledid },

    { (void *)sdram_init,        "void sdram_init(void)",                   usmart_call_void },
};

/* ---- Control block ---- */

struct _m_usmart_dev usmart_dev =
{
    .funs        = usmart_nametab,
    .init        = usmart_init,
    .cmd_rec     = usmart_cmd_rec,
    .exe         = usmart_exe,
    .scan        = usmart_scan,
    .fnum        = (uint8_t)(sizeof(usmart_nametab) / sizeof(usmart_nametab[0])),
    .pnum        = 0U,
    .id          = 0U,
    .sptype      = SP_TYPE_HEX,
    .parmtype    = 0U,
    .plentbl     = {0U},
    .parm        = {0U},
    .runtimeflag = USMART_RUNTIME_OFF,
    .runtime     = 0U,
};
