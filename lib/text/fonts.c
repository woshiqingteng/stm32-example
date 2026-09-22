/**
 * @file    fonts.c
 * @brief   GBK font store in the on-board SPI NOR flash, ported from the
 *          ALIENTEK TEXT middleware. The font set is copied from a source drive
 *          (SD card, "0:") into the flash and its layout descriptor is stored at
 *          FONTINFOADDR.
 */

#include <string.h>
#include <stdint.h>
#include "fonts.h"
#include "lcd.h"
#include "malloc.h"
#include "ff.h"
#include "delay.h"
#include "norflash.h"

/* 4 fonts + unigbk + descriptor: about 6.01 MB, 1539 4 KB sectors. */
#define FONTSECSIZE         1539U

/* Font store base offset inside the NOR flash (past the filesystem area). */
#define FONTINFOADDR        (25UL * 1024UL * 1024UL)

_font_info ftinfo;

char *const FONT_GBK_PATH[5] =
{
    "/SYSTEM/FONT/UNIGBK.BIN",
    "/SYSTEM/FONT/GBK12.FON",
    "/SYSTEM/FONT/GBK16.FON",
    "/SYSTEM/FONT/GBK24.FON",
    "/SYSTEM/FONT/GBK32.FON",
};

char *const FONT_UPDATE_REMIND_TBL[5] =
{
    "Updating UNIGBK.BIN",
    "Updating GBK12.FON ",
    "Updating GBK16.FON ",
    "Updating GBK24.FON ",
    "Updating GBK32.FON ",
};

static void fonts_progress_show(uint16_t x, uint16_t y, uint8_t size, uint32_t totsize, uint32_t pos, uint16_t color)
{
    float prog;
    uint8_t t = 0xFF;

    prog = (float)pos / totsize;
    prog *= 100;

    if (t != prog)
    {
        lcd_show_string((uint16_t)(x + 3 * size / 2), y, 240U, 320U, (lcd_font_size_t)size, "%", color);
        t = (uint8_t)prog;

        if (t > 100U)
        {
            t = 100U;
        }

        lcd_show_num(x, y, t, 3U, (lcd_font_size_t)size, color);
    }
}

static uint8_t fonts_update_fontx(uint16_t x, uint16_t y, uint8_t size, uint8_t *fpath, uint8_t fx, uint16_t color)
{
    uint32_t flashaddr = 0;
    FIL *fftemp;
    uint8_t *tempbuf;
    uint8_t res;
    UINT bread;
    uint32_t offx = 0;
    uint8_t rval = 0;

    fftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));

    if (fftemp == 0)
    {
        rval = 1;
    }

    tempbuf = mymalloc(SRAMIN, 4096);

    if (tempbuf == 0)
    {
        rval = 1;
    }

    res = f_open(fftemp, (const TCHAR *)fpath, FA_READ);

    if (res != FR_OK)
    {
        rval = 2;
    }

    if (rval == 0)
    {
        switch (fx)
        {
            case 0:
                ftinfo.ugbkaddr = FONTINFOADDR + sizeof(ftinfo);
                ftinfo.ugbksize = (uint32_t)fftemp->obj.objsize;
                flashaddr = ftinfo.ugbkaddr;
                break;

            case 1:
                ftinfo.f12addr = ftinfo.ugbkaddr + ftinfo.ugbksize;
                ftinfo.gbk12size = (uint32_t)fftemp->obj.objsize;
                flashaddr = ftinfo.f12addr;
                break;

            case 2:
                ftinfo.f16addr = ftinfo.f12addr + ftinfo.gbk12size;
                ftinfo.gbk16size = (uint32_t)fftemp->obj.objsize;
                flashaddr = ftinfo.f16addr;
                break;

            case 3:
                ftinfo.f24addr = ftinfo.f16addr + ftinfo.gbk16size;
                ftinfo.gbk24size = (uint32_t)fftemp->obj.objsize;
                flashaddr = ftinfo.f24addr;
                break;

            case 4:
                ftinfo.f32addr = ftinfo.f24addr + ftinfo.gbk24size;
                ftinfo.gbk32size = (uint32_t)fftemp->obj.objsize;
                flashaddr = ftinfo.f32addr;
                break;

            default:
                break;
        }

        while (res == FR_OK)
        {
            res = f_read(fftemp, tempbuf, 4096U, &bread);

            if (res != FR_OK)
            {
                break;
            }

            norflash_write(tempbuf, offx + flashaddr, (uint16_t)bread);
            offx += bread;
            fonts_progress_show(x, y, size, (uint32_t)fftemp->obj.objsize, offx, color);

            if (bread != 4096U)
            {
                break;
            }
        }

        (void)f_close(fftemp);
    }

    myfree(SRAMIN, fftemp);
    myfree(SRAMIN, tempbuf);

    return res;
}

uint8_t fonts_update_font(uint16_t x, uint16_t y, uint8_t size, uint8_t *src, uint16_t color)
{
    uint8_t *pname;
    uint8_t *buf;
    uint8_t res = 0;
    uint16_t i;
    uint32_t j;
    FIL *fftemp;
    uint8_t rval = 0;

    res = 0xFF;
    ftinfo.fontok = 0xFF;
    pname = mymalloc(SRAMIN, 100);
    buf = mymalloc(SRAMIN, 4096);
    fftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));

    if (buf == 0 || pname == 0 || fftemp == 0)
    {
        myfree(SRAMIN, fftemp);
        myfree(SRAMIN, pname);
        myfree(SRAMIN, buf);
        return 5;
    }

    for (i = 0; i < 5U; i++)
    {
        strcpy((char *)pname, (char *)src);
        strcat((char *)pname, (char *)FONT_GBK_PATH[i]);
        res = f_open(fftemp, (const TCHAR *)pname, FA_READ);

        if (res != FR_OK)
        {
            rval |= 1U << 7;
            break;
        }
    }

    myfree(SRAMIN, fftemp);

    if (rval == 0)
    {
        lcd_show_string(x, y, 240U, 320U, (lcd_font_size_t)size, "Erasing sectors... ", color);

        for (i = 0; i < FONTSECSIZE; i++)
        {
            fonts_progress_show((uint16_t)(x + 20 * size / 2), y, size, FONTSECSIZE, i, color);
            norflash_read(buf, (uint32_t)((FONTINFOADDR / 4096U) + i) * 4096U, 4096U);

            for (j = 0; j < 1024U; j++)
            {
                if (buf[j] != 0xFFU)
                {
                    break;
                }
            }

            if (j != 1024U)
            {
                norflash_erase_sector((uint32_t)((FONTINFOADDR / 4096U) + i));
            }
        }

        for (i = 0; i < 5U; i++)
        {
            lcd_show_string(x, y, 240U, 320U, (lcd_font_size_t)size, FONT_UPDATE_REMIND_TBL[i], color);
            strcpy((char *)pname, (char *)src);
            strcat((char *)pname, (char *)FONT_GBK_PATH[i]);
            res = fonts_update_fontx((uint16_t)(x + 20 * size / 2), y, size, pname, (uint8_t)i, color);

            if (res != 0)
            {
                myfree(SRAMIN, buf);
                myfree(SRAMIN, pname);
                return (uint8_t)(1U + i);
            }
        }

        ftinfo.fontok = 0xAA;
        norflash_write((uint8_t *)&ftinfo, FONTINFOADDR, (uint16_t)sizeof(ftinfo));
    }

    myfree(SRAMIN, pname);
    myfree(SRAMIN, buf);

    return rval;
}

uint8_t fonts_init(void)
{
    uint8_t t = 0;

    while (t < 10U)
    {
        t++;
        norflash_read((uint8_t *)&ftinfo, FONTINFOADDR, (uint16_t)sizeof(ftinfo));

        if (ftinfo.fontok == 0xAA)
        {
            break;
        }

        delay_ms(20);
    }

    if (ftinfo.fontok != 0xAA)
    {
        return 1;
    }

    return 0;
}
