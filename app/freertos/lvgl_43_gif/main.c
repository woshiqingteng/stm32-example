/**
 * @file    main.c
 * @brief   lvgl_43_gif: LVGL gif demo (ALIENTEK experiment 43).
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


typedef struct
{
    char *img_path;
    char *label_text;
}img_info_t;

const img_info_t GIF_PATH[] =
{
    {"0:/PICTURE/GIF/alientek.gif", "alientek.gif"},
};

#define image_mun (int)(sizeof(GIF_PATH)/sizeof(GIF_PATH[0]))
int image = 0;
lv_obj_t *img;
int has_next = 1;  /* LVGL's lv_gif loops by itself; keep the timer idle. */

void lv_my_timer(lv_timer_t *timer)
{ 
    if (has_next == 0)          /* 如果是最后一帧，那么切换GIF */
    {
        image++;
        lv_obj_del(img);        /* 删除前面的GIF */
      
        if (image >= image_mun) /* 判断GIF库包含的个数是否最大 */
        {
            image = 0;          /* 重新开始展示 */
            img = lv_gif_create(lv_scr_act());
            lv_gif_set_src(img, GIF_PATH[image].img_path);
        }
        else                    /* 如果不是最后的GIF */
        {
            img = lv_gif_create(lv_scr_act());
            lv_gif_set_src(img, GIF_PATH[image].img_path);
        }

        timer->user_data = img; /* 设置任务数据等于获取的图片数据 */

        lv_obj_align_to(img,NULL,LV_ALIGN_CENTER,0,0);
    }
}

void lv_mainstart(void)
{
    lv_obj_t *label;
    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label,"GIF_Decoder");
    lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_RED),LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label,&lv_font_montserrat_32,LV_STATE_DEFAULT);
    lv_obj_align_to(label,NULL,LV_ALIGN_TOP_MID,0,0);
    lv_obj_set_style_bg_color(lv_scr_act(),lv_palette_main(LV_PALETTE_BLUE),LV_STATE_DEFAULT);
  
    lv_obj_set_style_bg_color(lv_scr_act(),lv_palette_main(LV_PALETTE_BLUE),LV_STATE_DEFAULT);
    img = lv_gif_create(lv_scr_act());
    lv_gif_set_src(img, GIF_PATH[image].img_path);
    lv_obj_align_to(img,NULL,LV_ALIGN_CENTER,0,0);
    
    lv_timer_create(lv_my_timer,10,img);
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
