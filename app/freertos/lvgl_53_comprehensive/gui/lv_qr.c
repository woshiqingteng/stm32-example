/**
 ****************************************************************************************************
 * @file        lv_qr.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2022-03-23
 * @brief       ��ά��������
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
 
#include "lv_qr.h"
#include "lv_mainstart.h"
#include "lcd.h"
#include "lvgl.h"
#include <stdio.h>


lv_obj_t *ta;
lv_obj_t * lv_qr_win;
lv_obj_t * lv_qr_cont;
lv_obj_t * lv_qr_btn_win;
lv_obj_t * lv_qr_label;
lv_obj_t * lv_qr_c_label;
const char * data;

/* ��ȡ��ǰ���Ļ�Ŀ��� */
#define scr_act_width() lv_obj_get_width(lv_scr_act())
#define scr_act_height() lv_obj_get_height(lv_scr_act())

void lv_qr_windowm(void);


/**
 * @brief  ���̻ص�
 * @param  event���¼�
 * @return ��
 */
void lv_keyboard_cb(lv_event_t *event)
{
    lv_obj_t* obj = lv_event_get_target(event);
    lv_event_code_t code = lv_event_get_code(event);
    
    if(code == LV_EVENT_CLICKED)
    {
        if (lv_keyboard_get_selected_btn ((lv_obj_t *)event->user_data) == 39)
        {
            lv_obj_del((lv_obj_t *)event->user_data); /* ɾ������ */
        }
    }
}

/**
 * @brief  ��
 * @param  ��
 * @return ��
 */
static void lv_example_keyboard(void)
{
    lv_obj_t* keyboard = lv_keyboard_create(lv_scr_act());                      /* ���岢�������� */
    lv_keyboard_set_textarea(keyboard, ta);
    lv_obj_add_event_cb(keyboard, lv_keyboard_cb, LV_EVENT_ALL, keyboard);   /* �����ı����¼��ص� */
}

/**
 * @brief  �ı�����ص�
 * @param  event���¼�
 * @return ��
 */
void lv_ta_action_click(lv_event_t *event)
{
    lv_obj_t* obj = lv_event_get_target(event);
    lv_event_code_t code = lv_event_get_code(event);
    
    if(code == LV_EVENT_CLICKED)    /* �ı������� */
    {
        lv_example_keyboard();
    }
}

/**
 * @brief �˳��ص�����
 * @param  event���¼�
 * @return ��
 */
static void lv_qr_event_handler(lv_event_t * event)
{
    lv_obj_t* obj = lv_event_get_target(event);
    lv_event_code_t code = lv_event_get_code(event);
    
    if(code == LV_EVENT_CLICKED)    /*��� */
    {
        lv_obj_del(lv_qr_win);
        lv_mainstart();
    }
}

/**
 * @brief ��ά�����ɻص�����
 * @param  event���¼�
 * @return ��
 */
static void lv_btn_create_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);
    
    if (code == LV_EVENT_CLICKED)
    {
        lv_label_set_text(lv_qr_label,"Generated QR code successfully!");
        
        data = lv_textarea_get_text(ta);
        /* ����һ��lcd_info()->width/2�Ķ�ά�� */
        lv_obj_t * qr = lv_qrcode_create(lv_qr_btn_win, lv_obj_get_width(lv_qr_btn_win) - 10, lv_color_hex3(0x33f), lv_color_hex3(0xeef));
        /* �������� */
        lv_qrcode_update(qr, data, strlen(data));
        /* ��ά���м���� */
        lv_obj_center(qr);
        lv_obj_invalidate(qr);
    }
}

/**
 * @brief  ��ά�����
 * @param  ��
 * @return ��
 */
void lv_qr_windowm(void)
{
    if (lcd_info()->dir == 0)    /* ����ģʽ, x������Ҫ�任 */
    {
        lv_disp_set_rotation(lv_disp_get_default(), LV_DISP_ROT_90);
    }
    
    lv_qr_win = lv_win_create(lv_scr_act(),lv_font_montserrat_14.line_height + 50);
    lv_obj_set_size(lv_qr_win,lv_obj_get_width(lv_scr_act()),lv_obj_get_height(lv_scr_act()));
    lv_obj_set_style_text_font(lv_qr_win,&lv_font_montserrat_14,LV_STATE_DEFAULT);
    lv_win_add_title(lv_qr_win, "QR V1.0");
    lv_obj_t * btn = lv_win_add_btn(lv_qr_win, LV_SYMBOL_CLOSE, 50);
    lv_obj_add_event_cb(btn, lv_qr_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_qr_cont = lv_win_get_content(lv_qr_win);
    
    ta = lv_textarea_create(lv_qr_cont);                           /* �����ı�����ؼ� */
    lv_obj_set_style_radius(ta, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(ta,&lv_font_montserrat_14,LV_STATE_DEFAULT);     /* �����ı����� */
    lv_obj_set_style_border_color(ta,lv_color_make(0,0,0),LV_STATE_DEFAULT);
    lv_obj_set_size(ta, lv_obj_get_width(lv_scr_act())/2, lv_obj_get_height(lv_scr_act())/2);    /* �ı�����Ĵ�С */
    lv_obj_align_to(ta, lv_qr_cont, LV_ALIGN_TOP_LEFT, 0, 30);     /* ���� */
    lv_textarea_set_cursor_click_pos(ta, true);                    /* �����ı������ϵ��ı�����/���ù�궨λ */
    lv_obj_add_event_cb(ta, lv_ta_action_click,LV_EVENT_ALL,NULL); /* ���ûص����� */
    lv_textarea_set_text(ta, " ");                                 /* ���ó�ʼ״̬���ı�Ϊ�� */
    
    lv_qr_btn_win = lv_obj_create(lv_qr_cont);
    lv_obj_set_style_border_color(lv_qr_btn_win,lv_color_make(0,0,0),LV_STATE_DEFAULT);
    lv_obj_set_style_radius(lv_qr_btn_win, 0, LV_PART_MAIN);
    lv_obj_set_size(lv_qr_btn_win,lv_obj_get_width(ta)/2 + 40,lv_obj_get_width(ta)/2 + 40);
    lv_obj_invalidate(lv_qr_btn_win);
    lv_obj_align_to(lv_qr_btn_win, ta, LV_ALIGN_OUT_RIGHT_MID, lv_obj_get_width(ta)/4, 0);     /* ���� */
    
    lv_obj_t *lv_qr_btn = lv_btn_create(lv_qr_cont);
    lv_obj_set_style_bg_color(lv_qr_btn,lv_palette_main(LV_PALETTE_GREY),LV_STATE_DEFAULT);
    lv_obj_align_to(lv_qr_btn, lv_qr_btn_win, LV_ALIGN_OUT_BOTTOM_LEFT, 0, lv_obj_get_height(lv_qr_btn_win)/6);     /* ���� */
    lv_obj_set_size(lv_qr_btn,lv_obj_get_width(lv_qr_btn_win),lv_obj_get_height(lv_qr_btn_win)/3);
    lv_obj_add_event_cb(lv_qr_btn,lv_btn_create_cb,LV_EVENT_ALL,NULL);
    
    lv_obj_t *lv_qr_btn_label = lv_label_create(lv_qr_btn);
    lv_obj_set_style_text_color(lv_qr_btn_label,lv_color_make(255,255,255),LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lv_qr_btn_label,&lv_font_montserrat_14,LV_STATE_DEFAULT);     /* �����ı����� */
    lv_obj_clear_flag(lv_qr_btn_win,LV_OBJ_FLAG_SCROLLABLE);
    lv_label_set_text(lv_qr_btn_label,"Create Qr");
    lv_obj_center(lv_qr_btn_label);
    
    lv_qr_label = lv_label_create(lv_qr_cont);
    lv_obj_set_style_text_color(lv_qr_label,lv_palette_main(LV_PALETTE_BLUE),LV_STATE_DEFAULT);
    lv_label_set_text(lv_qr_label,"");
    lv_obj_align_to(lv_qr_label, ta, LV_ALIGN_OUT_BOTTOM_LEFT, 0, lv_obj_get_height(ta)/6);            /* ���� */
    
    lv_qr_c_label = lv_label_create(lv_qr_cont);
    lv_obj_set_style_text_color(lv_qr_c_label,lv_palette_main(LV_PALETTE_GREY),LV_STATE_DEFAULT);
    lv_label_set_text(lv_qr_c_label,"Alientek:http://www.openedv.com/");
    lv_obj_align_to(lv_qr_c_label, lv_qr_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, lv_obj_get_height(ta)/6); /* ���� */
}
