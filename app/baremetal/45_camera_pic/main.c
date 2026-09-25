/**
 * @file    main.c
 * @brief   45_camera_pic: OV5640 RGB565 live view plus JPEG capture. The DCMI
 *          streams the sensor into the LTDC frame buffer; KEY0 encodes the
 *          visible frame to a JPEG file on the SD card, KEY1 decodes and shows
 *          the last capture with the PICTURE middleware, KEY2 pauses the live
 *          view and WK_UP triggers a single auto-focus.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "text.h"
#include "piclib.h"
#include "jpeg_dec.h"

#define CAM_OUT_WIDTH    800U
#define CAM_OUT_HEIGHT   464U
#define CAM_TOP          16U
#define CAM_FPS_INVALID  0xFFFFFFFFU

#define STATUS_X         4U
#define STATUS_WIDTH     700U
#define STATUS_HEIGHT    16U

#define CAM_LOOP_MS      20U
#define CAM_KEY_DELAY_MS 200U
#define PHOTO_DIR        "0:/PHOTO"

static volatile bool g_paused;
static char          g_last_path[32];

static uint32_t         g_line_buf[2][CAM_OUT_WIDTH / 2U];
static volatile uint16_t g_cam_curline;

static void cam_line_cb(void)
{
    uint16_t *pbuf;

    if ((g_dma_dcmi_handle.Instance->CR & DMA_SxCR_CT) != 0U)
    {
        pbuf = (uint16_t *)g_line_buf[0];
    }
    else
    {
        pbuf = (uint16_t *)g_line_buf[1];
    }

    if (g_cam_curline < (uint16_t)(CAM_TOP + CAM_OUT_HEIGHT))
    {
        lcd_color_fill(0U, g_cam_curline, (uint16_t)(CAM_OUT_WIDTH - 1U), g_cam_curline, pbuf);
        g_cam_curline++;
    }
}

static void cam_frame_cb(void)
{
    g_cam_curline = CAM_TOP;
    timer_frame_inc();
    led_toggle(LED1);
}

static void cam_status(uint32_t fps, bool sd_ok)
{
    char line[64];

    (void)sprintf(line, "OV5640 %ux%u FPS:%u SD:%s%s",
                  (unsigned int)CAM_OUT_WIDTH, (unsigned int)CAM_OUT_HEIGHT,
                  (unsigned int)fps, sd_ok ? "OK" : "ERR",
                  g_paused ? " PAUSE" : "");

    lcd_fill(STATUS_X, 0U, (uint16_t)(STATUS_X + STATUS_WIDTH), STATUS_HEIGHT - 1U, BLACK);
    lcd_show_string(STATUS_X, 0U, STATUS_WIDTH, STATUS_HEIGHT, LCD_FONT_SIZE_16, line, GREEN);
}

static void cam_next_path(char *path)
{
    uint16_t index;
    FIL      f;

    for (index = 0U; index < 9999U; index++)
    {
        (void)sprintf(path, PHOTO_DIR "/PIC%05u.jpg", (unsigned int)index);

        if (f_open(&f, path, FA_READ) == FR_NO_FILE)
        {
            break;
        }

        (void)f_close(&f);
    }
}

static uint8_t cam_save_jpeg(bool sd_ok)
{
    uint8_t res;

    if (!sd_ok)
    {
        printf("no SD card, cannot save\r\n");
        return 1U;
    }

    dcmi_stop();
    cam_next_path(g_last_path);
    res = jpg_encode(g_last_path, 0U, CAM_TOP, CAM_OUT_WIDTH, CAM_OUT_HEIGHT);
    dcmi_start();

    if (res != 0U)
    {
        printf("jpeg encode failed (%u)\r\n", (unsigned int)res);
    }
    else
    {
        printf("saved %s\r\n", g_last_path);
    }

    return res;
}

static void cam_show_jpeg(bool sd_ok)
{
    if (!sd_ok || (g_last_path[0] == '\0'))
    {
        printf("no saved picture\r\n");
        return;
    }

    dcmi_stop();
    lcd_clear(BLACK);
    (void)piclib_ai_load_picfile(g_last_path, 0U, 0U, lcd_get_width(), lcd_get_height(), true);
    text_show_string(2U, 2U, lcd_get_width(), 16U, g_last_path, 16U, 1U, RED);
    delay_ms(2000U);
    lcd_clear(BLACK);
    dcmi_start();
}

int main(void)
{
    uint32_t fps = 0U;
    uint32_t last_fps = CAM_FPS_INVALID;
    bool     sd_ok = false;
    FRESULT  res;

    bsp_init();
    sdram_init();
    lcd_init();
    lcd_clear(BLACK);
    piclib_init();

    printf("45_camera_pic ready\r\n");

    if (sdio_init() != 0U)
    {
        printf("SD init failed\r\n");
    }
    else
    {
        (void)exfuns_init();

        if (f_mount(fs[0], "0:", 1) != FR_OK)
        {
            printf("mount failed\r\n");
        }
        else
        {
            res = f_mkdir(PHOTO_DIR);
            sd_ok = ((res == FR_OK) || (res == FR_EXIST));
            printf("SD mounted, PHOTO dir %s\r\n", sd_ok ? "ok" : "error");
        }
    }

    (void)pcf8574_init();

    while (ov5640_init() != 0U)
    {
        lcd_show_string(30U, 30U, 240U, STATUS_HEIGHT, LCD_FONT_SIZE_16, "OV5640 ERROR", RED);
        printf("OV5640 error\r\n");
        delay_ms(200U);
        lcd_fill(30U, 30U, 269U, 45U, BLACK);
        delay_ms(200U);
        led_toggle(LED0);
    }

    printf("OV5640 id: %04X\r\n", (unsigned int)ov5640_read_id());

    ov5640_rgb565_mode();
    ov5640_light_mode(0U);
    ov5640_color_saturation(3U);
    ov5640_brightness(4U);
    ov5640_contrast(3U);
    ov5640_sharpness(33U);
    (void)ov5640_focus_init();
    (void)ov5640_focus_constant();

    dcmi_init();
    dcmi_rx_callback    = cam_line_cb;
    dcmi_frame_callback = cam_frame_cb;
    dcmi_dma_init((uint32_t)g_line_buf[0], (uint32_t)g_line_buf[1],
                  (uint16_t)(CAM_OUT_WIDTH / 2U), DMA_MDATAALIGN_HALFWORD, DMA_MINC_ENABLE);

    g_cam_curline = CAM_TOP;
    (void)ov5640_outsize_set(4U, 0U, CAM_OUT_WIDTH, CAM_OUT_HEIGHT);

    timer_init();
    cam_status(0U, sd_ok);
    dcmi_start();

    for (;;)
    {
        key_id_t key = key_scan(false);

        if (key == KEY0)
        {
            (void)cam_save_jpeg(sd_ok);
        }
        else if (key == KEY1)
        {
            cam_show_jpeg(sd_ok);
        }
        else if (key == KEY2)
        {
            g_paused = !g_paused;

            if (g_paused)
            {
                dcmi_stop();
            }
            else
            {
                dcmi_start();
            }
        }
        else if (key == KEY_WKUP)
        {
            (void)ov5640_focus_single();
        }
        else
        {
            /* no key */
        }

        fps = timer_frame_rate();

        if (fps != last_fps)
        {
            last_fps = fps;
            cam_status(fps, sd_ok);
            printf("FPS:%u\r\n", (unsigned int)fps);
        }

        delay_ms(CAM_LOOP_MS);
    }
}
