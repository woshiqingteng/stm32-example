/**
 * @file    main.c
 * @brief   lvgl_31_list: LVGL list demo (ALIENTEK experiment 31).
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"
#include "lv_port.h"

/* ===== ported from ALIENTEK lv_mainstart.c ===== */

#include "lvgl.h"
#include <stdio.h>


/* 获取当前活动屏幕的宽高 */
#define scr_act_width() lv_obj_get_width(lv_scr_act())
#define scr_act_height() lv_obj_get_height(lv_scr_act())

static lv_obj_t *list;              /* 定义列表 */
static lv_obj_t *list_label;        /* 定义标签 */
static const lv_font_t *font;       /* 定义字体 */

/**
 * @brief  列表按钮事件回调
 * @param  *e ：事件相关参数的集合，它包含了该事件的所有数据
 * @return 无
 */
static void list_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *list_btn = lv_event_get_target(e);                                   /* 获取触发源 */

    lv_label_set_text(list_label, lv_list_get_btn_text(list, list_btn));           /* 获取按钮文本并显示 */
    
    lv_obj_add_state(list_btn, LV_STATE_FOCUS_KEY);                                /* 添加状态（聚焦） */
}

/**
 * @brief  例
 * @param  无
 * @return 无
 */
static void lv_example_list(void)
{
    /* 根据屏幕大小设置字体 */
    if (scr_act_width() <= 320)
    {
        font = &lv_font_montserrat_14;
    }
    else if (scr_act_width() <= 480)
    {
        font = &lv_font_montserrat_16;
    }
    else
    {
        font = &lv_font_montserrat_18;
    }

    /* 创建左侧矩形背景 */
    lv_obj_t* obj_left = lv_obj_create(lv_scr_act());                               /* 创建一个基础对象 */
    lv_obj_set_width(obj_left, scr_act_width() * 0.7);                              /* 设置宽度 */
    lv_obj_set_height(obj_left, scr_act_height() * 0.9);                            /* 设置高度 */
    lv_obj_align(obj_left, LV_ALIGN_LEFT_MID, 5, 0);                                /* 设置位置 */
    lv_obj_update_layout(obj_left);                                                 /* 手动更新物体的参数 */

    /* 创建右侧矩形背景 */
    lv_obj_t* obj_right = lv_obj_create(lv_scr_act());                              /* 创建一个基础对象 */
    lv_obj_set_width(obj_right, scr_act_width() - lv_obj_get_width(obj_left) - 15); /* 设置宽度 */
    lv_obj_set_height(obj_right, lv_obj_get_height(obj_left));                      /* 设置高度 */
    lv_obj_align_to(obj_right, obj_left, LV_ALIGN_OUT_RIGHT_MID, 5, 0);             /* 设置位置 */
    lv_obj_update_layout(obj_right);                                                /* 手动更新物体的参数 */

    /* 显示当前选项的文本内容 */
    list_label = lv_label_create(obj_right);                                        /* 创建标签 */
    lv_obj_set_width(list_label, lv_obj_get_width(obj_right) - 13);                 /* 设置标签的宽度 */
    lv_obj_align(list_label, LV_ALIGN_TOP_MID, 0, 5);                               /* 设置标签位置 */
    lv_obj_update_layout(list_label);                                               /* 手动更新标签的参数 */
    lv_obj_set_style_text_align(list_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);    /* 设置标签文本对齐方式 */
    lv_label_set_text(list_label, "New");                                           /* 设置标签文本 */
    lv_obj_set_style_text_font(list_label, font, LV_PART_MAIN);                     /* 设置标签文本字体 */

    /* 创建列表 */
    list = lv_list_create(obj_left);                                                /* 创建列表 */
    lv_obj_set_width(list, lv_obj_get_width(obj_left) * 0.8);                       /* 设置列表宽度 */
    lv_obj_set_height(list, lv_obj_get_height(obj_left) * 0.9);                     /* 设置列表高度 */
    lv_obj_center(list);                                                            /* 设置列表的位置 */
    lv_obj_set_style_text_font(list, font, LV_PART_MAIN);                           /* 设置字体 */

    /* 为列表添加按钮 */
    lv_obj_t* btn;
    lv_list_add_text(list, "File");                                                 /* 添加列表文本 */
    btn = lv_list_add_btn(list, LV_SYMBOL_FILE, "New");                             /* 添加按钮 */
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);            /* 添加按钮回调 */
    btn = lv_list_add_btn(list, LV_SYMBOL_DIRECTORY, "Open");                       /* 添加按钮 */
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);            /* 添加按钮回调 */
    btn = lv_list_add_btn(list, LV_SYMBOL_SAVE, "Save");                            /* 添加按钮 */
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);            /* 添加按钮回调 */
    btn = lv_list_add_btn(list, LV_SYMBOL_CLOSE, "Delete");                         /* 添加按钮 */
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);            /* 添加按钮回调 */
    btn = lv_list_add_btn(list, LV_SYMBOL_EDIT, "Edit");                            /* 添加按钮 */
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);            /* 添加按钮回调 */
    lv_list_add_text(list, "Connectivity");                                         /* 添加列表文本 */
    btn = lv_list_add_btn(list, LV_SYMBOL_BLUETOOTH, "Bluetooth");                  /* 添加按钮 */
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);            /* 添加按钮回调 */
    btn = lv_list_add_btn(list, LV_SYMBOL_GPS, "Navigation");                       /* 添加按钮 */
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);            /* 添加按钮回调 */
    btn = lv_list_add_btn(list, LV_SYMBOL_USB, "USB");                              /* 添加按钮 */
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);            /* 添加按钮回调 */
    btn = lv_list_add_btn(list, LV_SYMBOL_BATTERY_FULL, "Battery");                 /* 添加按钮 */
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);            /* 添加按钮回调 */
    lv_list_add_text(list, "Exit");                                                 /* 添加列表文本 */
    btn = lv_list_add_btn(list, LV_SYMBOL_OK, "Apply");                             /* 添加按钮 */
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);            /* 添加按钮回调 */
    btn = lv_list_add_btn(list, LV_SYMBOL_CLOSE, "Close");                          /* 添加按钮 */
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_CLICKED, NULL);            /* 添加按钮回调 */
}

/**
 * @brief  LVGL演示
 * @param  无
 * @return 无
 */
void lv_mainstart(void)
{
    lv_example_list();
}

/* ==================== app bring-up ==================== */

#define LVGL_TASK_PRIO     3
#define LVGL_TASK_STK_SIZE 1024
#define LED_TASK_PRIO      4
#define LED_TASK_STK_SIZE  128

static void lvgl_task(void *pvParameters)
{
    (void)pvParameters;

    lv_mainstart();

    for (;;)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void led_task(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        led_toggle(LED0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");
    sdram_init();

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    xTaskCreate(lvgl_task, "lvgl", LVGL_TASK_STK_SIZE, NULL, LVGL_TASK_PRIO, NULL);
    xTaskCreate(led_task, "led", LED_TASK_STK_SIZE, NULL, LED_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
