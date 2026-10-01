/**
 * @file    main.c
 * @brief   lvgl_42_png: LVGL png demo (ALIENTEK experiment 42).
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


/* PNG图片结构体 */
typedef struct
{
    char *img_path;    /* 图片路径 */
    char *label_text;  /* 图片名称 */
}img_info_t;

/* 定义PNG路径 */
const img_info_t PNG_PATH[] =
{
    {"0:/PICTURE/PNG/mlljt.png", "xiaomao.png"},
    {"0:/PICTURE/PNG/laji.png", "laji.png"},
};
/* 获取路径的个数 */
#define image_mun (int)(sizeof(PNG_PATH)/sizeof(PNG_PATH[0]))
  
lv_obj_t *img;

/**
 * @brief       创建PNG图片文件
 * @param       parent：父类
 * @param       path：  图片路径
 * @retval      返回图片控件
 */
lv_obj_t * lv_png_create_from_file(lv_obj_t * parent, const char * path)
{   
    lv_obj_t *im = lv_img_create(parent);
    lv_img_set_src(im, path); 
    lv_obj_align_to(im,NULL,LV_ALIGN_CENTER,0,0);

    return im;
}


/**
 * @brief       LVGL程序入口
 * @param       无
 * @retval      无
 */
void lv_mainstart()
{
    lv_png_init(); /* 初始化PNG解码库 */
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label,"PNG_Decoder");
    lv_obj_align_to(label,NULL,LV_ALIGN_TOP_MID,0,0);

    img = lv_png_create_from_file(lv_scr_act(),PNG_PATH[0].img_path); /* 创建PNG文件 */
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
