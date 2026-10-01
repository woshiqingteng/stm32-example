/**
 * @file    main.c
 * @brief   lvgl_53_comprehensive: ALIENTEK LVGL comprehensive demo (launcher
 *          with calculator, file manager, base converter, settings, board
 *          test, clock, QR and paint sub-apps).
 */

#include <stdio.h>

#include "bsp.h"
#include "sdram.h"
#include "exfuns.h"
#include "ff.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"
#include "lv_port.h"

#include "gui/lv_mainstart.h"
#include "gui/image.h"

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

    /* Load the launcher icon library from SD into SPI-NOR when missing. */
    if (images_init() != 0)
    {
        (void)images_update_image(0, 0, 16, (uint8_t *)"0:", 0xFFFF);
    }

    xTaskCreate(lvgl_task, "lvgl", LVGL_TASK_STK_SIZE, NULL, LVGL_TASK_PRIO, NULL);
    xTaskCreate(led_task, "led", LED_TASK_STK_SIZE, NULL, LED_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
