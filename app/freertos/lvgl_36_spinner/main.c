/**
 * @file    main.c
 * @brief   lvgl_36_spinner: LVGL spinner demo (ALIENTEK experiment 36).
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

static const lv_font_t *font;                   /* 定义字体 */
static lv_obj_t *spinner;                       /* 加载器 */
static lv_obj_t *label_load;                    /* 加载标题标签 */

/**
 * @brief  加载提示标签
 * @param  无
 * @return 无
 */
static void lv_example_label(void)
{
    /* 根据活动屏幕宽度选择字体 */
    if (scr_act_width() <= 480)
    {
        font = &lv_font_montserrat_14;
    }
    else
    {
        font = &lv_font_montserrat_20;
    }
    
    /* 加载标题标签 */
    label_load = lv_label_create(lv_scr_act());
    lv_label_set_text(label_load, "LOADING...");
    lv_obj_set_style_text_font(label_load, font, LV_STATE_DEFAULT);
    lv_obj_align(label_load, LV_ALIGN_CENTER, 0, scr_act_height() / 10 );
}

/**
 * @brief  加载器显示
 * @param  无
 * @return 无
 */
static void lv_example_spinner(void)
{
    spinner = lv_spinner_create(lv_scr_act(), 1000, 60);                            /* 创建加载器 */
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -scr_act_height() / 15 );             /* 设置位置 */
    lv_obj_set_size(spinner, scr_act_height() / 5, scr_act_height() / 5);           /* 设置大小 */
    lv_obj_set_style_arc_width(spinner, scr_act_height() / 35, LV_PART_MAIN);       /* 设置主体圆弧宽度 */
    lv_obj_set_style_arc_width(spinner, scr_act_height() / 35, LV_PART_INDICATOR);  /* 设置指示器圆弧宽度 */
}

/**
 * @brief  LVGL演示
 * @param  无
 * @return 无
 */
void lv_mainstart(void)
{
    lv_example_label();            /* 加载提示标签 */
    lv_example_spinner();          /* 加载器显示 */
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
