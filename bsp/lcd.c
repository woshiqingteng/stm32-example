/**
 * @file    lcd.c
 * @brief   RGB screen driver, ported from the vendor example (lcd.c) with all
 *          MCU (SSD1963/FMC) code and branches removed. Every lcd_* call is
 *          forwarded to the LTDC driver.
 */

#include "lcd.h"
#include "lcdfont.h"

_lcd_dev lcddev;
uint32_t g_point_color = 0xFF000000U;
uint32_t g_back_color  = 0xFFFFFFFFU;

void lcd_init(void)
{
    lcddev.id = ltdc_panelid_read();

    if (lcddev.id != 0U)
    {
        ltdc_init();
        /* The vendor code leaves ltdc_display_dir() commented out, which
         * leaves lcdltdc.width/height at 0 and makes ltdc_clear()/ltdc_fill()
         * address nothing. Select the default orientation explicitly. */
        lcd_display_dir(0);
    }
    else
    {
        /* no RGB panel present. */
    }
}

void lcd_display_dir(uint8_t dir)
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

void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t temp;
    uint8_t t1;
    uint8_t t;
    uint16_t y0 = y;
    uint8_t csize = 0;
    uint8_t *pfont = 0;

    csize = (uint8_t)((size / 8U + ((size % 8U) ? 1U : 0U)) * (size / 2U));
    chr = (char)(chr - ' ');

    switch (size)
    {
        case 12:
            pfont = (uint8_t *)asc2_1206[(uint8_t)chr];
            break;

        case 16:
            pfont = (uint8_t *)asc2_1608[(uint8_t)chr];
            break;

        case 24:
            pfont = (uint8_t *)asc2_2412[(uint8_t)chr];
            break;

        case 32:
            pfont = (uint8_t *)asc2_3216[(uint8_t)chr];
            break;

        default:
            return;
    }

    for (t = 0; t < csize; t++)
    {
        temp = pfont[t];

        for (t1 = 0; t1 < 8U; t1++)
        {
            if ((temp & 0x80U) != 0U)
            {
                lcd_draw_point(x, y, color);
            }
            else if (mode == 0U)
            {
                lcd_draw_point(x, y, (uint16_t)g_back_color);
            }

            temp <<= 1;
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

static uint32_t lcd_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;

    while (n-- != 0U)
    {
        result *= m;
    }

    return result;
}

void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t;
    uint8_t temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)
    {
        temp = (uint8_t)((num / lcd_pow(10U, (uint8_t)(len - t - 1U))) % 10U);

        if ((enshow == 0U) && (t < (uint8_t)(len - 1U)))
        {
            if (temp == 0U)
            {
                lcd_show_char((uint16_t)(x + (size / 2U) * t), y, ' ', size, 0, color);
                continue;
            }
            else
            {
                enshow = 1U;
            }
        }

        lcd_show_char((uint16_t)(x + (size / 2U) * t), y, (char)temp + '0', size, 0, color);
    }
}

void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t t;
    uint8_t temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)
    {
        temp = (uint8_t)((num / lcd_pow(10U, (uint8_t)(len - t - 1U))) % 10U);

        if ((enshow == 0U) && (t < (uint8_t)(len - 1U)))
        {
            if (temp == 0U)
            {
                if ((mode & 0x80U) != 0U)
                {
                    lcd_show_char((uint16_t)(x + (size / 2U) * t), y, '0', size, mode & 0x01U, color);
                }
                else
                {
                    lcd_show_char((uint16_t)(x + (size / 2U) * t), y, ' ', size, mode & 0x01U, color);
                }

                continue;
            }
            else
            {
                enshow = 1U;
            }
        }

        lcd_show_char((uint16_t)(x + (size / 2U) * t), y, (char)temp + '0', size, mode & 0x01U, color);
    }
}

void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = (uint8_t)x;

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

        lcd_show_char(x, y, *p, size, 0, color);
        x += size / 2U;
        p++;
    }
}
