/**
 * @file    image.c
 * @brief   SPI-NOR image library: update and init.
 */

#include "string.h" 
#include "image.h"
#include "lcd.h"
#include "malloc.h"
#include "ff.h"
#include "usart.h"
#include "delay.h"
#include "nor.h"


/* sectors used by the image library (4 KB per sector) */
#define IMAGESECSIZE         NOR_IMAGE_SECTORS


/* image library info (IMAGEINFOADDR is defined in image.h) */

/* descriptor: entry address and size of the whole image library */
_image_info g_ftinfo;

/* image library paths on the SD card */
char *const CONFIG_IMAGE_GBK_PATH[8] =
{
    "/PICTURE/LVGLBIN/Calculator.bin",
    "/PICTURE/LVGLBIN/File.bin",
    "/PICTURE/LVGLBIN/lv_system.bin",
    "/PICTURE/LVGLBIN/Setting.bin",
    "/PICTURE/LVGLBIN/Test.bin",
    "/PICTURE/LVGLBIN/Timer.bin",
    "/PICTURE/LVGLBIN/lv_qr.bin",
    "/PICTURE/LVGLBIN/lv_draw.bin",
};

/* update progress messages */
char *const IMAGE_UPDATE_REMIND_TBL[8] =
{
    "Updating Calculator.BIN",
    "Updating File.BIN",
    "Updating lv_system.BIN",
    "Updating Setting.BIN",
    "Updating Test.BIN",
    "Updating Timer.BIN",
    "Updating lv_qr.BIN",
    "Updating lv_draw.BIN",
};

#define CONFIG_IMAGE_GBK_NUM           (int)(sizeof(CONFIG_IMAGE_GBK_PATH)/sizeof(CONFIG_IMAGE_GBK_PATH[0]))
#define IMAGE_UPDATE_REMIND_NUM (int)(sizeof(IMAGE_UPDATE_REMIND_TBL)/sizeof(IMAGE_UPDATE_REMIND_TBL[0]))
/* Show the image-library update progress. */
static void images_progress_show(uint16_t x, uint16_t y, uint8_t size, uint32_t totsize, uint32_t pos, uint16_t color)
{
    float prog;
    uint8_t t = 0XFF;
    prog = (float)pos / totsize;
    prog *= 100;

    if (t != prog)
    {
        lcd_show_string(x + 3 * size / 2, y, 240, 320, size, "%", color);
        t = prog;

        if (t > 100)t = 100;

        lcd_show_num(x, y, t, 3, size, color);  /* show value */
    }
}

/* Update one image: display position/size, path, index and color.
 * Clears the caller's per-image info and chains the addresses. */
static uint8_t images_update_imagex(uint16_t x, uint16_t y, uint8_t size, uint8_t *fpath, uint8_t fx, uint16_t color)
{
    uint32_t flashaddr = 0;
    FIL *fftemp;
    uint8_t *tempbuf;
    uint8_t res;
    uint16_t bread;
    uint32_t offx = 0;
    uint8_t rval = 0;
    fftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));  /* allocate memory */

    if (fftemp == NULL)rval = 1;

    tempbuf = mymalloc(SRAMIN, 4096);               /* 4096-byte buffer */

    if (tempbuf == NULL)rval = 1;

    res = f_open(fftemp, (const TCHAR *)fpath, FA_READ);

    if (res)rval = 2;   /* open failed */

    if (rval == 0)
    {
        switch (fx)
        {
            case 0: /* atk01.BIN */
                g_ftinfo.lvgl_atk01addr = IMAGEINFOADDR + sizeof(g_ftinfo);                     /* after the header */
                g_ftinfo.lvgl_atk01size = fftemp->obj.objsize;                                  /* atk01 size */
                flashaddr = g_ftinfo.lvgl_atk01addr;
                break;
            case 1: /* atk02.BIN */
                g_ftinfo.lvgl_atk02addr = g_ftinfo.lvgl_atk01addr + g_ftinfo.lvgl_atk01size;    /* chain */
                g_ftinfo.lvgl_atk02size = fftemp->obj.objsize;                                  /* atk02 size */
                flashaddr = g_ftinfo.lvgl_atk02addr;
                break;
            case 2: /* atk03.BIN */
                g_ftinfo.lvgl_atk03addr = g_ftinfo.lvgl_atk02addr + g_ftinfo.lvgl_atk02size;    /* chain */
                g_ftinfo.lvgl_atk03size = fftemp->obj.objsize;                                  /* atk03 size */
                flashaddr = g_ftinfo.lvgl_atk03addr;
                break;
            case 3: /* atk04.BIN */
                g_ftinfo.lvgl_atk04addr = g_ftinfo.lvgl_atk03addr + g_ftinfo.lvgl_atk03size;    /* chain */
                g_ftinfo.lvgl_atk04size = fftemp->obj.objsize;                                  /* atk04 size */
                flashaddr = g_ftinfo.lvgl_atk04addr;
                break;
            case 4: /* atk05.BIN */
                g_ftinfo.lvgl_atk05addr = g_ftinfo.lvgl_atk04addr + g_ftinfo.lvgl_atk04size;    /* chain */
                g_ftinfo.lvgl_atk05size = fftemp->obj.objsize;                                  /* atk05 size */
                flashaddr = g_ftinfo.lvgl_atk05addr;
                break;
            case 5: /* atk06.BIN */
                g_ftinfo.lvgl_atk06addr = g_ftinfo.lvgl_atk05addr + g_ftinfo.lvgl_atk05size;    /* chain */
                g_ftinfo.lvgl_atk06size = fftemp->obj.objsize;                                  /* atk06 size */
                flashaddr = g_ftinfo.lvgl_atk06addr;
                break;
            case 6: /* atk07.BIN */
                g_ftinfo.lvgl_atk07addr = g_ftinfo.lvgl_atk06addr + g_ftinfo.lvgl_atk06size;    /* chain */
                g_ftinfo.lvgl_atk07size = fftemp->obj.objsize;                                  /* atk07 size */
                flashaddr = g_ftinfo.lvgl_atk07addr;
                break;
            case 7: /* atk08.BIN */
                g_ftinfo.lvgl_atk08addr = g_ftinfo.lvgl_atk07addr + g_ftinfo.lvgl_atk07size;    /* chain */
                g_ftinfo.lvgl_atk08size = fftemp->obj.objsize;                                  /* atk08 size */
                flashaddr = g_ftinfo.lvgl_atk08addr;
                break;
        }

        while (res == FR_OK)   /* loop */
        {
            res = f_read(fftemp, tempbuf, 4096, (UINT *)&bread);    /* read */

            if (res != FR_OK)break;     /* error */

            nor_write(tempbuf, offx + flashaddr, bread);    /* write bread bytes from offset 0 */
            offx += bread;
            images_progress_show(x, y, size, fftemp->obj.objsize, offx, color);    /* progress */

            if (bread != 4096)break;    /* last chunk */
        }

        f_close(fftemp);
    }

    myfree(SRAMIN, fftemp);     /* free memory */
    myfree(SRAMIN, tempbuf);    /* free memory */
    return res;
}

/* Update the whole image library from the given source driver ("0:" SD, ...).
 * Returns 0 on success, otherwise the failing index. */
uint8_t images_update_image(uint16_t x, uint16_t y, uint8_t size, uint8_t *src, uint16_t color)
{
    uint8_t *pname;
    uint32_t *buf;
    uint8_t res = 0;
    uint16_t i;
    FIL *fftemp;
    uint8_t rval = 0;
    res = 0XFF;
    g_ftinfo.imageok = 0XFF;
    pname = mymalloc(SRAMIN, 100);  /* 100-byte path buffer */
    buf = mymalloc(SRAMIN, 4096);   /* 4 KB buffer */
    fftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));  /* allocate memory */

    if (buf == NULL || pname == NULL || fftemp == NULL)
    {
        myfree(SRAMIN, fftemp);
        myfree(SRAMIN, pname);
        myfree(SRAMIN, buf);
        return 5;   /* allocation failed */
    }

    for (i = 0; i < CONFIG_IMAGE_GBK_NUM; i++) /* check all files are present */
    {
        strcpy((char *)pname, (char *)src);                  /* copy src */
        strcat((char *)pname, (char *)CONFIG_IMAGE_GBK_PATH[i]);    /* append path */
        res = f_open(fftemp, (const TCHAR *)pname, FA_READ); /* try to open */

        if (res)
        {
            rval |= 1 << 7; /* mark missing file */
            break;          /* stop */
        }
    }

    myfree(SRAMIN, fftemp); /* free memory */

    if (rval == 0)          /* all files found */
    {
        /* No pre-erase pass: nor_write() erases each sector on demand, and the
         * old scan erased past the 32 MB chip (addresses wrapped into the
         * FatFs area). */

        for (i = 0; i < IMAGE_UPDATE_REMIND_NUM; i++) /* update each image */
        {
            lcd_show_string(x, y, 240, 320, size, IMAGE_UPDATE_REMIND_TBL[i], color);
            strcpy((char *)pname, (char *)src);              /* copy src */
            strcat((char *)pname, (char *)CONFIG_IMAGE_GBK_PATH[i]);/* append path */
            res = images_update_imagex(x + 20 * size / 2, y, size, pname, i, color);    /* update */

            if (res)
            {
                myfree(SRAMIN, buf);
                myfree(SRAMIN, pname);
                return 1 + i;
            }
        }

        /* mark the library valid */
        g_ftinfo.imageok = 0XAA;
        nor_write((uint8_t *)&g_ftinfo, IMAGEINFOADDR, sizeof(g_ftinfo));    /* write descriptor */
    }

    myfree(SRAMIN, pname);  /* free memory */
    myfree(SRAMIN, buf);
    return rval;            /* no error */
}

/* Init the image library. Returns 0 if valid, otherwise 1. */
uint8_t images_init(void)
{
    uint8_t t = 0;

    nor_init();

    while (t < 10)  /* retry a few times */
    {
        t++;
        nor_read((uint8_t *)&g_ftinfo, IMAGEINFOADDR, sizeof(g_ftinfo)); /* read descriptor */

        if (g_ftinfo.imageok == 0XAA)
        {
            break;
        }
        
        delay_ms(20);
    }

    if (g_ftinfo.imageok != 0XAA)
    {
        return 1;
    }
    
    return 0;
}
