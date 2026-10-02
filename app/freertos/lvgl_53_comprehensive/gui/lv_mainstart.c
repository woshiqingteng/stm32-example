/**
 * @file    lv_mainstart.c
 * @brief   LVGL comprehensive demo launcher.
 */

#include "lv_mainstart.h"
#include "exfuns.h"
#include "malloc.h"
#include "lcd.h"
#include "usart.h"
#include "lv_qr.h"
#include "lv_draw.h"
#include "lv_file.h"
#include "lv_shelf.h"
#include "lv_setting.h"
#include "lv_calculator.h"
#include "lv_meter.h"
#include "lv_scale.h"
#include "lvgl.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"


lv_m_general lv_general_dev;

typedef struct
{
    char* app_text_English;
    char* app_text_Chinese;
    uint16_t app_witch;
    uint16_t app_hietch;
}app_image_info;


/* app icon bitmap paths on the SD card */
char *const IMAGE_GBK_PATH[8] =
{
    "0:/PICTURE/LVGLBIN/Calculator.bin",
    "0:/PICTURE/LVGLBIN/File.bin",
    "0:/PICTURE/LVGLBIN/lv_system.bin",
    "0:/PICTURE/LVGLBIN/Setting.bin",
    "0:/PICTURE/LVGLBIN/Test.bin",
    "0:/PICTURE/LVGLBIN/Timer.bin",
    "0:/PICTURE/LVGLBIN/lv_qr.bin",
    "0:/PICTURE/LVGLBIN/lv_draw.bin",
};

#define IMAGE_GBK_NUM (int)(sizeof(IMAGE_GBK_PATH)/sizeof(IMAGE_GBK_PATH[0]))
    
static const app_image_info app_image[] =
{
    {" "," ",0,0},
    {"Calculator","计算器",146,140},
    {"File","文件管理器",146,140},
    {"System","进制",146,140},
    {"Setting","设置",146,140},
    {"Test","测试",146,140},
    {"Timer","时钟",304,140},
    {"Qrcode","二维码",146,140},
    {"Draw","绘画",304,140},
};

/* number of app entries */
#define image_mun (int)(sizeof(app_image)/sizeof(app_image[0]))
/* app object / name / image arrays */
lv_obj_t *lv_app_t[image_mun];
lv_obj_t *lv_app_name[image_mun];
lv_obj_t* lv_app_img[image_mun];
/* app ready table */
unsigned int  app_readly_list[32];
/* app trigger bit */
int lv_trigger_bit = 0;


/* Create the back button. */
void lv_general_win_create(void)
{
    #define TOP_OFFSET  -10
    lv_obj_t* back_btn = lv_label_create(lv_general_dev.parent);
    lv_label_set_text(back_btn, BACK_BTN_TITLE);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align_to(back_btn, NULL, LV_ALIGN_TOP_LEFT, 15, TOP_OFFSET);
    lv_obj_set_style_text_color(back_btn, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
    lv_obj_add_event_cb(back_btn, lv_general_dev.lv_back_event, LV_EVENT_ALL, NULL);
}

/* Count leading zeros in the ready table. */
int lv_clz(unsigned int  app_readly_list[])
{
    int bit = 0;

    for (int i = 0; i < 32; i++)
    {
        if (app_readly_list[i] == 1)
        {
            break;
        }

        bit ++ ;
    }

    return bit;
}

/* App icon click callback: switch to the selected demo. */
static void lv_imgbtn_control_event_handler(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);
    lv_obj_t *lv_app_parent = lv_obj_get_parent(obj);

    if (code == LV_EVENT_CLICKED)
    {
        for (int i = 0;i < image_mun;i ++)
        {
            if (obj == lv_app_t[i])
            {
                app_readly_list[i] = 1 ;                                       /* mark ready */
            }
        }

        lv_trigger_bit = ((unsigned int)lv_clz((app_readly_list)));            /* find the triggered bit */
        app_readly_list[lv_trigger_bit] = 0;                                   /* clear it */
        lv_obj_del(lv_app_parent);                                             /* delete the launcher view */
        lv_app_parent = NULL;                                                  /* main container = none */

        switch(lv_trigger_bit)                                                 /* dispatch */
        {
            case 1:
              lv_calculator_demo();                                            /* calculator */
              break;
            case 2:
              lv_file_demo();                                                  /* file manager */
              break;
            case 3:
              lv_scale_demo();                                                 /* base converter */
              break;
            case 4:
              lv_setting_demo();                                               /* settings */
              break;
            case 5:
              lv_shelf_demo();                                                 /* on-board test */
              break;
            case 6:
              lv_meter_demo();                                                 /* clock */
              break;
            case 7:
              lv_qr_windowm();                                                 /* QR code */
              break;
            case 8:
              lv_draw_demo();                                                  /* drawing */
              break;
        }
    }
}

/* Lay out the app icons in a grid. */
void lv_mid_cont_add_app(lv_obj_t *parent)
{
    int line_feed_num = 0;
    int lv_index = 0;
    lv_app_t[lv_index] = NULL;
    lv_index ++;
    int i = 0;
    int n = 1;

    lv_app_t[lv_index] = lv_obj_create(parent);
    lv_obj_set_pos(lv_app_t[lv_index],-7,20);
    lv_obj_set_size(lv_app_t[lv_index], app_image[lv_index].app_witch, app_image[lv_index].app_hietch);
    lv_obj_set_style_bg_color(lv_app_t[lv_index], lv_color_make(26, 57, 137), LV_STATE_DEFAULT);
    lv_obj_clear_flag(lv_app_t[lv_index], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(lv_app_t[lv_index],0,LV_PART_MAIN);
    lv_obj_add_event_cb(lv_app_t[lv_index], lv_imgbtn_control_event_handler, LV_EVENT_ALL, NULL);

    lv_app_name[lv_index] = lv_label_create(lv_app_t[lv_index]);
    lv_obj_set_style_text_color(lv_app_name[lv_index],lv_color_make(255,255,255), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lv_app_name[lv_index],&lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_label_set_text(lv_app_name[lv_index], app_image[lv_index].app_text_English);
    lv_obj_align(lv_app_name[lv_index],LV_ALIGN_BOTTOM_LEFT,-10, 10);

    lv_app_img[lv_index] = lv_img_create(lv_app_t[lv_index]);
    lv_img_set_src(lv_app_img[lv_index], IMAGE_GBK_PATH[lv_index - 1]);
    lv_obj_center(lv_app_img[lv_index]);
    lv_obj_set_style_img_recolor_opa(lv_app_img[lv_index], 255, LV_PART_MAIN);
    lv_obj_set_style_img_recolor(lv_app_img[lv_index], lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
    unsigned int lv_width_x = lv_obj_get_width(lv_app_t[1]) + 20;
    lv_index ++;
    

    for (lv_index = 2 ; lv_index < image_mun ; lv_index ++)
    {
        lv_app_t[lv_index] = lv_obj_create(parent);
        lv_obj_set_size(lv_app_t[lv_index], app_image[lv_index].app_witch, app_image[lv_index].app_hietch);
        lv_obj_set_style_bg_color(lv_app_t[lv_index], lv_color_make(26, 57, 137), LV_STATE_DEFAULT);
        lv_obj_clear_flag(lv_app_t[lv_index], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(lv_app_t[lv_index], 0, LV_PART_MAIN);
        lv_obj_update_layout(lv_app_t[lv_index]);
        lv_width_x = lv_width_x + lv_obj_get_width(lv_app_t[lv_index]) + 10;

        if (lv_width_x < lv_obj_get_width(lv_scr_act()))
        {
            lv_obj_align_to(lv_app_t[lv_index], lv_app_t[lv_index - 1], LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        }
        else
        {
            line_feed_num++;

            if (line_feed_num >= 2)
            {
                i = 10 * n;
                n ++;
            }
            else
            {
                i = 0;
            }
            
            lv_obj_set_pos(lv_app_t[lv_index], -7, (lv_obj_get_height(lv_app_t[lv_index]) + 20) * line_feed_num + 10 - i );
            lv_width_x = lv_obj_get_width(lv_app_t[lv_index]) + 10;
        }
        
        lv_app_name[lv_index] = lv_label_create(lv_app_t[lv_index]);
        lv_obj_set_style_text_color(lv_app_name[lv_index], lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lv_app_name[lv_index], &lv_font_montserrat_14, LV_STATE_DEFAULT);
        lv_label_set_text(lv_app_name[lv_index], app_image[lv_index].app_text_English);
        lv_obj_align(lv_app_name[lv_index], LV_ALIGN_BOTTOM_LEFT, -10, 10);

        lv_app_img[lv_index] = lv_img_create(lv_app_t[lv_index]);
        lv_img_set_src(lv_app_img[lv_index], IMAGE_GBK_PATH[lv_index - 1]);
        lv_obj_center(lv_app_img[lv_index]);
        lv_obj_set_style_img_recolor_opa(lv_app_img[lv_index], 255, LV_PART_MAIN);
        lv_obj_set_style_img_recolor(lv_app_img[lv_index], lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
        lv_obj_add_event_cb(lv_app_t[lv_index], lv_imgbtn_control_event_handler, LV_EVENT_ALL, NULL);
    }
}

/* Status-bar icons (top-left, date, top-right). */
void lv_app_icon(lv_obj_t *praten)
{
    /* top-left icons */
    lv_obj_t* lv_letf_acon = lv_label_create(praten);
    lv_label_set_text(lv_letf_acon, LV_SYMBOL_WIFI " " LV_SYMBOL_AUDIO);
    lv_obj_align(lv_letf_acon,LV_ALIGN_TOP_LEFT,-5,-10);
    lv_obj_set_style_text_color(lv_letf_acon, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
    /* date */
    lv_obj_t* lv_timer = lv_label_create(praten);
    lv_label_set_text(lv_timer, "2022/1/20");
    lv_obj_align(lv_timer, LV_ALIGN_TOP_MID, 0, -10);
    lv_obj_set_style_text_color(lv_timer, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
    /* top-right icons */
    lv_obj_t* lv_right_acon = lv_label_create(praten);
    lv_label_set_text(lv_right_acon, LV_SYMBOL_BATTERY_3 " " LV_SYMBOL_USB);
    lv_obj_align(lv_right_acon, LV_ALIGN_TOP_RIGHT, 5, -10);
    lv_obj_set_style_text_color(lv_right_acon, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);

}

/* Main window. */
void lv_main_window(void)
{
    lv_obj_t *lv_main_cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(lv_main_cont, lv_obj_get_width(lv_scr_act()), lv_obj_get_height(lv_scr_act()));
    lv_obj_set_style_bg_color(lv_main_cont, lv_color_make(1, 27, 54), LV_STATE_DEFAULT);
    lv_obj_set_style_radius(lv_main_cont, 0, LV_PART_MAIN);
    lv_obj_clear_flag(lv_main_cont,LV_OBJ_FLAG_SCROLLABLE);
    lv_mid_cont_add_app(lv_main_cont);
    lv_app_icon(lv_main_cont);
}

/* LVGL entry. */
void lv_mainstart(void)
{
    lv_main_window();       /* show the launcher */
}
