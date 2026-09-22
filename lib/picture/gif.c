/**
 * @file    gif.c
 * @brief   GIF decoder, ported from the ALIENTEK PICTURE middleware to the RGB
 *          panel API.
 */

#include "gif.h"
#include "piclib.h"
#include "ff.h"
#include "malloc.h"
#include "delay.h"

const uint16_t _aMaskTbl[16] =
{
    0x0000, 0x0001, 0x0003, 0x0007,
    0x000f, 0x001f, 0x003f, 0x007f,
    0x00ff, 0x01ff, 0x03ff, 0x07ff,
    0x0fff, 0x1fff, 0x3fff, 0x7fff,
};
const uint8_t _aInterlaceOffset[] = {8, 8, 4, 2};
const uint8_t _aInterlaceYPos  [] = {0, 4, 2, 1};

uint8_t g_gif_decoding = 0;

#if GIF_USE_MALLOC == 0
gif89a tgif89a;
FIL f_gfile;
LZW_INFO tlzw;
#endif

static uint8_t gif_check_head(FIL *filename)
{
    uint8_t gifversion[6];
    uint32_t readed;
    uint8_t res;
    res = f_read(filename, gifversion, 6, (UINT *)&readed);

    if (res)return 1;

    if ((gifversion[0] != 'G') || (gifversion[1] != 'I') || (gifversion[2] != 'F') ||
        (gifversion[3] != '8') || ((gifversion[4] != '7') && (gifversion[4] != '9')) ||
        (gifversion[5] != 'a'))
    {
        return 2;
    }
    else
    {
        return 0;
    }
}

static uint16_t gif_getrgb565(uint8_t *ctb)
{
    uint16_t r, g, b;
    r = (ctb[0] >> 3) & 0X1F;
    g = (ctb[1] >> 2) & 0X3F;
    b = (ctb[2] >> 3) & 0X1F;
    return b + (g << 5) + (r << 11);
}

static uint8_t gif_readcolortbl(FIL *filename, gif89a *gif, uint16_t numcolors)
{
    uint8_t rgb[3];
    uint16_t t;
    uint8_t res;
    uint32_t readed;

    for (t = 0; t < numcolors; t++)
    {
        res = f_read(filename, rgb, 3, (UINT *)&readed);

        if (res)return 1;

        gif->colortbl[t] = gif_getrgb565(rgb);
    }

    return 0;
}

uint8_t gif_getinfo(void *file, gif89a *gif)
{
    FIL     *fil = (FIL *)file;
    uint32_t readed;
    uint8_t  res;
    res = f_read(fil, (uint8_t *)&gif->gifLSD, 7, (UINT *)&readed);

    if (res)return 1;

    if (gif->gifLSD.flag & 0x80)
    {
        gif->numcolors = 2 << (gif->gifLSD.flag & 0x07);

        if (gif_readcolortbl(fil, gif, gif->numcolors))
        {
            return 1;
        }
    }

    return 0;
}

static void gif_savegctbl(gif89a *gif)
{
    uint16_t i = 0;

    for (i = 0; i < 256; i++)
    {
        gif->bkpcolortbl[i] = gif->colortbl[i];
    }
}

static void gif_recovergctbl(gif89a *gif)
{
    uint16_t i = 0;

    for (i = 0; i < 256; i++)
    {
        gif->colortbl[i] = gif->bkpcolortbl[i];
    }
}

static void gif_initlzw(gif89a *gif, uint8_t codesize)
{
    my_mem_set((uint8_t *)gif->lzw, 0, sizeof(LZW_INFO));
    gif->lzw->SetCodeSize  = codesize;
    gif->lzw->CodeSize     = codesize + 1;
    gif->lzw->ClearCode    = (1 << codesize);
    gif->lzw->EndCode      = (1 << codesize) + 1;
    gif->lzw->MaxCode      = (1 << codesize) + 2;
    gif->lzw->MaxCodeSize  = (1 << codesize) << 1;
    gif->lzw->ReturnClear  = 1;
    gif->lzw->LastByte     = 2;
    gif->lzw->sp           = gif->lzw->aDecompBuffer;
}

static uint16_t gif_getdatablock(FIL *filename, uint8_t *buf, uint16_t maxnum)
{
    uint8_t cnt;
    uint32_t readed;
    uint32_t fpos;
    f_read(filename, &cnt, 1, (UINT *)&readed);

    if (cnt)
    {
        if (buf)
        {
            if (cnt > maxnum)
            {
                fpos = f_tell(filename);
                f_lseek(filename, fpos + cnt);
                return cnt;
            }

            f_read(filename, buf, cnt, (UINT *)&readed);
        }
        else
        {
            fpos = f_tell(filename);
            f_lseek(filename, fpos + cnt);
        }
    }

    return cnt;
}

static uint8_t gif_readextension(FIL *filename, gif89a *gif, int *p_transIndex, uint8_t *pdisposal)
{
    uint8_t temp;
    uint32_t readed;
    uint8_t buf[4];
    f_read(filename, &temp, 1, (UINT *)&readed);

    switch (temp)
    {
        case GIF_PLAINTEXT:
        case GIF_APPLICATION:
        case GIF_COMMENT:
            while (gif_getdatablock(filename, 0, 256) > 0);

            return 0;

        case GIF_GRAPHICCTL:
            if (gif_getdatablock(filename, buf, 4) != 4)return 1;

            gif->delay = (buf[2] << 8) | buf[1];
            *pdisposal = (buf[0] >> 2) & 0x7;

            if ((buf[0] & 0x1) != 0)*p_transIndex = buf[3];

            f_read(filename, &temp, 1, (UINT *)&readed);

            if (temp != 0)return 1;

            return 0;
    }

    return 1;
}

static int gif_getnextcode(FIL *filename, gif89a *gif)
{
    int i, j, end;
    long result;

    if (gif->lzw->ReturnClear)
    {

        gif->lzw->ReturnClear = 0;
        return gif->lzw->ClearCode;
    }

    end = gif->lzw->CurBit + gif->lzw->CodeSize;

    if (end >= gif->lzw->LastBit)
    {
        int count;

        if (gif->lzw->GetDone)return -1;

        gif->lzw->aBuffer[0] = gif->lzw->aBuffer[gif->lzw->LastByte - 2];
        gif->lzw->aBuffer[1] = gif->lzw->aBuffer[gif->lzw->LastByte - 1];

        if ((count = gif_getdatablock(filename, &gif->lzw->aBuffer[2], 300)) == 0)gif->lzw->GetDone = 1;

        if (count < 0)return -1;

        gif->lzw->LastByte = 2 + count;
        gif->lzw->CurBit = (gif->lzw->CurBit - gif->lzw->LastBit) + 16;
        gif->lzw->LastBit = (2 + count) * 8;
        end = gif->lzw->CurBit + gif->lzw->CodeSize;
    }

    j = end >> 3;
    i = gif->lzw->CurBit >> 3;

    if (i == j)result = (long)gif->lzw->aBuffer[i];
    else if (i + 1 == j)result = (long)gif->lzw->aBuffer[i] | ((long)gif->lzw->aBuffer[i + 1] << 8);
    else result = (long)gif->lzw->aBuffer[i] | ((long)gif->lzw->aBuffer[i + 1] << 8) | ((long)gif->lzw->aBuffer[i + 2] << 16);

    result = (result >> (gif->lzw->CurBit & 0x7))&_aMaskTbl[gif->lzw->CodeSize];
    gif->lzw->CurBit += gif->lzw->CodeSize;
    return (int)result;
}

static int gif_getnextbyte(FIL *filename, gif89a *gif)
{
    int i, code, incode;

    while ((code = gif_getnextcode(filename, gif)) >= 0)
    {
        if (code == gif->lzw->ClearCode)
        {

            if (gif->lzw->ClearCode >= (1 << MAX_NUM_LWZ_BITS))return -1;

            my_mem_set((uint8_t *)gif->lzw->aCode, 0, sizeof(gif->lzw->aCode));

            for (i = 0; i < gif->lzw->ClearCode; ++i)gif->lzw->aPrefix[i] = i;

            gif->lzw->CodeSize = gif->lzw->SetCodeSize + 1;
            gif->lzw->MaxCodeSize = gif->lzw->ClearCode << 1;
            gif->lzw->MaxCode = gif->lzw->ClearCode + 2;
            gif->lzw->sp = gif->lzw->aDecompBuffer;

            do
            {
                gif->lzw->FirstCode = gif_getnextcode(filename, gif);
            } while (gif->lzw->FirstCode == gif->lzw->ClearCode);

            gif->lzw->OldCode = gif->lzw->FirstCode;
            return gif->lzw->FirstCode;
        }

        if (code == gif->lzw->EndCode)return -2;

        incode = code;

        if (code >= gif->lzw->MaxCode)
        {
            *(gif->lzw->sp)++ = gif->lzw->FirstCode;
            code = gif->lzw->OldCode;
        }

        while (code >= gif->lzw->ClearCode)
        {
            *(gif->lzw->sp)++ = gif->lzw->aPrefix[code];

            if (code == gif->lzw->aCode[code])return code;

            if ((gif->lzw->sp - gif->lzw->aDecompBuffer) >= sizeof(gif->lzw->aDecompBuffer))return code;

            code = gif->lzw->aCode[code];
        }

        *(gif->lzw->sp)++ = gif->lzw->FirstCode = gif->lzw->aPrefix[code];

        if ((code = gif->lzw->MaxCode) < (1 << MAX_NUM_LWZ_BITS))
        {
            gif->lzw->aCode[code] = gif->lzw->OldCode;
            gif->lzw->aPrefix[code] = gif->lzw->FirstCode;
            ++gif->lzw->MaxCode;

            if ((gif->lzw->MaxCode >= gif->lzw->MaxCodeSize) && (gif->lzw->MaxCodeSize < (1 << MAX_NUM_LWZ_BITS)))
            {
                gif->lzw->MaxCodeSize <<= 1;
                ++gif->lzw->CodeSize;
            }
        }

        gif->lzw->OldCode = incode;

        if (gif->lzw->sp > gif->lzw->aDecompBuffer)return *--(gif->lzw->sp);
    }

    return code;
}

static uint8_t gif_dispimage(FIL *filename, gif89a *gif, uint16_t x0, uint16_t y0, int transparency, uint8_t disposal)
{
    uint32_t readed;
    uint8_t lzwlen;
    int index, oldindex, xpos, ypos, ycnt, pass, interlace, xend;
    int width, height, cnt, colorindex;
    uint16_t bkcolor;
    uint16_t *pTrans;

    width = gif->gifISD.width;
    height = gif->gifISD.height;
    xend = width + x0 - 1;
    bkcolor = gif->colortbl[gif->gifLSD.bkcindex];
    pTrans = (uint16_t *)gif->colortbl;
    f_read(filename, &lzwlen, 1, (UINT *)&readed);
    gif_initlzw(gif, lzwlen);
    interlace = gif->gifISD.flag & 0x40;

    for (ycnt = 0, ypos = y0, pass = 0; ycnt < height; ycnt++)
    {
        cnt = 0;
        oldindex = -1;

        for (xpos = x0; xpos <= xend; xpos++)
        {
            if (gif->lzw->sp > gif->lzw->aDecompBuffer)
            {
                index = *--(gif->lzw->sp);
            }
            else
            {
                index = gif_getnextbyte(filename, gif);
            }

            if (index == -2)return 0;

            if ((index < 0) || (index >= gif->numcolors))
            {

                return 1;
            }

            if ((index == oldindex) && (xpos <= xend))
            {
                cnt++;
            }
            else
            {
                if (cnt)
                {
                    if (oldindex != transparency)
                    {
                        pic_phy.draw_hline(xpos - cnt - 1, ypos, cnt + 1, *(pTrans + oldindex));
                    }
                    else if (disposal == 2)
                    {
                        pic_phy.draw_hline(xpos - cnt - 1, ypos, cnt + 1, bkcolor);
                    }

                    cnt = 0;
                }
                else
                {
                    if (oldindex >= 0)
                    {
                        if (oldindex != transparency)
                        {
                            pic_phy.draw_point(xpos - 1, ypos, *(pTrans + oldindex));
                        }
                        else if (disposal == 2)
                        {
                            pic_phy.draw_point(xpos - 1, ypos, bkcolor);
                        }
                    }
                }
            }

            oldindex = index;
        }

        if ((oldindex != transparency) || (disposal == 2))
        {
            if (oldindex != transparency)colorindex = *(pTrans + oldindex);
            else colorindex = bkcolor;

            if (cnt)
            {
                pic_phy.draw_hline(xpos - cnt - 1, ypos, cnt + 1, colorindex);
            }
            else
            {
                pic_phy.draw_point(xend, ypos, colorindex);
            }
        }

        if (interlace)
        {
            ypos += _aInterlaceOffset[pass];

            if ((ypos - y0) >= height)
            {
                ++pass;
                ypos = _aInterlaceYPos[pass] + y0;
            }
        }
        else
        {
            ypos++;
        }
    }

    return 0;
}

static void gif_clear2bkcolor(uint16_t x, uint16_t y, gif89a *gif, ImageScreenDescriptor pimge)
{
    uint16_t x0, y0, x1, y1;
    uint16_t color = gif->colortbl[gif->gifLSD.bkcindex];

    if (pimge.width == 0 || pimge.height == 0)return;

    if (gif->gifISD.yoff > pimge.yoff)
    {
        x0 = x + pimge.xoff;
        y0 = y + pimge.yoff;
        x1 = x + pimge.xoff + pimge.width - 1;
        y1 = y + gif->gifISD.yoff - 1;

        if (x0 < x1 && y0 < y1 && x1 < 320 && y1 < 320)
        {
            pic_phy.fill(x0, y0, x1, y1, color);
        }
    }

    if (gif->gifISD.xoff > pimge.xoff)
    {
        x0 = x + pimge.xoff;
        y0 = y + pimge.yoff;
        x1 = x + gif->gifISD.xoff - 1;
        y1 = y + pimge.yoff + pimge.height - 1;

        if (x0 < x1 && y0 < y1 && x1 < 320 && y1 < 320)
        {
            pic_phy.fill(x0, y0, x1, y1, color);
        }
    }

    if ((gif->gifISD.yoff + gif->gifISD.height) < (pimge.yoff + pimge.height))
    {
        x0 = x + pimge.xoff;
        y0 = y + gif->gifISD.yoff + gif->gifISD.height - 1;
        x1 = x + pimge.xoff + pimge.width - 1;;
        y1 = y + pimge.yoff + pimge.height - 1;

        if (x0 < x1 && y0 < y1 && x1 < 320 && y1 < 320)
        {
            pic_phy.fill(x0, y0, x1, y1, color);
        }
    }

    if ((gif->gifISD.xoff + gif->gifISD.width) < (pimge.xoff + pimge.width))
    {
        x0 = x + gif->gifISD.xoff + gif->gifISD.width - 1;
        y0 = y + pimge.yoff;
        x1 = x + pimge.xoff + pimge.width - 1;;
        y1 = y + pimge.yoff + pimge.height - 1;

        if (x0 < x1 && y0 < y1 && x1 < 320 && y1 < 320)
        {
            pic_phy.fill(x0, y0, x1, y1, color);
        }
    }
}

static uint8_t gif_drawimage(FIL *filename, gif89a *gif, uint16_t x0, uint16_t y0)
{
    uint32_t readed;
    uint8_t res, temp;
    uint16_t numcolors;
    ImageScreenDescriptor previmg;

    uint8_t disposal;
    int transindex;
    uint8_t introducer;
    transindex = -1;

    do
    {
        res = f_read(filename, &introducer, 1, (UINT *)&readed);

        if (res)return 1;

        switch (introducer)
        {
            case GIF_INTRO_IMAGE:
                previmg.xoff = gif->gifISD.xoff;
                previmg.yoff = gif->gifISD.yoff;
                previmg.width = gif->gifISD.width;
                previmg.height = gif->gifISD.height;

                res = f_read(filename, (uint8_t *)&gif->gifISD, 9, (UINT *)&readed);

                if (res)return 1;

                if (gif->gifISD.flag & 0x80)
                {
                    gif_savegctbl(gif);
                    numcolors = 2 << (gif->gifISD.flag & 0X07);

                    if (gif_readcolortbl(filename, gif, numcolors))return 1;
                }

                if (disposal == 2)gif_clear2bkcolor(x0, y0, gif, previmg);

                gif_dispimage(filename, gif, x0 + gif->gifISD.xoff, y0 + gif->gifISD.yoff, transindex, disposal);

                while (1)
                {
                    f_read(filename, &temp, 1, (UINT *)&readed);

                    if (temp == 0)break;

                    readed = f_tell(filename);

                    if (f_lseek(filename, readed + temp))break;
                }

                if (temp != 0)return 1;

                return 0;

            case GIF_INTRO_TERMINATOR:
                return 2;

            case GIF_INTRO_EXTENSION:

                res = gif_readextension(filename, gif, &transindex, &disposal);

                if (res)return 1;

                break;

            default:
                return 1;
        }
    } while (introducer != GIF_INTRO_TERMINATOR);

    return 0;
}

void gif_quit(void)
{
    g_gif_decoding = 0;
}

uint8_t gif_decode(const char *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    uint8_t res = 0;
    uint16_t dtime = 0;
    gif89a *mygif89a;
    FIL *gfile;

#if GIF_USE_MALLOC == 1
    gfile = (FIL *)piclib_mem_malloc(sizeof(FIL));

    if (gfile == NULL)res = PIC_MEM_ERR;

    mygif89a = (gif89a *)piclib_mem_malloc(sizeof(gif89a));

    if (mygif89a == NULL)res = PIC_MEM_ERR;

    mygif89a->lzw = (LZW_INFO *)piclib_mem_malloc(sizeof(LZW_INFO));

    if (mygif89a->lzw == NULL)res = PIC_MEM_ERR;

#else
    gfile = &f_gfile;
    mygif89a = &tgif89a;
    mygif89a->lzw = &tlzw;
#endif

    if (res == 0)
    {
        res = f_open(gfile, (TCHAR *)filename, FA_READ);

        if (res == 0)
        {
            if (gif_check_head(gfile))res = PIC_FORMAT_ERR;

            if (gif_getinfo(gfile, mygif89a))res = PIC_FORMAT_ERR;

            if (mygif89a->gifLSD.width > width || mygif89a->gifLSD.height > height)res = PIC_SIZE_ERR;
            else
            {
                x = (width - mygif89a->gifLSD.width) / 2 + x;
                y = (height - mygif89a->gifLSD.height) / 2 + y;
            }

            g_gif_decoding = 1;

            while (g_gif_decoding && res == 0)
            {
                res = gif_drawimage(gfile, mygif89a, x, y);

                if (mygif89a->gifISD.flag & 0x80)gif_recovergctbl(mygif89a);

                if (mygif89a->delay)
                {
                    dtime = mygif89a->delay;
                }
                else
                {
                    dtime = 10;
                }

                while (dtime-- && g_gif_decoding)
                {
                    delay_ms(10);
                }

                if (res == 2)
                {
                    res = 0;
                    break;
                }
            }
        }

        f_close(gfile);
    }

#if GIF_USE_MALLOC == 1
    piclib_mem_free(gfile);
    piclib_mem_free(mygif89a->lzw);
    piclib_mem_free(mygif89a);
#endif
    return res;
}
