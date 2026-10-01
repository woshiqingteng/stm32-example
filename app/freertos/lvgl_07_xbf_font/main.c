/**
 * @file    main.c
 * @brief   lvgl_07_xbf_font: LVGL XBF font read from SPI-NOR (ALIENTEK experiment 7).
 */

#include <stdio.h>

#include "bsp.h"
#include "sdram.h"
#include "fonts.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"
#include "lv_port.h"

#define LVGL_TASK_PRIO     3
#define LVGL_TASK_STK_SIZE 1024
#define LED_TASK_PRIO      4
#define LED_TASK_STK_SIZE  128

/* ===== ported from ALIENTEK lv_mainstart.c ===== */

LV_FONT_DECLARE(Font12)

static lv_obj_t *s_font_label;

static void lv_mainstart(void)
{
    s_font_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(s_font_label, &Font12, LV_STATE_DEFAULT);
    lv_label_set_text(s_font_label, "LVGL 正点原子XBF字库读取");
    lv_obj_align(s_font_label, LV_ALIGN_TOP_MID, 0, 0);
}

/* ==================== app bring-up ==================== */

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

    (void)fonts_init();

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
