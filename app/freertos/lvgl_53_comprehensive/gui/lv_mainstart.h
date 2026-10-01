/**
 ****************************************************************************************************
 * @file        lv_mainstart.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2022-03-23
 * @brief       LVGL�ۺ� ʵ��
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
 
#ifndef __LV_MAINSTART_H
#define __LV_MAINSTART_H
#include "lvgl.h"
#include "exfuns.h"


 /* ���ؿ����� */
typedef struct
{
    void (*lv_general_win_create)(void); /* ��ʼ�����ؿ����� */
    lv_obj_t* parent;
    lv_event_cb_t lv_back_event;
} lv_m_general;

#define BACK_BTN_TITLE    LV_SYMBOL_LEFT" Back\n"   /* ���ذ����������� */

extern lv_m_general lv_general_dev;     /* ���ؿ����� */


void lv_general_win_create(void);
int lv_clz(unsigned int  app_readly_list[]);
void lv_mainstart(void);

#endif
