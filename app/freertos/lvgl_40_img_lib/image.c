/**
 ****************************************************************************************************
 * @file        image.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-04
 * @brief       ͼƬ�� ����
 *              �ṩimage_update_image��images_init����ͼƬ����ºͳ�ʼ��
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� Mini Pro H750������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20200404
 * ��һ�η���
 *
 ****************************************************************************************************
 */

#include "string.h" 
#include "image.h"
#include "lcd.h"
#include "lvgl.h"
#include "ff.h"
#include "usart.h"
#include "delay.h"
#include "nor.h"


/* ͼƬ������ռ�õ�����������С(4��ͼƬ��=70408�ֽ�,Լռ18��25QXX����,һ������4K�ֽ�) */
#define IMAGESECSIZE         18


/* ͼƬ������ʼ��ַ
 * �ӵ�18��������ʼ���ͼƬ��
 * ǰ��18��������code����spb��ռ����.
 * 25M���������4��ͼƬ��,��ͼƬ��ռ����,���ܶ�!
 */
/* Image store base in the SPI NOR: 31.5 MB, past the 25 MB font store. */
#define IMAGEINFOADDR        0x1F80000UL

/* ��������ͼƬ�������Ϣ����ַ����С�� */
_image_info g_ftinfo;

/* ͼƬ�����ڴ����е�·�� */
char *const IMAGE_GBK_PATH[4] =
{
    "/PICTURE/LVGLBIN/atk05.BIN",
    "/PICTURE/LVGLBIN/atk06.BIN",
    "/PICTURE/LVGLBIN/atk07.BIN",
    "/PICTURE/LVGLBIN/money.BIN",
};

/* ����ʱ����ʾ��Ϣ */
char *const IMAGE_UPDATE_REMIND_TBL[4] =
{
    "Updating atk05.BIN",
    "Updating atk06.BIN",
    "Updating atk07.BIN",
    "Updating money.BIN",
};

#define IMAGE_GBK_NUM           (int)(sizeof(IMAGE_GBK_PATH)/sizeof(IMAGE_GBK_PATH[0]))
#define IMAGE_UPDATE_REMIND_NUM (int)(sizeof(IMAGE_UPDATE_REMIND_TBL)/sizeof(IMAGE_UPDATE_REMIND_TBL[0]))
/**
 * @brief       ��ʾ��ǰͼƬ���½���
 * @param       x, y    : ����
 * @param       size    : ͼƬ��С
 * @param       totsize : �����ļ���С
 * @param       pos     : ��ǰ�ļ�ָ��λ��
 * @param       color   : ͼƬ��ɫ
 * @retval      ��
 */
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

        lcd_show_num(x, y, t, 3, size, color);  /* ��ʾ��ֵ */
    }
}

/**
 * @brief       ����ĳһ��ͼƬ��
 * @param       x, y    : ��ʾ��Ϣ����ʾ��ַ
 * @param       size    : ��ʾ��ϢͼƬ��С
 * @param       fpath   : ͼƬ·��
 * @param       fx      : ���µ�����
 *   @arg                 0, atk01;
 *   @Arg                 1, atk02;
 *   @arg                 2, atk03;
 *   @arg                 3, atk04;
 *   @arg                 4, atk05;
 * @param       color   : ͼƬ��ɫ
 * @retval      0, �ɹ�; ����, �������;
 */
static uint8_t images_update_imagex(uint16_t x, uint16_t y, uint8_t size, uint8_t *fpath, uint8_t fx, uint16_t color)
{
    uint32_t flashaddr = 0;
    FIL *fftemp;
    uint8_t *tempbuf;
    uint8_t res;
    uint16_t bread;
    uint32_t offx = 0;
    uint8_t rval = 0;
    fftemp = (FIL *)lv_mem_alloc( sizeof(FIL));  /* �����ڴ� */

    if (fftemp == NULL)rval = 1;

    tempbuf = lv_mem_alloc( 4096);               /* ����4096���ֽڿռ� */

    if (tempbuf == NULL)rval = 1;

    res = f_open(fftemp, (const TCHAR *)fpath, FA_READ);

    if (res)rval = 2;   /* ���ļ�ʧ�� */

    if (rval == 0)
    {
        switch (fx)
        {
            case 0: /* ����atk01.BIN */
                g_ftinfo.lvgl_atk01addr = IMAGEINFOADDR + sizeof(g_ftinfo);                  /* ��Ϣͷ֮�󣬽���atk02 */
                g_ftinfo.lvgl_atk01size = fftemp->obj.objsize;                             /* atk01��С */
                flashaddr = g_ftinfo.lvgl_atk01addr;
                break;
            case 1: /* ����atk02.BIN */
                g_ftinfo.lvgl_atk02addr = g_ftinfo.lvgl_atk01addr + g_ftinfo.lvgl_atk01size;    /* ��Ϣͷ֮�󣬽���atk03 */
                g_ftinfo.lvgl_atk02size = fftemp->obj.objsize;                              /* atk02��С */
                flashaddr = g_ftinfo.lvgl_atk02addr;
                break;
            case 2: /* ����atk03.BIN */
                g_ftinfo.lvgl_atk03addr = g_ftinfo.lvgl_atk02addr + g_ftinfo.lvgl_atk02size;    /* ��Ϣͷ֮�󣬽���money */
                g_ftinfo.lvgl_atk03size = fftemp->obj.objsize;                              /* atk03��С */
                flashaddr = g_ftinfo.lvgl_atk03addr;
                break;
            case 3: /* ����money.BIN */
                g_ftinfo.lvgl_moneyaddr = g_ftinfo.lvgl_atk03addr + g_ftinfo.lvgl_atk03size;    /* ��Ϣͷ֮�� */
                g_ftinfo.lvgl_moneysize = fftemp->obj.objsize;                              /* money��С */
                flashaddr = g_ftinfo.lvgl_moneyaddr;
                break;
        }

        while (res == FR_OK)   /* ��ѭ��ִ�� */
        {
            res = f_read(fftemp, tempbuf, 4096, (UINT *)&bread);    /* ��ȡ���� */

            if (res != FR_OK)break;     /* ִ�д��� */

            nor_write(tempbuf, offx + flashaddr, bread);    /* ��0��ʼд��bread������ */
            offx += bread;
            images_progress_show(x, y, size, fftemp->obj.objsize, offx, color);    /* ������ʾ */

            if (bread != 4096)break;    /* ������. */
        }

        f_close(fftemp);
    }

    lv_mem_free( fftemp);     /* �ͷ��ڴ� */
    lv_mem_free( tempbuf);    /* �ͷ��ڴ� */
    return res;
}

/**
 * @brief       ����ͼƬ�ļ�
 *   @note      ����ͼƬ��һ�����(UNIGBK,GBK12,GBK16,GBK24,GBK32)
 * @param       x, y    : ��ʾ��Ϣ����ʾ��ַ
 * @param       size    : ��ʾ��ϢͼƬ��С
 * @param       src     : ͼƬ����Դ����
 *   @arg                 "0:", SD��;
 *   @Arg                 "1:", FLASH��
 *   @arg                 "2:", U��
 * @param       color   : ͼƬ��ɫ
 * @retval      0, �ɹ�; ����, �������;
 */
uint8_t images_update_image(uint16_t x, uint16_t y, uint8_t size, uint8_t *src, uint16_t color)
{
    uint8_t *pname;
    uint32_t *buf;
    uint8_t res = 0;
    uint16_t i, j;
    FIL *fftemp;
    uint8_t rval = 0;
    res = 0XFF;
    g_ftinfo.imageok = 0XFF;
    pname = lv_mem_alloc( 100);  /* ����100�ֽ��ڴ� */
    buf = lv_mem_alloc( 4096);   /* ����4K�ֽ��ڴ� */
    fftemp = (FIL *)lv_mem_alloc( sizeof(FIL));  /* �����ڴ� */

    if (buf == NULL || pname == NULL || fftemp == NULL)
    {
        lv_mem_free( fftemp);
        lv_mem_free( pname);
        lv_mem_free( buf);
        return 5;   /* �ڴ�����ʧ�� */
    }

    for (i = 0; i < IMAGE_GBK_NUM; i++) /* �Ȳ����ļ�atk01,atk02,atk03,money�Ƿ����� */
    {
        strcpy((char *)pname, (char *)src);                  /* copy src���ݵ�pname */
        strcat((char *)pname, (char *)IMAGE_GBK_PATH[i]);    /* ׷�Ӿ����ļ�·�� */
        res = f_open(fftemp, (const TCHAR *)pname, FA_READ); /* ���Դ� */

        if (res)
        {
            rval |= 1 << 7; /* ��Ǵ��ļ�ʧ�� */
            break;          /* ������,ֱ���˳� */
        }
    }

    lv_mem_free( fftemp); /* �ͷ��ڴ� */

    if (rval == 0)          /* ͼƬ���ļ�������. */
    {
        lcd_show_string(x, y, 240, 320, size, "Erasing sectors... ", color);    /* ��ʾ���ڲ������� */

        for (i = 0; i < IMAGESECSIZE; i++)   /* �Ȳ���ͼƬ������,���д���ٶ� */
        {
            images_progress_show(x + 20 * size / 2, y, size, IMAGESECSIZE, i, color);    /* ������ʾ */
            nor_read((uint8_t *)buf, ((IMAGEINFOADDR / 4096) + i) * 4096, 4096); /* ������������������ */

            for (j = 0; j < 1024; j++)          /* У������ */
            {
                if (buf[j] != 0XFFFFFFFF)break; /* ��Ҫ���� */
            }

            if (j != 1024)
            {
                nor_erase_sector((IMAGEINFOADDR / 4096) + i); /* ��Ҫ���������� */
            }
        }

        for (i = 0; i < IMAGE_UPDATE_REMIND_NUM; i++) /* ���θ���atk01,atk02,atk03,money */
        {
            lcd_show_string(x, y, 240, 320, size, IMAGE_UPDATE_REMIND_TBL[i], color);
            strcpy((char *)pname, (char *)src);              /* copy src���ݵ�pname */
            strcat((char *)pname, (char *)IMAGE_GBK_PATH[i]);/* ׷�Ӿ����ļ�·�� */
            res = images_update_imagex(x + 20 * size / 2, y, size, pname, i, color);    /* ����ͼƬ�� */

            if (res)
            {
                lv_mem_free( buf);
                lv_mem_free( pname);
                return 1 + i;
            }
        }

        /* ȫ�����º��� */
        g_ftinfo.imageok = 0XAA;
        nor_write((uint8_t *)&g_ftinfo, IMAGEINFOADDR, sizeof(g_ftinfo));    /* ����ͼƬ����Ϣ */
    }

    lv_mem_free( pname);  /* �ͷ��ڴ� */
    lv_mem_free( buf);
    return rval;            /* �޴���. */
}

/**
 * @brief       ��ʼ��ͼƬ
 * @param       ��
 * @retval      0, ͼƬ�����; ����, ͼƬ�ⶪʧ;
 */
uint8_t images_init(void)
{
    uint8_t t = 0;

    nor_init();

    while (t < 10)  /* ������ȡ10��,���Ǵ���,˵��ȷʵ��������,�ø���ͼƬ���� */
    {
        t++;
        nor_read((uint8_t *)&g_ftinfo, IMAGEINFOADDR, sizeof(g_ftinfo)); /* ����g_ftinfo�ṹ������ */

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
