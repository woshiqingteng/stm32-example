/**
 * @file    bmp.c
 * @brief   BMP decoder and encoder, ported from the ALIENTEK PICTURE middleware
 *          to the RGB panel API.
 */

#include <string.h>
#include <stdbool.h>
#include "bmp.h"
#include "piclib.h"
#include "ff.h"
#include "malloc.h"
#include "lcd.h"

#if BMP_USE_MALLOC == 0
FIL f_bfile;
uint8_t bmpreadbuf[BMP_DBUF_SIZE];
#endif

uint8_t stdbmp_decode(const char *filename)
{
    FIL *f_bmp;
    UINT br;

    uint16_t count;
    uint8_t  rgb, color_byte;
    uint16_t x, y, color;
    uint16_t countpix = 0;

    uint16_t  realx = 0;
    uint16_t realy = 0;
    bool     yok = true;
    uint8_t res;

    uint8_t *databuf;
    uint16_t readlen = BMP_DBUF_SIZE;

    uint8_t *bmpbuf;
    uint8_t bicompression = 0;

    uint16_t rowlen;
    BITMAPINFO *pbmp;

#if BMP_USE_MALLOC == 1
    databuf = (uint8_t *)piclib_mem_malloc(readlen);

    if (databuf == NULL)return PIC_MEM_ERR;

    f_bmp = (FIL *)piclib_mem_malloc(sizeof(FIL));

    if (f_bmp == NULL)
    {
        piclib_mem_free(databuf);
        return PIC_MEM_ERR;
    }

#else
    databuf = bmpreadbuf;
    f_bmp = &f_bfile;
#endif
    res = f_open(f_bmp, (const TCHAR *)filename, FA_READ);

    if (res == 0)
    {
        f_read(f_bmp, databuf, readlen, (UINT *)&br);
        pbmp = (BITMAPINFO *)databuf;
        count = pbmp->bmfHeader.bfOffBits;
        color_byte = pbmp->bmiHeader.biBitCount / 8;
        bicompression = pbmp->bmiHeader.biCompression;
        picinfo.ImgHeight = pbmp->bmiHeader.biHeight;
        picinfo.ImgWidth = pbmp->bmiHeader.biWidth;
        piclib_ai_draw_init();

        if ((picinfo.ImgWidth * color_byte) % 4)
        {
            rowlen = ((picinfo.ImgWidth * color_byte) / 4 + 1) * 4;
        }
        else
        {
            rowlen = picinfo.ImgWidth * color_byte;
        }

        color = 0;
        x = 0 ;
        y = picinfo.ImgHeight;
        rgb = 0;

        realy = (y * picinfo.Div_Fac) >> 13;
        bmpbuf = databuf;

        while (1)
        {
            while (count < readlen)
            {
                if (color_byte == 3)
                {
                    switch (rgb)
                    {
                        case 0:
                            color = bmpbuf[count] >> 3;
                            break ;

                        case 1:
                            color += ((uint16_t)bmpbuf[count] << 3) & 0X07E0;
                            break;

                        case 2 :
                            color += ((uint16_t)bmpbuf[count] << 8) & 0XF800;
                            break ;
                    }
                }
                else if (color_byte == 2)
                {
                    switch (rgb)
                    {
                        case 0 :
                            if (bicompression == BI_RGB)
                            {
                                color = ((uint16_t)bmpbuf[count] & 0X1F);
                                color += (((uint16_t)bmpbuf[count]) & 0XE0) << 1;
                            }
                            else
                            {
                                color = bmpbuf[count];
                            }

                            break ;

                        case 1 :
                            if (bicompression == BI_RGB)
                            {
                                color += (uint16_t)bmpbuf[count] << 9;
                            }
                            else
                            {
                                color += (uint16_t)bmpbuf[count] << 8;
                            }

                            break ;
                    }
                }
                else if (color_byte == 4)
                {
                    switch (rgb)
                    {
                        case 0:
                            color = bmpbuf[count] >> 3;
                            break ;

                        case 1:
                            color += ((uint16_t)bmpbuf[count] << 3) & 0X07E0;
                            break;

                        case 2 :
                            color += ((uint16_t)bmpbuf[count] << 8) & 0XF800;
                            break ;

                        case 3 :

                            break ;
                    }
                }
                else if (color_byte == 1)
                {
                }

                rgb++;
                count++ ;

                if (rgb == color_byte)
                {
                    if (x < picinfo.ImgWidth)
                    {
                        realx = (x * picinfo.Div_Fac) >> 13;

                        if (piclib_is_element_ok(realx, realy, true) && yok)
                        {
                            pic_phy.draw_point(realx + picinfo.S_XOFF, realy + picinfo.S_YOFF - 1, color);
                        }
                    }

                    x++;
                    color = 0x00;
                    rgb = 0;
                }

                countpix++;

                if (countpix >= rowlen)
                {
                    y--;

                    if (y == 0)break;

                    realy = (y * picinfo.Div_Fac) >> 13;

                    yok = piclib_is_element_ok(realx, realy, false);

                    if ((realy + picinfo.S_YOFF) == 0)break;

                    x = 0;
                    countpix = 0;
                    color = 0x00;
                    rgb = 0;
                }
            }

            res = f_read(f_bmp, databuf, readlen, (UINT *)&br);

            if (br != readlen)readlen = br;

            if (res || br == 0)break;

            bmpbuf = databuf;
            count = 0;
        }

        f_close(f_bmp);
    }

#if BMP_USE_MALLOC == 1
    piclib_mem_free(databuf);
    piclib_mem_free(f_bmp);
#endif
    return res;
}

uint8_t minibmp_decode(uint8_t *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t acolor, uint8_t mode)
{
    FIL *f_bmp;
    UINT br;
    uint8_t  color_byte;
    uint16_t tx, ty, color;

    uint8_t res;
    uint16_t i, j;
    uint8_t *databuf;
    uint16_t readlen = BMP_DBUF_SIZE;

    uint8_t *bmpbuf;
    uint8_t bicompression = 0;

    uint16_t rowcnt;
    uint16_t rowlen;
    uint16_t rowpix = 0;
    uint8_t rowadd;

    uint16_t tmp_color;

    uint8_t alphabend = 0xff;
    uint8_t alphamode = mode >> 6;
    BITMAPINFO *pbmp;

    picinfo.S_Height = height;
    picinfo.S_Width = width;

#if BMP_USE_MALLOC == 1
    databuf = (uint8_t *)piclib_mem_malloc(readlen);

    if (databuf == NULL)return PIC_MEM_ERR;

    f_bmp = (FIL *)piclib_mem_malloc(sizeof(FIL));

    if (f_bmp == NULL)
    {
        piclib_mem_free(databuf);
        return PIC_MEM_ERR;
    }

#else
    databuf = bmpreadbuf;
    f_bmp = &f_bfile;
#endif
    res = f_open(f_bmp, (const TCHAR *)filename, FA_READ);

    if (res == 0)
    {
        f_read(f_bmp, databuf, sizeof(BITMAPINFO), (UINT *)&br);
        pbmp = (BITMAPINFO *)databuf;
        color_byte = pbmp->bmiHeader.biBitCount / 8;
        bicompression = pbmp->bmiHeader.biCompression;
        picinfo.ImgHeight = pbmp->bmiHeader.biHeight;
        picinfo.ImgWidth = pbmp->bmiHeader.biWidth;

        if ((picinfo.ImgWidth * color_byte) % 4)
        {
            rowlen = ((picinfo.ImgWidth * color_byte) / 4 + 1) * 4;
        }
        else
        {
            rowlen = picinfo.ImgWidth * color_byte;
        }

        rowadd = rowlen - picinfo.ImgWidth * color_byte;

        color = 0;
        tx = 0 ;
        ty = picinfo.ImgHeight - 1;

        if (picinfo.ImgWidth <= picinfo.S_Width && picinfo.ImgHeight <= picinfo.S_Height)
        {
            x += (picinfo.S_Width - picinfo.ImgWidth) / 2;
            y += (picinfo.S_Height - picinfo.ImgHeight) / 2;
            rowcnt = readlen / rowlen;
            readlen = rowcnt * rowlen;
            rowpix = picinfo.ImgWidth;
            f_lseek(f_bmp, pbmp->bmfHeader.bfOffBits);

            while (1)
            {
                res = f_read(f_bmp, databuf, readlen, (UINT *)&br);
                bmpbuf = databuf;

                if (br != readlen)rowcnt = br / rowlen;

                if (color_byte == 3)
                {
                    for (j = 0; j < rowcnt; j++)
                    {
                        for (i = 0; i < rowpix; i++)
                        {
                            color = (*bmpbuf++) >> 3;
                            color += ((uint16_t)(*bmpbuf++) << 3) & 0X07E0;
                            color += (((uint16_t) * bmpbuf++) << 8) & 0XF800;
                            pic_phy.draw_point(x + tx, y + ty, color);
                            tx++;
                        }

                        bmpbuf += rowadd;
                        tx = 0;
                        ty--;
                    }
                }
                else if (color_byte == 2)
                {
                    for (j = 0; j < rowcnt; j++)
                    {
                        if (bicompression == BI_RGB)
                        {
                            for (i = 0; i < rowpix; i++)
                            {
                                color = ((uint16_t) * bmpbuf & 0X1F);
                                color += (((uint16_t) * bmpbuf++) & 0XE0) << 1;
                                color += ((uint16_t) * bmpbuf++) << 9;
                                pic_phy.draw_point(x + tx, y + ty, color);
                                tx++;
                            }
                        }
                        else
                        {
                            for (i = 0; i < rowpix; i++)
                            {
                                color = *bmpbuf++;
                                color += ((uint16_t) * bmpbuf++) << 8;
                                pic_phy.draw_point(x + tx, y + ty, color);
                                tx++;
                            }
                        }

                        bmpbuf += rowadd;
                        tx = 0;
                        ty--;
                    }
                }
                else if (color_byte == 4)
                {
                    for (j = 0; j < rowcnt; j++)
                    {
                        for (i = 0; i < rowpix; i++)
                        {
                            color = (*bmpbuf++) >> 3;
                            color += ((uint16_t)(*bmpbuf++) << 3) & 0X07E0;
                            color += (((uint16_t) * bmpbuf++) << 8) & 0XF800;
                            alphabend = *bmpbuf++;

                            if (alphamode != 1)
                            {
                                tmp_color = pic_phy.read_point(x + tx, y + ty);

                                if (alphamode == 2)
                                {
                                    tmp_color = piclib_alpha_blend(tmp_color, acolor, mode & 0X1F);
                                }

                                color = piclib_alpha_blend(tmp_color, color, alphabend / 8);
                            }
                            else
                            {
                                tmp_color = piclib_alpha_blend(acolor, color, alphabend / 8);
                            }

                            pic_phy.draw_point(x + tx, y + ty, color);
                            tx++;
                        }

                        bmpbuf += rowadd;
                        tx = 0;
                        ty--;
                    }

                }

                if (br != readlen || res)break;
            }
        }

        f_close(f_bmp);
    }
    else
    {
        res = PIC_SIZE_ERR;
    }

#if BMP_USE_MALLOC == 1
    piclib_mem_free(databuf);
    piclib_mem_free(f_bmp);
#endif
    return res;
}

uint8_t bmp_encode(uint8_t *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t mode)
{
    FIL *f_bmp;
    UINT bw = 0;
    uint16_t bmpheadsize;
    BITMAPINFO hbmp;
    uint8_t res = 0;
    uint16_t tx, ty;
    uint16_t *databuf;
    uint16_t pixcnt;
    uint16_t bi4width;

    if (width == 0 || height == 0)return PIC_WINDOW_ERR;

    if ((x + width - 1) > lcd_get_width())return PIC_WINDOW_ERR;

    if ((y + height - 1) > lcd_get_height())return PIC_WINDOW_ERR;

#if BMP_USE_MALLOC == 1

    databuf = (uint16_t *)piclib_mem_malloc(2048);

    if (databuf == NULL)return PIC_MEM_ERR;

    f_bmp = (FIL *)piclib_mem_malloc(sizeof(FIL));

    if (f_bmp == NULL)
    {
        piclib_mem_free(databuf);
        return PIC_MEM_ERR;
    }

#else
    databuf = (uint16_t *)bmpreadbuf;
    f_bmp = &f_bfile;
#endif
    bmpheadsize = sizeof(hbmp);
    my_mem_set((uint8_t *)&hbmp, 0, sizeof(hbmp));
    hbmp.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    hbmp.bmiHeader.biWidth = width;
    hbmp.bmiHeader.biHeight = height;
    hbmp.bmiHeader.biPlanes = 1;
    hbmp.bmiHeader.biBitCount = 16;
    hbmp.bmiHeader.biCompression = BI_BITFIELDS;
    hbmp.bmiHeader.biSizeImage = hbmp.bmiHeader.biHeight * hbmp.bmiHeader.biWidth * hbmp.bmiHeader.biBitCount / 8;

    hbmp.bmfHeader.bfType = ((uint16_t)'M' << 8) + 'B';
    hbmp.bmfHeader.bfSize = bmpheadsize + hbmp.bmiHeader.biSizeImage;
    hbmp.bmfHeader.bfOffBits = bmpheadsize;

    hbmp.RGB_MASK[0] = 0X00F800;
    hbmp.RGB_MASK[1] = 0X0007E0;
    hbmp.RGB_MASK[2] = 0X00001F;

    if (mode == 1)
    {
        res = f_open(f_bmp, (const TCHAR *)filename, FA_READ | FA_WRITE);
    }

    if (mode == 0 || res == 0x04)
    {
        res = f_open(f_bmp, (const TCHAR *)filename, FA_WRITE | FA_CREATE_NEW);
    }

    if ((hbmp.bmiHeader.biWidth * 2) % 4)
    {
        bi4width = ((hbmp.bmiHeader.biWidth * 2) / 4 + 1) * 4;
    }
    else
    {
        bi4width = hbmp.bmiHeader.biWidth * 2;
    }

    if (res == FR_OK)
    {
        res = f_write(f_bmp, (uint8_t *)&hbmp, bmpheadsize, &bw);

        for (ty = y + height - 1; hbmp.bmiHeader.biHeight; ty--)
        {
            pixcnt = 0;

            for (tx = x; pixcnt != (bi4width / 2);)
            {
                if (pixcnt < hbmp.bmiHeader.biWidth)
                {
                    databuf[pixcnt] = pic_phy.read_point(tx, ty);
                }
                else
                {
                    databuf[pixcnt] = 0Xffff;
                }

                pixcnt++;
                tx++;
            }

            hbmp.bmiHeader.biHeight--;
            res = f_write(f_bmp, (uint8_t *)databuf, bi4width, &bw);
        }

        f_close(f_bmp);
    }

#if BMP_USE_MALLOC == 1
    piclib_mem_free(databuf);
    piclib_mem_free(f_bmp);
#endif
    return res;
}
