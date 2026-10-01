/**
 * @file    main.c
 * @brief   lvgl_14_canvas: LVGL canvas demo (ALIENTEK experiment 14).
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



LV_FONT_DECLARE(myFont14) \
LV_FONT_DECLARE(myFont24)

/* 画布的宽高 */
#define canvas_width()  100
#define canvas_height() 100
/* 图片的宽高 */
#define img_width()     150
#define img_height()    100

/**
 * @brief  例
 * @param  无
 * @return 无
 */
static void lv_example_canvas(void)
{
    static lv_color_t canvas_buf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(canvas_width(), canvas_height())];       /* 定义画布缓冲区 */
    lv_obj_t* canvas = lv_canvas_create(lv_scr_act());                                                  /* 定义并初始化画布 */
    lv_canvas_set_buffer(canvas, canvas_buf, canvas_width(), canvas_height(), LV_IMG_CF_TRUE_COLOR);    /* 设置画布缓冲区 */
    lv_obj_center(canvas);                                                                              /* 设置画布位置 */
    lv_canvas_fill_bg(canvas, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER);                    /* 设置画布背景颜色 */

    lv_draw_rect_dsc_t rect_dsc;                                                                        /* 定义绘画矩形 */
    lv_draw_rect_dsc_init(&rect_dsc);                                                                   /* 初始化绘画矩形 */
    rect_dsc.radius = 10;                                                                               /* 设置圆角 */
    rect_dsc.bg_opa = LV_OPA_COVER;                                                                     /* 设置透明度 */
    rect_dsc.bg_grad.dir = LV_GRAD_DIR_HOR;                                                             /* 设置颜色渐变方向 */
    rect_dsc.bg_grad.stops[0].color = lv_palette_main(LV_PALETTE_RED);                                  /* 设置开始颜色 */
    rect_dsc.bg_grad.stops[1].color = lv_palette_main(LV_PALETTE_BLUE);                                 /* 设置结束颜色 */
    rect_dsc.border_width = 2;                                                                          /* 设置边缘宽度 */
    rect_dsc.border_opa = LV_OPA_90;                                                                    /* 设置边缘透明度 */
    rect_dsc.border_color = lv_color_white();                                                           /* 设置边缘颜色 */
    lv_canvas_draw_rect(canvas,                                                                         /* 在画布上绘制矩形 */
                        (canvas_width() - img_width()) / 2,
                        (canvas_height() - img_height()) / 2,
                        img_width(),
                        img_height(),
                        &rect_dsc);

    lv_draw_label_dsc_t label_dsc;                                                                      /* 定义绘制标签 */
    lv_draw_label_dsc_init(&label_dsc);                                                                 /* 初始化绘制标签 */
    label_dsc.color = lv_color_black();                                                                 /* 设置标签颜色 */
    lv_canvas_draw_text(canvas,                                                                         /* 在画布上绘制标签 */
                        canvas_width() / 8,
                        canvas_height() / 8,
                        100,
                        &label_dsc,
                        "Some text on text canvas");

    static lv_color_t canvas_buf_temp[LV_CANVAS_BUF_SIZE_TRUE_COLOR(canvas_width(), canvas_height())];  /* 定义图片缓冲区 */
    lv_memcpy(canvas_buf_temp, canvas_buf, sizeof(canvas_buf_temp));                                    /* 复制旧缓冲区 */
    lv_img_dsc_t img;                                                                                   /* 定义图片 */
    img.data = (uint8_t*)canvas_buf_temp;                                                               /* 设置图片数据 */
    img.header.cf = LV_IMG_CF_TRUE_COLOR;                                                               /* 图片颜色格式 */
    img.header.w = canvas_width();                                                                      /* 宽度 */
    img.header.h = canvas_height();                                                                     /* 高度 */

    lv_canvas_transform(canvas,                                                                         /* 旋转画布 */
                        &img, 30,
                        LV_IMG_ZOOM_NONE,
                        0, 0,
                        canvas_width() / 2,
                        canvas_height() / 2,
                        true);
}

/**
 * @brief  LVGL演示
 * @param  无
 * @return 无
 */
void lv_mainstart(void)
{
    lv_example_canvas();
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
