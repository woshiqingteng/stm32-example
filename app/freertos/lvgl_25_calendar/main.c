/**
 * @file    main.c
 * @brief   lvgl_25_calendar: LVGL calendar demo (ALIENTEK experiment 25).
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "sdram.h"
#include "fonts.h"
#include "exfuns.h"
#include "ff.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"
#include "lv_port.h"

/* ===== ported from ALIENTEK lv_mainstart.c ===== */

#include "lvgl.h"
#include <stdio.h>



LV_FONT_DECLARE(Font14) \
LV_FONT_DECLARE(Font24)

#if LV_CALENDAR_WEEK_STARTS_MONDAY == 0
#error LV_CALENDAR_WEEK_STARTS_MONDAY MUST SET 1.
#endif

/* 获取当前活动屏幕的宽高 */
#define scr_act_width() lv_obj_get_width(lv_scr_act())
#define scr_act_height() lv_obj_get_height(lv_scr_act())

lv_obj_t* calendar;

/**
 * @brief  日历回调
 * @param  无
 * @return 无
 */
static void event_calendar_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    (void)obj;
    lv_obj_t* label = (lv_obj_t*)e->user_data;
    lv_calendar_date_t date_temp;
    char buf[11];

    if (LV_EVENT_VALUE_CHANGED == code)
    {
        if (LV_RES_OK == lv_calendar_get_pressed_date(calendar, &date_temp))
         {
            lv_calendar_set_today_date(calendar, date_temp.year, date_temp.month, date_temp.day);
            lv_snprintf(buf, sizeof(buf), "%d/%02d/%02d", date_temp.year, date_temp.month, date_temp.day);
            lv_label_set_text(label, buf);
        }
    }
}

static lv_calendar_date_t highlight_days[2];/* 定义的日期,必须用全局或静态定义 */

/**
 * @brief  例
 * @param  无
 * @return 无
 */
static void lv_example_calendar(void)
{
    /* 定义并初始化日历 */
    calendar = lv_calendar_create(lv_scr_act());
    /* 设置日历的大小 */
    lv_obj_set_size(calendar, scr_act_height() * 0.85, scr_act_height()* 0.85);
    lv_obj_center(calendar);
    /* 设置日历的日期 */
    lv_calendar_set_today_date(calendar, 2022, 4, 7);
    /* 设置日历显示的月份 */
    lv_calendar_set_showed_date(calendar, 2022, 4);
    /* 设置日历头 */
    lv_calendar_header_dropdown_create(calendar);


    highlight_days[0].year = 2022;  /* 设置第一个日期 */
    highlight_days[0].month = 4;
    highlight_days[0].day = 5;
    highlight_days[1].year = 2022;  /* 设置第二个日期 */
    highlight_days[1].month = 4;
    highlight_days[1].day = 6;

    lv_calendar_set_highlighted_dates(calendar, highlight_days, 3);
    /* 更新日历参数 */
    lv_obj_update_layout(calendar);


    lv_obj_t* label = lv_label_create(lv_scr_act());                                /* 定义并创建标签 */
    lv_obj_set_width(label, lv_obj_get_x(calendar));                                /* 设置标签宽度 */
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);                                   /* 设置标签位置 */
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);         /* 设置标签文本对齐方式 */
    lv_label_set_text(label, "Wait input...");                                      /* 设置标签文本 */

    lv_obj_add_event_cb(calendar, event_calendar_cb, LV_EVENT_ALL, label);           /* 设置日历回调 */
}

/**
 * @brief  LVGL演示
 * @param  无
 * @return 无
 */
void lv_mainstart(void)
{
    lv_example_calendar();
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

    (void)exfuns_init();
    (void)f_mount(fs[0], "0:", 1);

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    /* Load the CJK XBF fonts from SD into the SPI-NOR store on first use
     * (progress is drawn on the LCD, so run after lv_port_disp_init). */
    if (fonts_init() != 0 || fonts_lvgl_ok() == 0)
    {
        (void)fonts_update_font(0, 0, 16, (uint8_t *)"0:", 0xFFFF);
        (void)fonts_init();
    }

    xTaskCreate(lvgl_task, "lvgl", LVGL_TASK_STK_SIZE, NULL, LVGL_TASK_PRIO, NULL);
    xTaskCreate(led_task, "led", LED_TASK_STK_SIZE, NULL, LED_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
