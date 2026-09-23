/**
 * @file    main.c
 * @brief   38_camera: OV5640 RGB565 live view on the RGB panel (mirrors vendor
 *          experiment 38, RGB565 path).
 *
 * The sensor is configured for 800x464 RGB565 and the DCMI DMA writes each
 * line into the LTDC framebuffer at row CAM_TOP, leaving the top 16 pixel rows
 * free for a live status line (resolution and frame rate). Keys change the
 * contrast / effect / focus and pause the capture.
 */

#include <stdbool.h>
#include <stdio.h>
#include "bsp.h"

#define CAM_OUT_WIDTH    800U
#define CAM_OUT_HEIGHT   464U
#define CAM_TOP          16U
#define CAM_FPS_INVALID  0xFFFFFFFFU

#define STATUS_X         4U
#define STATUS_WIDTH     700U
#define STATUS_HEIGHT    16U


#define CAM_KEY_DELAY_MS 200U
#define CAM_LOOP_MS      20U

static volatile bool g_paused;

static void cam_frame_cb(void)
{
    timer_frame_inc();
    led_toggle(LED1);
}

static void cam_show_status(uint32_t fps, uint8_t contrast, uint8_t effect)
{
    char line[64];

    (void)sprintf(line, "OV5640 RGB565 %ux%u FPS:%u C:%u E:%u%s",
                  (unsigned int)CAM_OUT_WIDTH, (unsigned int)CAM_OUT_HEIGHT,
                  (unsigned int)fps, (unsigned int)contrast, (unsigned int)effect,
                  g_paused ? " PAUSE" : "");

    lcd_fill(STATUS_X, 0U, (uint16_t)(STATUS_X + STATUS_WIDTH), STATUS_HEIGHT - 1U, BLACK);
    lcd_show_string(STATUS_X, 0U, STATUS_WIDTH, STATUS_HEIGHT, LCD_FONT_SIZE_16, line, GREEN);
}

int main(void)
{
    uint32_t fps = 0U;
    uint32_t last_fps = CAM_FPS_INVALID;
    uint8_t contrast = 2U;
    uint8_t effect = 0U;
    bool    redraw = true;
    uint16_t id;

    bsp_init();
    sdram_init();
    lcd_init();
    lcd_clear(BLACK);

    printf("38_camera ready\r\n");

    while (ov5640_init() != 0U)
    {
        lcd_show_string(30U, 30U, 240U, STATUS_HEIGHT, LCD_FONT_SIZE_16, "OV5640 ERROR", RED);
        printf("OV5640 error\r\n");
        delay_ms(200U);
        lcd_fill(30U, 30U, 269U, 45U, BLACK);
        delay_ms(200U);
        led_toggle(LED0);
    }

    id = ov5640_read_id();
    printf("OV5640 id: %04X\r\n", (unsigned int)id);

    ov5640_rgb565_mode();
    ov5640_light_mode(0U);
    ov5640_color_saturation(3U);
    ov5640_brightness(4U);
    ov5640_contrast(3U);
    ov5640_sharpness(33U);
    (void)ov5640_focus_init();
    (void)ov5640_focus_constant();

    dcmi_init((uint16_t *)LTDC_FRAME_BUF_ADDR, lcd_get_width(), lcd_get_height());
    dcmi_config(0U, CAM_TOP, CAM_OUT_WIDTH, CAM_OUT_HEIGHT);
    dcmi_register_frame_callback(cam_frame_cb);

    (void)ov5640_outsize_set(4U, 0U, CAM_OUT_WIDTH, CAM_OUT_HEIGHT);

    timer_init();
    cam_show_status(0U, contrast, effect);
    dcmi_start();

    for (;;)
    {
        key_id_t key = key_scan(0);

        if (key == KEY0)
        {
            contrast++;
            if (contrast > 6U)
            {
                contrast = 0U;
            }

            dcmi_stop();
            ov5640_contrast(contrast);
            delay_ms(CAM_KEY_DELAY_MS);
            dcmi_start();
            redraw = true;
        }
        else if (key == KEY1)
        {
            (void)ov5640_focus_single();
        }
        else if (key == KEY2)
        {
            effect++;
            if (effect > 6U)
            {
                effect = 0U;
            }

            dcmi_stop();
            ov5640_special_effects(effect);
            delay_ms(CAM_KEY_DELAY_MS);
            dcmi_start();
            redraw = true;
        }
        else if (key == KEY_WKUP)
        {
            if (!g_paused)
            {
                g_paused = true;
                dcmi_stop();
            }
            else
            {
                g_paused = false;
                dcmi_start();
            }
            redraw = true;
        }
        else
        {
            /* no key pressed */
        }

        fps = timer_frame_rate();

        if ((fps != last_fps) || redraw)
        {
            last_fps = fps;
            redraw = false;
            cam_show_status(fps, contrast, effect);
            printf("FPS:%u\r\n", (unsigned int)fps);
        }

        delay_ms(CAM_LOOP_MS);
    }
}
