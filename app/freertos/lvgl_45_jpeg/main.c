/**
 * @file    main.c
 * @brief   lvgl_45_jpeg: LVGL jpeg demo (ALIENTEK experiment 45).
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "sdram.h"
#include "exfuns.h"
#include "ff.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"
#include "lv_port.h"

/* ===== ported from ALIENTEK lv_mainstart.c ===== */

#include "lvgl.h"
#include <stdio.h>


#include "lvgl.h"
#include <stdio.h>


/**
 * @brief       lvgl程序入口
 * @param       无
 * @retval      无
 */
void lv_mainstart()
{
    /* 创建标签 */
    lv_obj_t *label = lv_label_create(lv_scr_act());
    /* 设置标签颜色 */
    lv_label_set_text(label,"JPEG_Decoder");
    /* 设置文本颜色 */
    lv_obj_set_style_text_color(label,lv_palette_main(LV_PALETTE_RED),LV_STATE_DEFAULT);
    /* 设置文本字体 */
    lv_obj_set_style_text_font(label,&lv_font_montserrat_32,LV_STATE_DEFAULT);
    /* 设置顶部中间对齐 */
    lv_obj_align(label,LV_ALIGN_TOP_MID,0,0);
    /* 设置背景颜色 */
    lv_obj_set_style_bg_color(lv_scr_act(),lv_palette_main(LV_PALETTE_BLUE),LV_STATE_DEFAULT);
    /* 创建image控件 */
    lv_obj_t * img = lv_img_create(lv_scr_act());
    /* 设置图像源 */
    lv_img_set_src(img, "0:/PICTURE/JPEG/SIM900A.jpg");
    /* 中间对齐 */
    lv_obj_align(img,LV_ALIGN_CENTER,0,0);
}

/* ==================== app bring-up ==================== */

#define LVGL_TASK_PRIO     3
#define LVGL_TASK_STK_SIZE 2048
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
    lv_port_fs_init();

    xTaskCreate(lvgl_task, "lvgl", LVGL_TASK_STK_SIZE, NULL, LVGL_TASK_PRIO, NULL);
    xTaskCreate(led_task, "led", LED_TASK_STK_SIZE, NULL, LED_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
