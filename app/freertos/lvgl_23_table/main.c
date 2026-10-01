/**
 * @file    main.c
 * @brief   lvgl_23_table: LVGL table demo (ALIENTEK experiment 23).
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

/**
 * @brief  表格实例
 * @param  无
 * @return 无
 */
static void lv_example_table(void)
{
    /* 标题 */
    lv_obj_t *label_title = lv_label_create(lv_scr_act());                                  /* 创建标题 */
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, scr_act_height()/8);                     /* 设置位置 */
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_20, LV_STATE_DEFAULT);      /* 设置字体 */
    lv_label_set_text(label_title, "Today's prices");                                       /* 设置文本内容 */

    /* 表格 */
    lv_obj_t *table = lv_table_create(lv_scr_act());                                        /* 创建表格 */
    lv_obj_set_height(table, scr_act_height()/2);                                           /* 设置表格总的高度 */
    lv_obj_center(table);                                                                   /* 设置位置 */

    /* 设置第1列单元格内容（名称） */
    lv_table_set_cell_value(table, 0, 0, "Name");
    lv_table_set_cell_value(table, 1, 0, "Apple");
    lv_table_set_cell_value(table, 2, 0, "Banana");
    lv_table_set_cell_value(table, 3, 0, "Lemon");
    lv_table_set_cell_value(table, 4, 0, "Grape");
    lv_table_set_cell_value(table, 5, 0, "Melon");
    lv_table_set_cell_value(table, 6, 0, "Peach");
    lv_table_set_cell_value(table, 7, 0, "Nuts");

    /* 设置第2列单元格内容（价格） */
    lv_table_set_cell_value(table, 0, 1, "Price");
    lv_table_set_cell_value(table, 1, 1, "$7");
    lv_table_set_cell_value(table, 2, 1, "$4");
    lv_table_set_cell_value(table, 3, 1, "$6");
    lv_table_set_cell_value(table, 4, 1, "$2");
    lv_table_set_cell_value(table, 5, 1, "$5");
    lv_table_set_cell_value(table, 6, 1, "$1");
    lv_table_set_cell_value(table, 7, 1, "$9");

    /* 单元格宽度 */
    lv_table_set_col_width(table, 0, scr_act_width()/3);
    lv_table_set_col_width(table, 1, scr_act_width()/3);
}

/**
 * @brief  LVGL演示
 * @param  无
 * @return 无
 */
void lv_mainstart(void)
{
    lv_example_table();
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
