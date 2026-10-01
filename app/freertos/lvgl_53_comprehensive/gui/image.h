/**
 ****************************************************************************************************
 * @file        images.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-04
 * @brief       ͼƬ�� ����
 *              �ṩimages_update_image��images_init����ͼƬ����ºͳ�ʼ��
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

#ifndef __IMAGE_H
#define __IMAGE_H

#include "bsp.h"



/* ͼƬ��Ϣ�����׵�ַ
 * ռ41���ֽ�,��1���ֽ����ڱ��ͼƬ���Ƿ����.����ÿ8���ֽ�һ��,�ֱ𱣴���ʼ��ַ���ļ���С
 */
#define IMAGEINFOADDR 0x1F80000UL

/* ͼƬ����Ϣ�ṹ�嶨��
 * ��������ͼƬ�������Ϣ����ַ����С��
 */
typedef struct __attribute__((packed))
{
    uint8_t imageok;             /* ͼƬ����ڱ�־��0XAA��ͼƬ��������������ͼƬ�ⲻ���� */
    
    uint32_t lvgl_atk01addr;    /* LVGL_atk01��ַ */
    uint32_t lvgl_atk01size;    /* LVGL_atk01�Ĵ�С */
  
    uint32_t lvgl_atk02addr;    /* LVGL_atk02��ַ */
    uint32_t lvgl_atk02size;    /* LVGL_atk02�Ĵ�С */
  
    uint32_t lvgl_atk03addr;    /* LVGL_atk03��ַ */
    uint32_t lvgl_atk03size;    /* LVGL_atk03�Ĵ�С */
  
    uint32_t lvgl_atk04addr;    /* LVGL_atk04��ַ */
    uint32_t lvgl_atk04size;    /* LVGL_atk04�Ĵ�С */
    
    uint32_t lvgl_atk05addr;    /* LVGL_atk05��ַ */
    uint32_t lvgl_atk05size;    /* LVGL_atk05�Ĵ�С */
  
    uint32_t lvgl_atk06addr;    /* LVGL_atk06��ַ */
    uint32_t lvgl_atk06size;    /* LVGL_atk06�Ĵ�С */
  
    uint32_t lvgl_atk07addr;    /* LVGL_atk07��ַ */
    uint32_t lvgl_atk07size;    /* LVGL_atk07�Ĵ�С */
  
    uint32_t lvgl_atk08addr;    /* LVGL_atk08��ַ */
    uint32_t lvgl_atk08size;    /* LVGL_atk09�Ĵ�С */

} _image_info;

/* ͼƬ����Ϣ�ṹ�� */
extern _image_info g_ftinfo;


uint8_t images_update_image(uint16_t x, uint16_t y, uint8_t size, uint8_t *src, uint16_t color);  /* ����ȫ��ͼƬ�� */
uint8_t images_init(void);       /* ��ʼ��ͼƬ�� */

#endif
