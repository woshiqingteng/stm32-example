/**
 * @file    main.c
 * @brief   38_camera_stream: OV5640 camera (vendor experiment 38).
 *
 * Two modes, selected with the keys at boot:
 *   KEY0 -> RGB565 live view on the RGB panel.
 *   KEY1 -> JPEG capture streamed over USART2 (921600 baud).
 *
 * The DCMI is driven with a double-buffered line DMA; the application provides
 * the transfer-complete hook (dcmi_rx_callback) and the frame (VSYNC) hook
 * (dcmi_frame_callback), exactly like the vendor code. Frame rate is counted
 * with the basic timer (1 Hz).
 */

#include <stdbool.h>
#include <stdio.h>

#include "bsp.h"
#include "btim.h"
#include "dcmi.h"
#include "lcd.h"
#include "ov5640.h"
#include "sdram.h"
#include "ltdc.h"

/** @brief  Current camera mode. */
typedef enum
{
    CAM_MODE_RGB565 = 0,
    CAM_MODE_JPEG = 1
} cam_mode_t;

cam_mode_t g_cam_mode = CAM_MODE_RGB565;
uint16_t   g_curline = 0;                 /* current capture line (RGB mode) */
uint16_t   g_yoffset = 0;                 /* vertical offset (RGB mode) */

#define JPEG_BUF_WORD_COUNT   (1U * 1024U * 1024U)               /* 4 MB JPEG buffer (words) */
#define RGB_LINE_WORD_COUNT   (LTDC_PANEL_WIDTH_PX / 2U)            /* per-line DMA buffer (words) */
#define CAM_OUTSIZE_OFFSET_X 4U                              /* sensor output window X offset */

/* The RGB565 panel frame buffer occupies the start of SDRAM; the JPEG capture
 * buffer is placed right after it. */
#define JPEG_BUF_ADDR    (LTDC_FRAME_BUF_ADDR + \
                          ((uint32_t)LTDC_PANEL_WIDTH_PX * LTDC_PANEL_HEIGHT_PX * 2U))

static uint32_t g_dcmi_line_buf[2][RGB_LINE_WORD_COUNT];

static const uint16_t jpeg_img_size_tbl[][2] =
{
    { 160, 120 }, { 320, 240 }, { 640, 480 }, { 800, 600 },
    { 1024, 768 }, { 1280, 800 }, { 1440, 900 }, { 1280, 1024 },
    { 1600, 1200 }, { 1920, 1080 }, { 2048, 1536 }, { 2592, 1944 },
};

static const char *const EFFECTS_TBL[7] =
{ "Normal", "Cool", "Warm", "B&W", "Yellowish", "Inverse", "Greenish" };

static const char *const JPEG_SIZE_TBL[12] =
{ "QQVGA", "QVGA", "VGA", "SVGA", "XGA", "WXGA", "WXGA+", "SXGA", "UXGA", "1080P", "QXGA", "500W" };

/* ---- Application DCMI hooks ------------------------------------------------ */

static void rgb_frame_cb(void)
{
    g_curline = g_yoffset;
}

void rgblcd_dcmi_rx_callback(void)
{
    uint16_t *pbuf;

    if ((g_dma_dcmi_handle.Instance->CR & DMA_SxCR_CT) != 0U)
    {
        pbuf = (uint16_t *)g_dcmi_line_buf[0];
    }
    else
    {
        pbuf = (uint16_t *)g_dcmi_line_buf[1];
    }

    if (g_curline < lcd_get_height())
    {
        lcd_color_fill(0U, g_curline, (uint16_t)(lcd_get_width() - 1U), g_curline, pbuf);
        g_curline++;
    }
}

static void fps_cb(void)
{
    printf("frame:%u\r\n", (unsigned int)g_dcmi_frame_count);
    g_dcmi_frame_count = 0U;
}

/* ---- JPEG test (capture + USART2 transfer) --------------------------------- */

static void jpeg_test(void)
{
    key_id_t key;
    uint8_t *p;
    uint8_t  headok;
    uint32_t i;
    uint32_t jpgstart;
    uint32_t jpglen;
    uint8_t  effect   = 0U;
    uint8_t  contrast = 2U;
    uint8_t  size     = 1U;                                 /* QVGA */
    char     msg[24];

    lcd_clear(WHITE);
    lcd_show_string(30, 50, 200, 16, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, LCD_FONT_SIZE_16, "OV5640 JPEG Mode", RED);
    lcd_show_string(30, 100, 200, 16, LCD_FONT_SIZE_16, "KEY0:Contrast", RED);
    lcd_show_string(30, 120, 200, 16, LCD_FONT_SIZE_16, "KEY1:AUTO Focus", RED);
    lcd_show_string(30, 140, 200, 16, LCD_FONT_SIZE_16, "KEY2:Effect", RED);
    lcd_show_string(30, 160, 200, 16, LCD_FONT_SIZE_16, "KEY_UP:Size", RED);
    (void)sprintf(msg, "JPEG Size:%s", JPEG_SIZE_TBL[size]);
    lcd_show_string(30, 180, 200, 16, LCD_FONT_SIZE_16, msg, RED);

    ov5640_rgb565_mode();
    (void)ov5640_focus_init();
    ov5640_jpeg_mode();
    ov5640_light_mode(0U);
    ov5640_color_saturation(3U);
    ov5640_brightness(4U);
    ov5640_contrast(3U);
    ov5640_sharpness(33U);
    (void)ov5640_focus_constant();

    dcmi_init();
    dcmi_jpeg_init((uint32_t *)JPEG_BUF_ADDR, JPEG_BUF_WORD_COUNT);
    (void)ov5640_outsize_set(CAM_OUTSIZE_OFFSET_X, 0U, jpeg_img_size_tbl[size][0],
                             jpeg_img_size_tbl[size][1]);
    dcmi_jpeg_begin();

    for (;;)
    {
        if (dcmi_jpeg_state() == DCMI_JPEG_READY)                         /* a whole frame is ready */
        {
            dcmi_jpeg_end();
            jpglen = 0U;

            p = (uint8_t *)JPEG_BUF_ADDR;
            printf("g_jpeg_data_len:%u\r\n", (unsigned int)(dcmi_jpeg_words() * 4U));
            lcd_show_string(30, 210, 210, 16, LCD_FONT_SIZE_16, "Sending JPEG data...", RED);

            headok  = 0U;

            for (i = 0U; i < (dcmi_jpeg_words() * 4U); i++)
            {
                if ((p[i] == 0xFFU) && (p[i + 1U] == 0xD8U))
                {
                    jpgstart = i;
                    headok   = 1U;
                }
                if ((p[i] == 0xFFU) && (p[i + 1U] == 0xD9U) && (headok != 0U))
                {
                    jpglen = i - jpgstart + 2U;
                    break;
                }
            }

            key = KEY_NONE;

            if (jpglen != 0U)                               /* an entire JPEG frame was found */
            {
                p += jpgstart;

                for (i = 0U; i < jpglen; i++)
                {
                    (void)usart_write(USART_ID_2, &p[i], 1U);
                    key = key_scan(false);
                    if (key != KEY_NONE)
                    {
                        break;                              /* a key aborts the transfer */
                    }
                }
            }

            if (key != KEY_NONE)
            {
                lcd_show_string(30, 210, 210, 16, LCD_FONT_SIZE_16, "Quit Sending data", RED);

                switch (key)
                {
                    case KEY0:
                        contrast++;
                        if (contrast > 6U)
                        {
                            contrast = 0U;
                        }
                        ov5640_contrast(contrast);
                        (void)sprintf(msg, "Contrast:%d", (signed char)contrast - 3);
                        break;

                    case KEY1:
                        (void)ov5640_focus_single();
                        break;

                    case KEY2:
                        effect++;
                        if (effect > 6U)
                        {
                            effect = 0U;
                        }
                        ov5640_special_effects(effect);
                        (void)sprintf(msg, "%s", EFFECTS_TBL[effect]);
                        break;

                    case KEY_WKUP:
                        size++;
                        if (size > 11U)
                        {
                            size = 0U;
                        }
                        (void)ov5640_outsize_set(16U, 4U, jpeg_img_size_tbl[size][0],
                                                 jpeg_img_size_tbl[size][1]);
                        (void)sprintf(msg, "JPEG Size:%s", JPEG_SIZE_TBL[size]);
                        break;

                    default:
                        break;
                }

                lcd_fill(30, 180, 239, 196, WHITE);
                lcd_show_string(30, 180, 210, 16, LCD_FONT_SIZE_16, msg, RED);
                delay_ms(800);
            }
            else
            {
                lcd_show_string(30, 210, 210, 16, LCD_FONT_SIZE_16, "Send data complete!!", RED);
            }

            dcmi_jpeg_begin();                            /* allow the next frame */
        }

        delay_ms(1);
    }
}

/* ---- RGB565 test ----------------------------------------------------------- */

static void rgb565_test(void)
{
    key_id_t key;
    uint8_t  effect   = 0U;
    uint8_t  contrast = 2U;
    uint8_t  scale    = 1U;
    uint16_t outputheight;

    lcd_clear(WHITE);
    lcd_show_string(30, 50, 200, 16, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, LCD_FONT_SIZE_16, "OV5640 RGB565 Mode", RED);
    lcd_show_string(30, 100, 200, 16, LCD_FONT_SIZE_16, "KEY0:Contrast", RED);
    lcd_show_string(30, 120, 200, 16, LCD_FONT_SIZE_16, "KEY1:AUTO Focus", RED);
    lcd_show_string(30, 140, 200, 16, LCD_FONT_SIZE_16, "KEY2:Effects", RED);
    lcd_show_string(30, 160, 200, 16, LCD_FONT_SIZE_16, "KEY_UP:FullSize/Scale", RED);

    ov5640_rgb565_mode();
    (void)ov5640_focus_init();
    ov5640_light_mode(0U);
    ov5640_color_saturation(3U);
    ov5640_brightness(4U);
    ov5640_contrast(3U);
    ov5640_sharpness(33U);
    (void)ov5640_focus_constant();

    dcmi_init();
    dcmi_rx_callback    = &rgblcd_dcmi_rx_callback;
    dcmi_frame_callback = &rgb_frame_cb;
    dcmi_dma_init((uint32_t)g_dcmi_line_buf[0], (uint32_t)g_dcmi_line_buf[1],
                  (uint16_t)(lcd_get_width() / 2U), DMA_MDATAALIGN_HALFWORD, DMA_MINC_ENABLE);

    g_yoffset   = 0U;
    outputheight = lcd_get_height();
    g_curline    = g_yoffset;

    (void)ov5640_outsize_set(4U, 0U, lcd_get_width(), outputheight);
    dcmi_start();

    lcd_clear(WHITE);

    for (;;)
    {
        key = key_scan(false);

        if (key != KEY_NONE)
        {
            if (key != KEY1)
            {
                dcmi_stop();
            }

            switch (key)
            {
                case KEY0:
                    contrast++;
                    if (contrast > 6U)
                    {
                        contrast = 0U;
                    }
                    ov5640_contrast(contrast);
                    break;

                case KEY1:
                    (void)ov5640_focus_single();
                    break;

                case KEY2:
                    effect++;
                    if (effect > 6U)
                    {
                        effect = 0U;
                    }
                    ov5640_special_effects(effect);
                    break;

                case KEY_WKUP:
                    scale = (uint8_t)(scale == 0U ? 1U : 0U);
                    if (scale == 0U)
                    {
                        (void)ov5640_outsize_set(0U, 0U, lcd_get_width(), outputheight);
                    }
                    else
                    {
                        (void)ov5640_outsize_set(4U, 0U, lcd_get_width(), outputheight);
                    }
                    break;

                default:
                    break;
            }

            if (key != KEY1)
            {
                delay_ms(800);
                dcmi_start();
            }
        }

        delay_ms(10);
    }
}

int main(void)
{
    key_id_t key;
    uint16_t t = 0U;

    bsp_init();
    printf(APP_BANNER "\r\n");
    {
        usart_cfg_t cfg = { USART_CFG_DEFAULT(USART_ID_2) };

        cfg.baudrate = 921600U;
        usart_init(&cfg);
    }
    sdram_init();
    lcd_init();

    /* 90 MHz / (9000 * 10000) = 1 Hz -> per-second FPS */
    btim_timx_int_init(10000U - 1U, 9000U - 1U);
    btim_timx_int_register(&fps_cb);

    lcd_show_string(30, 50, 200, 16, LCD_FONT_SIZE_16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, LCD_FONT_SIZE_16, "OV5640 TEST", RED);
    lcd_show_string(30, 90, 200, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", RED);

    while (ov5640_init() != 0U)
    {
        lcd_show_string(30, 130, 240, 16, LCD_FONT_SIZE_16, "OV5640 ERROR", RED);
        printf("OV5640 error\r\n");
        delay_ms(200);
        lcd_fill(30, 130, 239, 150, WHITE);
        delay_ms(200);
        led_toggle(LED0);
    }

    lcd_show_string(30, 130, 200, 16, LCD_FONT_SIZE_16, "OV5640 OK", RED);

    for (;;)
    {
        key = key_scan(false);

        if (key == KEY0)
        {
            g_cam_mode = CAM_MODE_RGB565;
            break;
        }
        else if (key == KEY1)
        {
            g_cam_mode = CAM_MODE_JPEG;
            break;
        }

        t++;

        if (t == 100U)
        {
            lcd_show_string(30, 150, 230, 16, LCD_FONT_SIZE_16, "KEY0:RGB565  KEY1:JPEG", RED);
        }

        if (t == 200U)
        {
            lcd_fill(30, 150, 210, 166, WHITE);
            t = 0U;
            led_toggle(LED0);
        }

        delay_ms(5);
    }

    if (g_cam_mode == CAM_MODE_JPEG)
    {
        jpeg_test();
    }
    else
    {
        rgb565_test();
    }

    for (;;)
    {
    }
}
