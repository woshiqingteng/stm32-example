/**
 * @file    main.c
 * @brief   lvgl_44_qrcode: LVGL qrcode demo (ALIENTEK experiment 44).
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


const char * data = "Hello ALIENTEK(正点原子) ";

/**
 * @brief  二维码显示
 * @param  无
 * @return 无
 */
void lv_mainstart(void)
{
    /* 创建一个标签 */
    lv_obj_t *label_time = lv_label_create(lv_scr_act());
    /* 设置标签的文本 */
    lv_label_set_text(label_time,"ALIENTEK_QR");
    /* 设置标签的文本字体颜色 */
    lv_obj_set_style_text_color(label_time,lv_palette_main(LV_PALETTE_RED),LV_STATE_DEFAULT);
    /* 设置标签的文本字体 */
    lv_obj_set_style_text_font(label_time,&lv_font_montserrat_32,LV_STATE_DEFAULT);
    /* 设置标签的顶部中间对齐 */
    lv_obj_align(label_time,LV_ALIGN_TOP_MID,0,0);
    /* 创建一个lcddev.width/2的二维码 */
    lv_obj_t * qr = lv_qrcode_create(lv_scr_act(), lv_obj_get_width(lv_scr_act())/2, lv_color_hex3(0x33f), lv_color_hex3(0xeef));
    /* 设置数据 */
    lv_qrcode_update(qr, data, strlen(data));
    /* 二维码中间对齐 */
    lv_obj_align(qr,LV_ALIGN_CENTER,0,0);
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
