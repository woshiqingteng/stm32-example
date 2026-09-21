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

#include "usmart/usmart.h"
#include "delay.h"
#include "led.h"
#include "lcd.h"
#include "ltdc.h"
#include "sdram.h"

/* ---- Exact-typed trampolines (one per distinct target signature) ---- */

static uint32_t usmart_call_u32_u32(void *func, const uint32_t *args)
{
    return (*(uint32_t (*)(uint32_t))func)(args[0]);
}

static uint32_t usmart_call_u32_u16_u16(void *func, const uint32_t *args)
{
    return (*(uint32_t (*)(uint16_t, uint16_t))func)((uint16_t)args[0], (uint16_t)args[1]);
}

static uint32_t usmart_call_void(void *func, const uint32_t *args)
{
    (void)args;
    (*(void (*)(void))func)();
    return 0U;
}

static uint32_t usmart_call_void_u8(void *func, const uint32_t *args)
{
    (*(void (*)(uint8_t))func)((uint8_t)args[0]);
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

static uint32_t usmart_call_void_ltdclayer(void *func, const uint32_t *args)
{
    (*(void (*)(ltdc_layer_t))func)((ltdc_layer_t)args[0]);
    return 0U;
}

static uint32_t usmart_call_void_ltdcdir(void *func, const uint32_t *args)
{
    (*(void (*)(ltdc_dir_t))func)((ltdc_dir_t)args[0]);
    return 0U;
}

static uint32_t usmart_call_void_u32_u32(void *func, const uint32_t *args)
{
    (*(void (*)(uint32_t, uint32_t))func)(args[0], args[1]);
    return 0U;
}

static uint32_t usmart_call_void_ltdclayer_u8(void *func, const uint32_t *args)
{
    (*(void (*)(ltdc_layer_t, uint8_t))func)((ltdc_layer_t)args[0], (uint8_t)args[1]);
    return 0U;
}

static uint32_t usmart_call_void_u16_u16_u32(void *func, const uint32_t *args)
{
    (*(void (*)(uint16_t, uint16_t, uint32_t))func)((uint16_t)args[0], (uint16_t)args[1], args[2]);
    return 0U;
}

static uint32_t usmart_call_void_u16x4_u32(void *func, const uint32_t *args)
{
    (*(void (*)(uint16_t, uint16_t, uint16_t, uint16_t, uint32_t))func)(
        (uint16_t)args[0], (uint16_t)args[1], (uint16_t)args[2], (uint16_t)args[3], args[4]);
    return 0U;
}

static uint32_t usmart_call_void_ltdclayer_u16x4(void *func, const uint32_t *args)
{
    (*(void (*)(ltdc_layer_t, uint16_t, uint16_t, uint16_t, uint16_t))func)(
        (ltdc_layer_t)args[0], (uint16_t)args[1], (uint16_t)args[2],
        (uint16_t)args[3], (uint16_t)args[4]);
    return 0U;
}

static uint32_t usmart_call_void_u16_u16_u32_u8_u8_u16(void *func, const uint32_t *args)
{
    (*(void (*)(uint16_t, uint16_t, uint32_t, uint8_t, uint8_t, uint16_t))func)(
        (uint16_t)args[0], (uint16_t)args[1], args[2],
        (uint8_t)args[3], (uint8_t)args[4], (uint16_t)args[5]);
    return 0U;
}

static uint32_t usmart_call_void_u16_u16_u32_u8_u8_textmode_u16(void *func, const uint32_t *args)
{
    (*(void (*)(uint16_t, uint16_t, uint32_t, uint8_t, uint8_t, lcd_text_mode_t, uint16_t))func)(
        (uint16_t)args[0], (uint16_t)args[1], args[2],
        (uint8_t)args[3], (uint8_t)args[4], (lcd_text_mode_t)args[5], (uint16_t)args[6]);
    return 0U;
}

static uint32_t usmart_call_void_u16_u16_u16_u16_u8_str_u16(void *func, const uint32_t *args)
{
    (*(void (*)(uint16_t, uint16_t, uint16_t, uint16_t, uint8_t, const char *, uint16_t))func)(
        (uint16_t)args[0], (uint16_t)args[1], (uint16_t)args[2], (uint16_t)args[3],
        (uint8_t)args[4], (const char *)(uintptr_t)args[5], (uint16_t)args[6]);
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

    { (void *)lcd_clear,      "void lcd_clear(uint16_t color)",             usmart_call_void_u16 },
    { (void *)lcd_draw_point, "void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)",
      usmart_call_void_u16_u16_u32 },
    { (void *)lcd_fill,       "void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)",
      usmart_call_void_u16x4_u32 },
    { (void *)lcd_show_num,   "void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)",
      usmart_call_void_u16_u16_u32_u8_u8_u16 },
    { (void *)lcd_show_xnum,  "void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)",
      usmart_call_void_u16_u16_u32_u8_u8_textmode_u16 },
    { (void *)lcd_show_string, "void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)",
      usmart_call_void_u16_u16_u16_u16_u8_str_u16 },

    { (void *)ltdc_switch,    "void ltdc_switch(uint8_t sw)",               usmart_call_void_u8 },
    { (void *)ltdc_layer_switch, "void ltdc_layer_switch(uint8_t layerx, uint8_t sw)",
      usmart_call_void_ltdclayer_u8 },
    { (void *)ltdc_select_layer, "void ltdc_select_layer(uint8_t layerx)",  usmart_call_void_ltdclayer },
    { (void *)ltdc_display_dir,  "void ltdc_display_dir(uint8_t dir)",      usmart_call_void_ltdcdir },
    { (void *)ltdc_draw_point,   "void ltdc_draw_point(uint16_t x, uint16_t y, uint32_t color)",
      usmart_call_void_u16_u16_u32 },
    { (void *)ltdc_read_point,   "uint32_t ltdc_read_point(uint16_t x, uint16_t y)",
      usmart_call_u32_u16_u16 },
    { (void *)ltdc_fill,         "void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)",
      usmart_call_void_u16x4_u32 },
    { (void *)ltdc_clear,        "void ltdc_clear(uint32_t color)",         usmart_call_void_u32 },
    { (void *)ltdc_layer_window_config,
      "void ltdc_layer_window_config(uint8_t layerx, uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)",
      usmart_call_void_ltdclayer_u16x4 },

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
    .runtimeflag = false,
    .runtime     = 0U,
};
