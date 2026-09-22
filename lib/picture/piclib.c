/**
 * @file    piclib.c
 * @brief   Image viewer middleware (ALIENTEK PICTURE), ported to the RGB panel.
 *          The vendor MCU-screen path is removed; all drawing uses the RGB
 *          panel / LTDC API.
 */

#include "piclib.h"
#include "ltdc.h"
#include "lcd.h"
#include "malloc.h"
#include "exfuns.h"

_pic_info picinfo;
_pic_phy  pic_phy;

extern uint32_t *g_ltdc_framebuf[2];

/** @brief  Horizontal line helper: the GIF decoder needs one and the RGB panel
 *          only offers a rectangle fill. */
static void piclib_draw_hline(uint16_t x0, uint16_t y0, uint16_t len, uint16_t color)
{
    if (len == 0U)
    {
        return;
    }

    lcd_fill(x0, y0, (uint16_t)(x0 + len - 1U), y0, color);
}

/** @brief  Fast colour fill straight into the LTDC frame buffer. */
static void piclib_fill_color(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *color)
{
    uint16_t i, j;

    if ((lcdltdc.pwidth != 0U) && (lcdltdc.dir == LTDC_DIR_PORTRAIT))
    {
        for (i = 0; i < height; i++)
        {
            for (j = 0; j < width; j++)
            {
                *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] +
                              lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - x - j - 1) + y + i)) =
                    color[i * width + j];
            }
        }
    }
    else
    {
        lcd_color_fill(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U), color);
    }
}

void piclib_init(void)
{
    pic_phy.read_point  = ltdc_read_point;
    pic_phy.draw_point  = lcd_draw_point;
    pic_phy.fill        = lcd_fill;
    pic_phy.draw_hline  = piclib_draw_hline;
    pic_phy.fillcolor   = piclib_fill_color;

    picinfo.lcdwidth  = lcd_get_width();
    picinfo.lcdheight = lcd_get_height();

    picinfo.ImgWidth  = 0;
    picinfo.ImgHeight = 0;
    picinfo.Div_Fac   = 0;
    picinfo.S_Height  = 0;
    picinfo.S_Width   = 0;
    picinfo.S_XOFF    = 0;
    picinfo.S_YOFF    = 0;
    picinfo.staticx   = 0;
    picinfo.staticy   = 0;
}

uint16_t piclib_alpha_blend(uint16_t src, uint16_t dst, uint8_t alpha)
{
    uint32_t src2;
    uint32_t dst2;

    src2 = ((src << 16) | src) & 0x07E0F81F;
    dst2 = ((dst << 16) | dst) & 0x07E0F81F;

    dst2 = ((((dst2 - src2) * alpha) >> 5) + src2) & 0x07E0F81F;
    return (uint16_t)((dst2 >> 16) | dst2);
}

void piclib_ai_draw_init(void)
{
    float temp, temp1;

    temp  = (float)picinfo.S_Width / (float)picinfo.ImgWidth;
    temp1 = (float)picinfo.S_Height / (float)picinfo.ImgHeight;

    if (temp < temp1)
    {
        temp1 = temp;
    }

    if (temp1 > 1)
    {
        temp1 = 1;
    }

    picinfo.S_XOFF += (uint32_t)((picinfo.S_Width - temp1 * picinfo.ImgWidth) / 2);
    picinfo.S_YOFF += (uint32_t)((picinfo.S_Height - temp1 * picinfo.ImgHeight) / 2);
    temp1 *= 8192;
    picinfo.Div_Fac = (uint32_t)temp1;
    picinfo.staticx = 0xffff;
    picinfo.staticy = 0xffff;
}

uint8_t piclib_is_element_ok(uint16_t x, uint16_t y, uint8_t chg)
{
    if ((x != picinfo.staticx) || (y != picinfo.staticy))
    {
        if (chg == 1U)
        {
            picinfo.staticx = x;
            picinfo.staticy = y;
        }

        return 1;
    }

    return 0;
}

uint8_t piclib_ai_load_picfile(char *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t fast)
{
    uint8_t res;
    uint8_t temp;

    if ((x + width) > picinfo.lcdwidth)
    {
        return PIC_WINDOW_ERR;
    }

    if ((y + height) > picinfo.lcdheight)
    {
        return PIC_WINDOW_ERR;
    }

    if ((width == 0U) || (height == 0U))
    {
        return PIC_WINDOW_ERR;
    }

    picinfo.S_Height = height;
    picinfo.S_Width  = width;

    if ((picinfo.S_Height == 0U) || (picinfo.S_Width == 0U))
    {
        picinfo.S_Height = lcd_get_height();
        picinfo.S_Width  = lcd_get_width();
        return FALSE;
    }

    if (pic_phy.fillcolor == NULL)
    {
        fast = 0U;
    }

    picinfo.S_YOFF = y;
    picinfo.S_XOFF = x;

    temp = exfuns_file_type(filename);

    switch (temp)
    {
        case T_BMP:
            res = stdbmp_decode(filename);
            break;

        case T_JPG:
        case T_JPEG:
            res = jpg_decode(filename, fast);
            break;

        case T_GIF:
            res = gif_decode(filename, x, y, width, height);
            break;

        default:
            res = PIC_FORMAT_ERR;
            break;
    }

    return res;
}

void *piclib_mem_malloc(uint32_t size)
{
    return (void *)mymalloc(SRAMIN, size);
}

void piclib_mem_free(void *paddr)
{
    myfree(SRAMIN, paddr);
}
