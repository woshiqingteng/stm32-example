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
#include "dcmi.h"
#include "ov5640.h"

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

static uint32_t usmart_call_void_u8(void *func, const uint32_t *args)
{
    (*(void (*)(uint8_t))func)((uint8_t)args[0]);
    return 0U;
}

static uint32_t usmart_call_u8_u16_u8(void *func, const uint32_t *args)
{
    return (*(uint8_t (*)(uint16_t, uint8_t))func)((uint16_t)args[0], (uint8_t)args[1]);
}

static uint32_t usmart_call_u8_u16(void *func, const uint32_t *args)
{
    return (*(uint8_t (*)(uint16_t))func)((uint16_t)args[0]);
}

static uint32_t usmart_call_u8_void(void *func, const uint32_t *args)
{
    (void)args;
    return (*(uint8_t (*)(void))func)();
}

static uint32_t usmart_call_void_u8_u8_u8(void *func, const uint32_t *args)
{
    (*(void (*)(uint8_t, uint8_t, uint8_t))func)((uint8_t)args[0], (uint8_t)args[1], (uint8_t)args[2]);
    return 0U;
}

static uint32_t usmart_call_void_u16x4(void *func, const uint32_t *args)
{
    (*(void (*)(uint16_t, uint16_t, uint16_t, uint16_t))func)(
        (uint16_t)args[0], (uint16_t)args[1], (uint16_t)args[2], (uint16_t)args[3]);
    return 0U;
}

static uint32_t usmart_call_u8_u16x4(void *func, const uint32_t *args)
{
    return (*(uint8_t (*)(uint16_t, uint16_t, uint16_t, uint16_t))func)(
        (uint16_t)args[0], (uint16_t)args[1], (uint16_t)args[2], (uint16_t)args[3]);
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

    /* Camera (vendor experiment 38 debug surface). */
    { (void *)ov5640_brightness,       "void ov5640_brightness(uint8_t bright)",                        usmart_call_void_u8 },
    { (void *)ov5640_contrast,         "void ov5640_contrast(uint8_t contrast)",                        usmart_call_void_u8 },
    { (void *)ov5640_color_saturation, "void ov5640_color_saturation(uint8_t sat)",                     usmart_call_void_u8 },
    { (void *)ov5640_light_mode,       "void ov5640_light_mode(uint8_t mode)",                          usmart_call_void_u8 },
    { (void *)ov5640_special_effects,  "void ov5640_special_effects(uint8_t eft)",                      usmart_call_void_u8 },
    { (void *)ov5640_sharpness,        "void ov5640_sharpness(uint8_t sharp)",                          usmart_call_void_u8 },
    { (void *)ov5640_test_pattern,     "void ov5640_test_pattern(uint8_t mode)",                        usmart_call_void_u8 },
    { (void *)ov5640_flash_ctrl,       "void ov5640_flash_ctrl(uint8_t sw)",                            usmart_call_void_u8 },
    { (void *)ov5640_write_reg,        "uint8_t ov5640_write_reg(uint16_t reg, uint8_t data)",          usmart_call_u8_u16_u8 },
    { (void *)ov5640_read_reg,         "uint8_t ov5640_read_reg(uint16_t reg)",                         usmart_call_u8_u16 },
    { (void *)ov5640_focus_init,       "uint8_t ov5640_focus_init(void)",                               usmart_call_u8_void },
    { (void *)ov5640_focus_single,     "uint8_t ov5640_focus_single(void)",                             usmart_call_u8_void },
    { (void *)ov5640_focus_constant,   "uint8_t ov5640_focus_constant(void)",                           usmart_call_u8_void },
    { (void *)ov5640_outsize_set,      "uint8_t ov5640_outsize_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height)", usmart_call_u8_u16x4 },
    { (void *)ov5640_image_window_set, "uint8_t ov5640_image_window_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height)", usmart_call_u8_u16x4 },
    { (void *)dcmi_cr_set,             "void dcmi_cr_set(uint8_t pclk, uint8_t hsync, uint8_t vsync)",  usmart_call_void_u8_u8_u8 },
    { (void *)dcmi_set_window,         "void dcmi_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)", usmart_call_void_u16x4 },
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
