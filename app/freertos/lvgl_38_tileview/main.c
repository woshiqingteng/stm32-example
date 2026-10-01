/**
 * @file    main.c
 * @brief   lvgl_38_tileview: LVGL tileview demo (ALIENTEK experiment 38).
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
#define scr_act_width()  lv_obj_get_width(lv_scr_act())
#define scr_act_height() lv_obj_get_height(lv_scr_act())

static const lv_font_t *font;           /* 定义字体 */

/**
 * @brief  平铺视图实例
 * @param  无
 * @return 无
 */
static void lv_example_tileview(void)
{
    /* 根据屏幕宽度设置字体 */
    if (scr_act_width() <= 480)
    {
        font = &lv_font_montserrat_14;
    }
    else
    {
        font = &lv_font_montserrat_20;
    }

    /* 创建平铺视图页面 */
    lv_obj_t *tileview = lv_tileview_create(lv_scr_act());                                  /* 创建平铺视图 */

    lv_obj_t *tile_1 = lv_tileview_add_tile( tileview, 0, 0, LV_DIR_RIGHT );                /* 添加页面1 */
    lv_obj_t *tile_2 = lv_tileview_add_tile( tileview, 1, 0, LV_DIR_LEFT|LV_DIR_RIGHT );    /* 添加页面2 */
    lv_obj_t *tile_3 = lv_tileview_add_tile( tileview, 2, 0, LV_DIR_LEFT );                 /* 添加页面3 */

    /* 设置页面内容 */
    lv_obj_t *label_1 = lv_label_create(tile_1);                                            /* 创建标签 */
    lv_label_set_text(label_1, "Page_1");                                                   /* 设置文本内容 */
    lv_obj_set_style_text_font(label_1, font, LV_STATE_DEFAULT);                            /* 设置字体 */
    lv_obj_center(label_1);                                                                 /* 设置位置 */

    lv_obj_t *label_2 = lv_label_create(tile_2);                                            /* 创建标签 */
    lv_label_set_text(label_2, "Page_2");                                                   /* 设置文本内容 */
    lv_obj_set_style_text_font(label_2, font, LV_STATE_DEFAULT);                            /* 设置字体 */
    lv_obj_center(label_2);                                                                 /* 设置位置 */

    lv_obj_t *label_3 = lv_label_create(tile_3);                                            /* 创建标签 */
    lv_label_set_text(label_3, "Page_3");                                                   /* 设置文本内容 */
    lv_obj_set_style_text_font(label_3, font, LV_STATE_DEFAULT);                            /* 设置字体 */
    lv_obj_center(label_3);                                                                 /* 设置位置 */
    lv_obj_remove_style(tileview, NULL, LV_PART_SCROLLBAR);                                 /* 移除滚动条 */

    /* 左侧状态栏 */
    lv_obj_t *label_left = lv_label_create(lv_scr_act());                                   /* 创建标签 */
    lv_label_set_text(label_left, "AM 8:30" );                                              /* 设置文本内容 */
    lv_obj_set_style_text_font(label_left, font, LV_STATE_DEFAULT);                         /* 设置字体 */
    lv_obj_align(label_left, LV_ALIGN_TOP_LEFT, 10, 10);                                    /* 设置位置 */

    /* 右侧状态栏 */
    lv_obj_t *label_right = lv_label_create(lv_scr_act());                                  /* 创建标签 */
    lv_label_set_text(label_right, LV_SYMBOL_WIFI "   80% " LV_SYMBOL_BATTERY_3);           /* 设置文本内容 */
    lv_obj_set_style_text_font(label_right, font, LV_STATE_DEFAULT);                        /* 设置字体 */
    lv_obj_align(label_right, LV_ALIGN_TOP_RIGHT, -10, 10);                                 /* 设置位置 */
}

/**
 * @brief  LVGL演示
 * @param  无
 * @return 无
 */
void lv_mainstart(void)
{
    lv_example_tileview();
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
