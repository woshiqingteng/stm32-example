/**
 * @file    main.c
 * @brief   lvgl_11_bar: LVGL bar demo (ALIENTEK experiment 11).
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
#include "delay.h"

/* 获取当前活动屏幕的宽高 */
#define scr_act_width() lv_obj_get_width(lv_scr_act())
#define scr_act_height() lv_obj_get_height(lv_scr_act())

static int16_t val = 0;                         /* 当前值 */

static const lv_font_t *font;                   /* 定义字体 */

static lv_obj_t *label_load;                    /* 加载标题标签 */
static lv_obj_t *label_per;                     /* 百分比标签 */
static lv_obj_t *bar;                           /* 进度条 */

/**
 * @brief  定时器回调
 * @param  *timer ：该定时器相关的数据
 * @return 无
 */
static void timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if(val < 100)                                                           /* 当前值小于100 */
    {
        val ++;
        lv_bar_set_value(bar, val, LV_ANIM_ON);                             /* 设置当前值 */
        lv_label_set_text_fmt(label_per, "%d %%", (int)lv_bar_get_value(bar)); /* 获取当前值，更新显示 */
    }
    else                                                                    /* 当前值大于等于100 */
    {
        lv_label_set_text(label_per, "finished!");                          /* 加载完成 */
    }
}

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
    lv_obj_align(label_load, LV_ALIGN_CENTER, 0, -scr_act_height() / 10 );

    /* 百分比标签 */
    label_per = lv_label_create(lv_scr_act());
    lv_label_set_text(label_per, "%0");
    lv_obj_set_style_text_font(label_per, font, LV_STATE_DEFAULT);
    lv_obj_align(label_per, LV_ALIGN_CENTER, 0, scr_act_height() / 10 );
}

/**
 * @brief  加载进度条
 * @param  无
 * @return 无
 */
static void lv_example_bar(void)
{
    bar = lv_bar_create(lv_scr_act());                            /* 创建进度条 */
    lv_obj_set_align(bar, LV_ALIGN_CENTER);                       /* 设置位置 */
    lv_obj_set_size(bar, scr_act_width() * 3 / 5, 20);            /* 设置大小 */
    lv_obj_set_style_anim_time(bar, 100, LV_STATE_DEFAULT);       /* 设置动画时间 */
    lv_timer_create(timer_cb, 100, NULL);                         /* 初始化定时器 */
}

/**
 * @brief  LVGL演示
 * @param  无
 * @return 无
 */
void lv_mainstart(void)
{
    lv_example_label();                                           /* 加载提示标签 */
    lv_example_bar();                                             /* 加载进度条 */
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
