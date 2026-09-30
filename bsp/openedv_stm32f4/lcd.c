/**
 * @file    lcd.c
 * @brief   RGB panel layer: geometry, pixels and ASCII text over an LTDC panel.
 *
 * All panel access goes through the LTDC controller (ltdc.h); image/decoder
 * code uses only this lcd_* API.
 */

#include "lcd.h"
#include "lcd_rgb.h"
#include "lcdfont.h"
#include "sys.h"

/* Packed glyphs are MSB-first and half as wide as they are tall. */
#define LCD_CHAR_WIDTH_DIV  2U
#define LCD_FONT_BITS       8U

/* Packed bytes per glyph for each supported raster. */
#define LCD_FONT_1206_BYTE 12U
#define LCD_FONT_1608_BYTE 16U
#define LCD_FONT_2412_BYTE 36U
#define LCD_FONT_3216_BYTE 64U

typedef struct
{
    lcd_font_size_t size;   /* glyph height in pixels */
    const uint8_t  *data;   /* glyph table base */
    uint16_t        bytes;  /* packed bytes per glyph */
} lcd_font_desc_t;

static const lcd_font_desc_t g_lcd_fonts[] =
{
    { LCD_FONT_SIZE_12, (const uint8_t *)asc2_1206, LCD_FONT_1206_BYTE },
    { LCD_FONT_SIZE_16, (const uint8_t *)asc2_1608, LCD_FONT_1608_BYTE },
    { LCD_FONT_SIZE_24, (const uint8_t *)asc2_2412, LCD_FONT_2412_BYTE },
    { LCD_FONT_SIZE_32, (const uint8_t *)asc2_3216, LCD_FONT_3216_BYTE }
};

#define LCD_FONT_COUNT (sizeof(g_lcd_fonts) / sizeof(g_lcd_fonts[0]))

static lcd_info_t g_lcd_info;
static uint32_t   g_lcd_back_color = 0xFFFFFFFFU;

static const lcd_font_desc_t *lcd_font_get(lcd_font_size_t size)
{
    uint8_t i;

    for (i = 0U; i < (uint8_t)LCD_FONT_COUNT; i++)
    {
        if (g_lcd_fonts[i].size == size)
        {
            return &g_lcd_fonts[i];
        }
    }

    return 0;
}

/* Packed glyphs are MSB-first: row 0 is bit 7. */
static uint8_t glyph_bit(uint8_t byte, uint8_t row)
{
    return (uint8_t)((byte >> (7U - row)) & 1U);
}

/* Refresh the cached info from the LTDC controller state. */
static void lcd_sync_info(void)
{
    const ltdc_dev_t *dev = ltdc_info();

    g_lcd_info.pwidth   = (uint16_t)dev->pwidth;
    g_lcd_info.pheight  = (uint16_t)dev->pheight;
    g_lcd_info.width    = (uint16_t)dev->width;
    g_lcd_info.height   = (uint16_t)dev->height;
    g_lcd_info.pixsize  = (uint8_t)dev->pixsize;
    g_lcd_info.framebuf = ltdc_framebuf();
}

const lcd_info_t *lcd_info(void)
{
    return &g_lcd_info;
}

void lcd_set_back_color(uint32_t color)
{
    g_lcd_back_color = color;
}

uint32_t lcd_get_back_color(void)
{
    return g_lcd_back_color;
}

void lcd_init(void)
{
    const lcd_rgb_cfg_t *panel = lcd_rgb_probe();

    g_lcd_info.id = (panel != 0) ? panel->id : 0U;

    if (panel != 0)
    {
        ltdc_init(panel);
        lcd_display_dir(LCD_DIR_PORTRAIT);
    }
}

void lcd_display_dir(lcd_dir_t dir)
{
    g_lcd_info.dir = dir;

    if (ltdc_info()->pwidth != 0U)
    {
        ltdc_display_dir((dir == LCD_DIR_LANDSCAPE) ? LTDC_DIR_LANDSCAPE : LTDC_DIR_PORTRAIT);
        lcd_sync_info();
        g_lcd_info.dir = dir;
    }
}

void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    if (ltdc_info()->pwidth != 0U)
    {
        ltdc_draw_point(x, y, color);
    }
}

uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
    return ltdc_read_point(x, y);
}

void lcd_clear(uint32_t color)
{
    if (ltdc_info()->pwidth != 0U)
    {
        ltdc_clear(color);
    }
}

void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    if (ltdc_info()->pwidth != 0U)
    {
        ltdc_fill(sx, sy, ex, ey, color);
    }
}

void lcd_blit(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, const uint16_t *src)
{
    if (ltdc_info()->pwidth != 0U)
    {
        ltdc_blit(sx, sy, ex, ey, src);
    }
}

void lcd_show_char(uint16_t x, uint16_t y, char chr, lcd_font_size_t size, lcd_text_mode_t mode, uint32_t color)
{
    uint8_t t1;
    uint8_t t;
    uint16_t y0 = y;
    uint8_t csize;
    const uint8_t *pfont;
    const lcd_font_desc_t *font;

    if ((chr < ' ') || (chr > '~'))
    {
        return;
    }

    font = lcd_font_get(size);

    if (font == 0)
    {
        return;
    }

    csize = (uint8_t)font->bytes;
    pfont = font->data + ((uint8_t)(chr - ' ') * font->bytes);

    for (t = 0; t < csize; t++)
    {
        for (t1 = 0; t1 < LCD_FONT_BITS; t1++)
        {
            if (glyph_bit(pfont[t], t1) != 0U)
            {
                lcd_draw_point(x, y, color);
            }
            else if ((mode == LCD_TEXT_BG_OVERWRITE) || (mode == LCD_TEXT_BG_OVERWRITE_PAD_ZERO))
            {
                lcd_draw_point(x, y, g_lcd_back_color);
            }

            y++;

            if (y >= g_lcd_info.height)
            {
                return;
            }

            if ((uint16_t)(y - y0) == size)
            {
                y = y0;
                x++;

                if (x >= g_lcd_info.width)
                {
                    return;
                }

                break;
            }
        }
    }
}

void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, lcd_font_size_t size, uint32_t color)
{
    uint8_t t;
    uint8_t temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)
    {
        temp = (uint8_t)((num / bsp_pow(10U, (uint8_t)(len - t - 1U))) % 10U);

        if ((enshow == 0U) && (t < (uint8_t)(len - 1U)))
        {
            if (temp == 0U)
            {
                lcd_show_char((uint16_t)(x + (size / LCD_CHAR_WIDTH_DIV) * t), y, ' ', size,
                              LCD_TEXT_BG_OVERWRITE, color);
                continue;
            }
            else
            {
                enshow = 1U;
            }
        }

        lcd_show_char((uint16_t)(x + (size / LCD_CHAR_WIDTH_DIV) * t), y, (char)temp + '0', size,
                      LCD_TEXT_BG_OVERWRITE, color);
    }
}

void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, lcd_font_size_t size, const char *p, uint32_t color)
{
    uint16_t x0 = x;

    width += x;
    height += y;

    while ((*p <= '~') && (*p >= ' '))
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }

        if (y >= height)
        {
            break;
        }

        lcd_show_char(x, y, *p, size, LCD_TEXT_BG_OVERWRITE, color);
        x += size / LCD_CHAR_WIDTH_DIV;
        p++;
    }
}
