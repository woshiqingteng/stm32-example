/**
 * @file    lcd.c
 * @brief   RGB screen driver, ported from the vendor example (lcd.c) with all
 *          MCU (SSD1963/FMC) code and branches removed. Every lcd_* call is
 *          forwarded to the LTDC driver.
 */

#include "lcd.h"
#include "lcdfont.h"
#include "sys.h"

/* Packed glyphs are MSB-first and half as wide as they are tall. */
#define LCD_CHAR_WIDTH_DIV  2U
#define LCD_FONT_BITS       8U

/* Packed bytes per glyph for each supported raster. */
#define LCD_FONT_1206_BYTES 12U
#define LCD_FONT_1608_BYTES 16U
#define LCD_FONT_2412_BYTES 36U
#define LCD_FONT_3216_BYTES 128U

typedef struct
{
    lcd_font_size_t size;   /* glyph height in pixels */
    const uint8_t  *data;   /* glyph table base */
    uint16_t        bytes;  /* packed bytes per glyph */
} lcd_font_desc_t;

static const lcd_font_desc_t g_lcd_fonts[] =
{
    { LCD_FONT_SIZE_12, (const uint8_t *)asc2_1206, LCD_FONT_1206_BYTES },
    { LCD_FONT_SIZE_16, (const uint8_t *)asc2_1608, LCD_FONT_1608_BYTES },
    { LCD_FONT_SIZE_24, (const uint8_t *)asc2_2412, LCD_FONT_2412_BYTES },
    { LCD_FONT_SIZE_32, (const uint8_t *)asc2_3216, LCD_FONT_3216_BYTES }
};

#define LCD_FONT_COUNT (sizeof(g_lcd_fonts) / sizeof(g_lcd_fonts[0]))

_lcd_dev lcddev;
uint32_t g_point_color = 0xFF000000U;
uint32_t g_back_color  = 0xFFFFFFFFU;

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

void lcd_init(void)
{
    lcddev.id = ltdc_panelid_read();

    if (lcddev.id != 0U)
    {
        ltdc_init();
        /* The vendor code leaves ltdc_display_dir() commented out, which
         * leaves lcdltdc.width/height at 0 and makes ltdc_clear()/ltdc_fill()
         * address nothing. Select the default orientation explicitly. */
        lcd_display_dir(LTDC_DIR_PORTRAIT);
    }
    else
    {
        /* no RGB panel present. */
    }
}

void lcd_display_dir(ltdc_dir_t dir)
{
    lcddev.dir = dir;

    if (lcdltdc.pwidth != 0U)
    {
        ltdc_display_dir(dir);
        lcddev.width  = (uint16_t)lcdltdc.width;
        lcddev.height = (uint16_t)lcdltdc.height;
    }
}

void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    if (lcdltdc.pwidth != 0U)
    {
        ltdc_draw_point(x, y, color);
    }
}

uint16_t lcd_get_width(void)
{
    return lcddev.width;
}

uint16_t lcd_get_height(void)
{
    return lcddev.height;
}

void lcd_clear(uint16_t color)
{
    if (lcdltdc.pwidth != 0U)
    {
        ltdc_clear(color);
    }
}

void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    if (lcdltdc.pwidth != 0U)
    {
        ltdc_fill(sx, sy, ex, ey, color);
    }
}

void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    if (lcdltdc.pwidth != 0U)
    {
        ltdc_color_fill(sx, sy, ex, ey, color);
    }
}

void lcd_show_char(uint16_t x, uint16_t y, char chr, lcd_font_size_t size, lcd_text_mode_t mode, uint16_t color)
{
    uint8_t t1;
    uint8_t t;
    uint16_t y0 = y;
    uint8_t csize;
    const uint8_t *pfont;
    const lcd_font_desc_t *font;

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
                lcd_draw_point(x, y, (uint16_t)g_back_color);
            }

            y++;

            if (y >= lcddev.height)
            {
                return;
            }

            if ((uint16_t)(y - y0) == size)
            {
                y = y0;
                x++;

                if (x >= lcddev.width)
                {
                    return;
                }

                break;
            }
        }
    }
}

void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, lcd_font_size_t size, uint16_t color)
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

void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, lcd_font_size_t size, lcd_text_mode_t mode, uint16_t color)
{
    uint8_t t;
    uint8_t temp;
    uint8_t enshow = 0;
    char pad = ' ';

    if ((mode == LCD_TEXT_BG_OVERWRITE_PAD_ZERO) || (mode == LCD_TEXT_TRANSPARENT_PAD_ZERO))
    {
        pad = '0';
    }

    for (t = 0; t < len; t++)
    {
        temp = (uint8_t)((num / bsp_pow(10U, (uint8_t)(len - t - 1U))) % 10U);

        if ((enshow == 0U) && (t < (uint8_t)(len - 1U)))
        {
            if (temp == 0U)
            {
                lcd_show_char((uint16_t)(x + (size / LCD_CHAR_WIDTH_DIV) * t), y, pad, size, mode, color);
                continue;
            }
            else
            {
                enshow = 1U;
            }
        }

        lcd_show_char((uint16_t)(x + (size / LCD_CHAR_WIDTH_DIV) * t), y, (char)temp + '0', size, mode, color);
    }
}

void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, lcd_font_size_t size, const char *p, uint16_t color)
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
