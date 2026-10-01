/**
 * @file    main.c
 * @brief   lvgl_40_img_lib: LVGL images read from SPI-NOR (ALIENTEK experiment 40).
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"
#include "lv_port.h"

#include "nor.h"
#include "exfuns.h"
#include "ff.h"
#include "image.h"

#define LVGL_TASK_PRIO     3
#define LVGL_TASK_STK_SIZE 1024
#define LED_TASK_PRIO      4
#define LED_TASK_STK_SIZE  128

/* ===== ported from ALIENTEK lv_mainstart.c ===== */

static uint8_t lv_load_img(lv_img_dsc_t *image, uint32_t addr, uint32_t size)
{
    uint8_t *image_buffer;
    uint32_t image_header;
    uint8_t *image_jpeg;
    uint32_t off = 0;

    /* The image is shown permanently, so the buffer is not released. */
    image_buffer = lv_mem_alloc(size);
    if (image_buffer == NULL)
    {
        return 1;
    }

    while (off < size)
    {
        uint16_t chunk = (uint16_t)(((size - off) > 4096U) ? 4096U : (size - off));
        nor_read(image_buffer + off, addr + off, chunk);
        off += chunk;
    }

    /* 4-byte little-endian header: cf | (w << 8) | (h << 20). */
    image_header = (uint32_t)image_buffer[3] << 24;
    image_header |= (uint32_t)image_buffer[2] << 16;
    image_header |= (uint32_t)image_buffer[1] << 8;
    image_header |= (uint32_t)image_buffer[0];

    image_jpeg = (uint8_t *)image_buffer + 4;

    image->header.cf = (lv_img_cf_t)(image_header & 0xFFU);
    image->header.always_zero = 0;
    image->header.w = (uint16_t)((image_header >> 8) & 0xFFFU);
    image->header.h = (uint16_t)((image_header >> 20) & 0xFFFU);
    image->data_size = size - 4;
    image->data = image_jpeg;

    return 0;
}

static lv_img_dsc_t image_bin1;
static lv_obj_t *image;
static lv_img_dsc_t image_bin2;
static lv_obj_t *image1;
static lv_img_dsc_t image_bin3;
static lv_obj_t *image2;
static int image_number = 0;
static int zoom_factor_size = 128;

static void lv_my_timer(lv_timer_t *t)
{
    uint32_t *user_data = t->user_data;

    if (*user_data == 0)
    {
        image_number = image_number + 100;
        lv_img_set_angle(image, image_number);
        lv_img_set_antialias(image, true);
    }

    if (image_number >= 3600 || *user_data == 1)
    {
        *user_data = 1;
        lv_img_set_zoom(image, zoom_factor_size);
        zoom_factor_size = zoom_factor_size * 2;

        if (zoom_factor_size >= 512)
        {
            zoom_factor_size = 128;
            image_number = 0;
            *user_data = 0;
        }
    }
}

static void lv_mainstart(void)
{
    static uint32_t user_data = 0;
    int image_flag;

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
    image_flag = lv_load_img(&image_bin1, g_ftinfo.lvgl_atk01addr, g_ftinfo.lvgl_atk01size);
    image_flag = lv_load_img(&image_bin2, g_ftinfo.lvgl_atk03addr, g_ftinfo.lvgl_atk03size);
    image_flag = lv_load_img(&image_bin3, g_ftinfo.lvgl_moneyaddr, g_ftinfo.lvgl_moneysize);

    if (image_flag == 0)
    {
        image = lv_img_create(lv_scr_act());
        lv_img_set_src(image, &image_bin1);
        lv_obj_align(image, LV_ALIGN_CENTER, 0, 0);
        lv_img_set_pivot(image, image_bin1.header.w / 2, image_bin1.header.h / 2);

        image1 = lv_img_create(lv_scr_act());
        lv_img_set_src(image1, &image_bin2);
        lv_obj_align(image1, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_img_set_pivot(image1, image_bin2.header.w / 2, image_bin2.header.h / 2);

        image2 = lv_img_create(lv_scr_act());
        lv_img_set_src(image2, &image_bin3);
        lv_obj_align(image2, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_img_set_pivot(image2, image_bin3.header.w / 2, image_bin3.header.h / 2);

        lv_timer_create(lv_my_timer, 1000, &user_data);
    }
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

    (void)exfuns_init();
    (void)f_mount(fs[0], "0:", 1);

    /* Copy the image library from the SD card to the SPI NOR when it is
     * missing, mirroring the ALIENTEK example. */
    if (images_init() != 0)
    {
        (void)images_update_image(0, 0, 16, (uint8_t *)"0:", 0xFFFF);
    }

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
