/**
 ****************************************************************************************************
 * @file        lv_file.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-03-23
 * @brief       �ļ�����ϵͳ
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� ������ F429������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#ifndef __LV_FILE_H
#define __LV_FILE_H
#include "lvgl.h"
#include "exfuns.h"


#define LIST_SIZE    100   /* �����ļ��к��ļ������������ж��� */
#define FILE_SEZE    1992  /* ���ö�ȡ�ļ����ݵĴ�С�����ж��� */

typedef struct
{
    lv_obj_t * list;          /* �б��ؼ� */
    lv_obj_t *lv_page_obj;    /* �������Ķ��� */
    lv_obj_t * list_btn[LIST_SIZE]; /* �����б��������� */
    uint8_t list_flie_nuber;  /* �洢�ı��к��ļ������� */
    FRESULT fr;               /* ���ļ�����ֵ */
    DIR lv_dir;               /* ��ȡ��Ŀ¼ */
    FILINFO SD_fno;           /* �ļ���Ϣ�ṹ�� */
    char *pname;              /* ��·�����ļ��� */
    char *lv_pname;           /* ��ȡ�ļ��� */
    char *lv_pname_shift;     /* ��ȡ�ļ����м�洢 */
    const char* lv_pash;      /* ��ȡ·�� */
    int lv_suffix_flag;       /* ����׺��־λ */
    int lv_prev_file_flag;    /* ��һ���ļ�·����־λ */
    char *lv_prev_file[LIST_SIZE];  /* �洢�ļ�·�� */
    const void *image_scr;    /* ����ļ�����ͼƬ�����ĵ� */
    lv_obj_t * lv_back_obj;   /* ���巵��/�˵��Ķ��� */
    lv_obj_t * lv_prev_btn;   /* ���ذ��� */
    lv_obj_t * lv_back_btn;   /* �˵����� */
    lv_obj_t *lv_page_cont;   /* ��ʾ�ı������� */
    char rbuf[FILE_SEZE];     /* ��ȡ�ı������ݴ�С */
    lv_obj_t *lv_return_page; /* ��ȡ��ʾ��ҳ�� */
    lv_obj_t *lv_image_read;  /* ��ȡͼƬ���� */
    lv_obj_t *lv_file_cont;
}lv_file_struct;

/* ������ԣ��������Դ���ĵ�termΪ1ʱ��û�д��󣬵�termΪ0ʱ��������������� */
#define FILE_ASSERT(term)                                                                                   \
do                                                                                                          \
{                                                                                                           \
    if (!(term))                                                                                            \
    {                                                                                                       \
        printf("Assert failed. Condition(%s). [%s][%d]\r\n", term, __FUNCTION__, __LINE__);                 \
        while(1)                                                                                            \
        {                                                                                                   \
            ;                                                                                               \
        }                                                                                                   \
    }                                                                                                       \
} while (0)

void lv_file_demo(void);

#endif
