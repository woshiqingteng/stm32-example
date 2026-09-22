/**
 * @file    text.c
 * @brief   GBK text rendering on top of the RGB panel. Ported from the ALIENTEK
 *          TEXT middleware; the vendor MCU-screen branches are dropped and all
 *          drawing goes through the RGB panel API.
 */

#include <string.h>
#include <stdint.h>
#include "text.h"
#include "lcd.h"
#include "malloc.h"
#include "norflash.h"

/** @brief  Translate the middleware mode (0 overwrite, 1 transparent) to the
 *          RGB panel text mode. */
static lcd_text_mode_t text_lcd_mode(uint8_t mode)
{
    return (mode == 0U) ? LCD_TEXT_BG_OVERWRITE : LCD_TEXT_TRANSPARENT;
}

/** @brief  Fetch the GBK glyph bitmap for one character from the NOR flash
 *          font store. */
static void text_get_hz_mat(uint8_t *code, uint8_t *mat, uint8_t size)
{
    uint8_t qh, ql;
    uint8_t i;
    uint64_t foffset;
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);

    qh = *code;
    ql = *(++code);

    if (qh < 0x81 || ql < 0x40 || ql == 0xff || qh == 0xff)
    {
        for (i = 0; i < csize; i++)
        {
            *mat++ = 0x00;
        }

        return;
    }

    if (ql < 0x7f)
    {
        ql -= 0x40;
    }
    else
    {
        ql -= 0x41;
    }

    qh -= 0x81;
    foffset = ((uint64_t)190 * qh + ql) * csize;

    switch (size)
    {
        case 12:
            norflash_read(mat, (uint32_t)(foffset + ftinfo.f12addr), csize);
            break;

        case 16:
            norflash_read(mat, (uint32_t)(foffset + ftinfo.f16addr), csize);
            break;

        case 24:
            norflash_read(mat, (uint32_t)(foffset + ftinfo.f24addr), csize);
            break;

        case 32:
            norflash_read(mat, (uint32_t)(foffset + ftinfo.f32addr), csize);
            break;

        default:
            break;
    }
}

void text_show_font(uint16_t x, uint16_t y, uint8_t *font, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t temp, t, t1;
    uint16_t y0 = y;
    uint8_t *dzk;
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);

    if (size != 12 && size != 16 && size != 24 && size != 32)
    {
        return;
    }

    dzk = mymalloc(SRAMIN, csize);

    if (dzk == 0)
    {
        return;
    }

    text_get_hz_mat(font, dzk, size);

    for (t = 0; t < csize; t++)
    {
        temp = dzk[t];

        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)
            {
                lcd_draw_point(x, y, color);
            }
            else if (mode == 0)
            {
                lcd_draw_point(x, y, g_back_color);
            }

            temp <<= 1;
            y++;

            if ((y - y0) == size)
            {
                y = y0;
                x++;
                break;
            }
        }
    }

    myfree(SRAMIN, dzk);
}

void text_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *str, uint8_t size, uint8_t mode, uint16_t color)
{
    uint16_t x0 = x;
    uint16_t y0 = y;
    uint8_t bHz = 0;
    uint8_t *pstr = (uint8_t *)str;

    while (*pstr != 0)
    {
        if (!bHz)
        {
            if (*pstr > 0x80)
            {
                bHz = 1;
            }
            else
            {
                if (x > (x0 + width - size / 2))
                {
                    y += size;
                    x = x0;
                }

                if (y > (y0 + height - size))
                {
                    break;
                }

                if (*pstr == 13)
                {
                    y += size;
                    x = x0;
                    pstr++;
                }
                else
                {
                    lcd_show_char(x, y, (char)*pstr, (lcd_font_size_t)size, text_lcd_mode(mode), color);
                }

                pstr++;

                x += size / 2;
            }
        }
        else
        {
            bHz = 0;

            if (x > (x0 + width - size))
            {
                y += size;
                x = x0;
            }

            if (y > (y0 + height - size))
            {
                break;
            }

            text_show_font(x, y, pstr, size, mode, color);
            pstr += 2;
            x += size;
        }
    }
}

void text_show_string_middle(uint16_t x, uint16_t y, char *str, uint8_t size, uint16_t width, uint16_t color)
{
    uint16_t strlenth = 0;

    strlenth = (uint16_t)strlen((const char *)str);
    strlenth = (uint16_t)(strlenth * (size / 2));

    if (strlenth > width)
    {
        text_show_string(x, y, lcd_get_width(), lcd_get_height(), str, size, 1, color);
    }
    else
    {
        strlenth = (uint16_t)((width - strlenth) / 2);
        text_show_string((uint16_t)(strlenth + x), y, lcd_get_width(), lcd_get_height(), str, size, 1, color);
    }
}
