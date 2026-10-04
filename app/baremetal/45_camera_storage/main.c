/**
 * @file    main.c
 * @brief   45_camera_storage: OV5640 RGB565 live view plus photo capture.
 *          KEY0 saves a BMP of the current frame, KEY1 captures a sensor-native
 *          500W JPEG (truncated to SOI/EOI) into 0:/PHOTO, KEY2 triggers a
 *          single auto-focus, WK_UP replays the last JPEG. The DCMI/SDIO shared
 *          pins are muxed around each card access.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "dcmi.h"
#include "tim.h"
#include "io_expand.h"
#include "lcd.h"
#include "ov5640.h"
#include "sdio.h"
#include "sdram.h"
#include "ff.h"
#include "exfuns.h"
#include "text.h"
#include "piclib.h"
#include "jpeg_dec.h"
#include "bmp.h"

#define CAM_OUT_WIDTH_PIXEL    800U
#define CAM_OUT_HEIGHT_PIXEL   464U
#define CAM_TOP          16U
#define CAM_FPS_INVALID  0xFFFFFFFFU

#define STATUS_X         4U
#define STATUS_WIDTH_PIXEL     700U
#define STATUS_HEIGHT_PIXEL    16U

#define CAM_LOOP_MS      20U
#define CAM_OUTSIZE_OFFSET_X 4U   /* sensor output window X offset */
#define PHOTO_DIR        "0:/PHOTO"

#define JPEG_SIZE_WIDTH_PIXEL      2592U
#define JPEG_SIZE_HEIGHT_PIXEL      1944U
#define JPEG_BUF_ADDR    (lcd_info()->framebuf + \
                          ((uint32_t)LCD_WIDTH_PX * LCD_HEIGHT_PX * 2U))
#define JPEG_BUF_WORD_COUNT   (1U * 1024U * 1024U)   /* 4 MB capture buffer */
#define JPEG_CAPTURE_TIMEOUT_MS 3000U

static volatile bool g_paused;
static char          g_last_path[32];

static uint32_t         g_line_buf[2][CAM_OUT_WIDTH_PIXEL / 2U];
static volatile uint16_t g_cam_curline;

/* 1 Hz frame-rate counter (moved out of the timer driver). */
static volatile uint32_t g_frame_count;
static volatile uint32_t g_frame_rate;

static void frame_tick(void)
{
    g_frame_rate  = g_frame_count;
    g_frame_count = 0U;
    printf("frame:%u\r\n", (unsigned int)g_frame_rate);
}

static void frame_inc(void)
{
    g_frame_count++;
}

static uint32_t frame_rate(void)
{
    return g_frame_rate;
}

static void frame_init(void)
{
    tim_cfg_t cfg = { TIM_CFG_DEFAULT };

    /* TIM14: 90 MHz / (9000 * 10000) = 1 Hz. */
    cfg.id        = TIM_ID_14;
    cfg.mode      = TIM_MODE_BASE;
    cfg.arr       = 10000U - 1U;
    cfg.psc       = 9000U - 1U;
    cfg.update_cb = &frame_tick;
    tim_init(&cfg);
}

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

    if (g_cam_curline < (uint16_t)(CAM_TOP + CAM_OUT_HEIGHT_PIXEL))
    {
        lcd_blit(0U, g_cam_curline, (uint16_t)(CAM_OUT_WIDTH_PIXEL - 1U), g_cam_curline, pbuf);
        g_cam_curline++;
    }
}

static void cam_frame_cb(void)
{
    g_cam_curline = CAM_TOP;
    frame_inc();
    led_toggle(LED1);
}

static void cam_status(uint32_t fps, bool sd_ok)
{
    char line[64];

    (void)sprintf(line, "OV5640 %ux%u FPS:%u SD:%s%s",
                  (unsigned int)CAM_OUT_WIDTH_PIXEL, (unsigned int)CAM_OUT_HEIGHT_PIXEL,
                  (unsigned int)fps, sd_ok ? "OK" : "ERR",
                  g_paused ? " PAUSE" : "");

    lcd_fill(STATUS_X, 0U, (uint16_t)(STATUS_X + STATUS_WIDTH_PIXEL), STATUS_HEIGHT_PIXEL - 1U, BLACK);
    lcd_show_string(STATUS_X, 0U, STATUS_WIDTH_PIXEL, STATUS_HEIGHT_PIXEL, LCD_FONT_SIZE_16, line, GREEN);
}

static void cam_next_path(char *path, const char *ext)
{
    uint16_t index;
    FIL      f;

    for (index = 0U; index < 9999U; index++)
    {
        (void)sprintf(path, PHOTO_DIR "/PIC%05u.%s", (unsigned int)index, ext);

        if (f_open(&f, path, FA_READ) == FR_NO_FILE)
        {
            break;
        }

        (void)f_close(&f);
    }
}

/* Locate the native JPEG inside the capture buffer (SOI 0xFFD8 .. EOI 0xFFD9)
 * and return its length; *start receives the SOI offset. 0 when not found. */
static uint32_t cam_jpeg_find(uint32_t *start)
{
    uint8_t *buf = (uint8_t *)JPEG_BUF_ADDR;
    uint32_t total = dcmi_jpeg_words() * 4U;
    uint32_t i;
    uint32_t s = 0U;

    for (i = 0U; (i + 1U) < total; i++)
    {
        if ((buf[i] == 0xFFU) && (buf[i + 1U] == 0xD8U))
        {
            s = i;
            break;
        }
    }

    for (i = s + 2U; (i + 1U) < total; i++)
    {
        if ((buf[i] == 0xFFU) && (buf[i + 1U] == 0xD9U))
        {
            *start = s;
            return (i + 2U - s);
        }
    }

    return 0U;
}

/* Sensor-native JPEG: switch the sensor to JPEG mode, capture one frame at
 * 500W into SDRAM, store it, then restore the RGB565 live view. */
static uint8_t cam_save_native_jpeg(bool sd_ok)
{
    FIL      f;
    UINT     bw = 0U;
    FRESULT  fr;
    bool     captured;

    if (!sd_ok)
    {
        printf("no SD card\r\n");
        return 1U;
    }

    dcmi_stop();
    dcmi_switch_sdcard();
    cam_next_path(g_last_path, "jpg");

    ov5640_jpeg_mode();
    (void)ov5640_outsize_set(CAM_OUTSIZE_OFFSET_X, 0U, JPEG_SIZE_WIDTH_PIXEL, JPEG_SIZE_HEIGHT_PIXEL);

    dcmi_init();
    dcmi_jpeg_init((uint32_t *)JPEG_BUF_ADDR, JPEG_BUF_WORD_COUNT);
    captured = dcmi_jpeg_capture(JPEG_CAPTURE_TIMEOUT_MS);

    fr = f_open(&f, g_last_path, FA_CREATE_ALWAYS | FA_WRITE);

    if (captured && (fr == FR_OK))
    {
        uint32_t start = 0U;
        uint32_t len   = cam_jpeg_find(&start);

        if (len != 0U)
        {
            (void)f_write(&f, (uint8_t *)(JPEG_BUF_ADDR + start), len, &bw);
        }

        (void)f_close(&f);
        printf("native jpeg %s %u bytes\r\n", g_last_path, (unsigned)bw);
    }
    else
    {
        if (fr == FR_OK)
        {
            (void)f_close(&f);
        }

        printf("jpeg capture failed\r\n");
        bw = 0U;
    }

    /* Restore the RGB565 live view. */
    ov5640_rgb565_mode();
    (void)ov5640_outsize_set(CAM_OUTSIZE_OFFSET_X, 0U, CAM_OUT_WIDTH_PIXEL, CAM_OUT_HEIGHT_PIXEL);

    dcmi_init();
    dcmi_switch_ov5640();
    dcmi_rx_callback    = &cam_line_cb;
    dcmi_frame_callback = &cam_frame_cb;
    dcmi_dma_init((uint32_t)g_line_buf[0], (uint32_t)g_line_buf[1],
                  (uint16_t)(CAM_OUT_WIDTH_PIXEL / 2U), DMA_MDATAALIGN_HALFWORD, DMA_MINC_ENABLE);
    g_cam_curline = CAM_TOP;
    dcmi_start();

    return (bw == 0U) ? 1U : 0U;
}

static uint8_t cam_save_bmp(bool sd_ok)
{
    char path[32];

    if (!sd_ok)
    {
        printf("no SD card\r\n");
        return 1U;
    }

    dcmi_stop();
    dcmi_switch_sdcard();
    cam_next_path(path, "bmp");

    (void)bmp_encode((uint8_t *)path, 0U, CAM_TOP, CAM_OUT_WIDTH_PIXEL, CAM_OUT_HEIGHT_PIXEL, 0);

    dcmi_switch_ov5640();
    dcmi_start();

    printf("saved %s\r\n", path);
    return 0U;
}

static void cam_show_jpeg(bool sd_ok)
{
    if (!sd_ok || (g_last_path[0] == '\0'))
    {
        printf("no saved picture\r\n");
        return;
    }

    dcmi_stop();
    dcmi_switch_sdcard();
    lcd_clear(BLACK);
    (void)piclib_ai_load_picfile(g_last_path, 0U, 0U, lcd_info()->width, lcd_info()->height, true);
    text_show_string(2U, 2U, lcd_info()->width, 16U, g_last_path, 16U, 1U, RED);
    delay_ms(2000U);
    lcd_clear(BLACK);
    dcmi_switch_ov5640();
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
    (void)fonts_init();

    printf(APP_BANNER "\r\n");

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

    (void)io_expand_init();

    while (ov5640_init() != 0U)
    {
        lcd_show_string(30U, 30U, 240U, STATUS_HEIGHT_PIXEL, LCD_FONT_SIZE_16, "OV5640 ERROR", RED);
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
    dcmi_rx_callback    = &cam_line_cb;
    dcmi_frame_callback = &cam_frame_cb;
    dcmi_dma_init((uint32_t)g_line_buf[0], (uint32_t)g_line_buf[1],
                  (uint16_t)(CAM_OUT_WIDTH_PIXEL / 2U), DMA_MDATAALIGN_HALFWORD, DMA_MINC_ENABLE);

    g_cam_curline = CAM_TOP;
    (void)ov5640_outsize_set(4U, 0U, CAM_OUT_WIDTH_PIXEL, CAM_OUT_HEIGHT_PIXEL);

    frame_init();
    cam_status(0U, sd_ok);
    dcmi_start();

    for (;;)
    {
        key_id_t key = key_scan(false);

        if (key == KEY0)
        {
            (void)cam_save_bmp(sd_ok);
        }
        else if (key == KEY1)
        {
            (void)cam_save_native_jpeg(sd_ok);
        }
        else if (key == KEY2)
        {
            (void)ov5640_focus_single();
        }
        else if (key == KEY_WKUP)
        {
            cam_show_jpeg(sd_ok);
        }
        fps = frame_rate();

        if (fps != last_fps)
        {
            last_fps = fps;
            cam_status(fps, sd_ok);
            printf("FPS:%u\r\n", (unsigned int)fps);
        }

        delay_ms(CAM_LOOP_MS);
    }
}
