/**
 * @file    main.c
 * @brief   43_font: GBK text display (aligned with the ALIENTEK experiment 43).
 *          The GBK font store is kept in the on-board NOR flash (copied from
 *          the SD card when missing). The whole GBK code range is swept
 *          automatically, showing each character at four sizes at once, with
 *          the lead/trail bytes and a running count. KEY0 refreshes the font
 *          store from the SD card; the other keys are unused.
 */

#include <stdio.h>

#include "bsp.h"
#include "lcd.h"
#include "sdio.h"
#include "sdram.h"
#include "ff.h"
#include "exfuns.h"
#include "text.h"

#define DRIVE             "0:"
#define FONT_SIZE_PIXEL   16U

/* GBK sweep range: lead 0x81..0xFE, trail 0x40..0xFD (0x7F skipped). */
#define GBK_LO_FIRST      0x81U
#define GBK_LO_END        0xFFU
#define GBK_HI_FIRST      0x40U
#define GBK_HI_END        0xFEU
#define GBK_HI_SKIP       0x7FU

/* On-screen layout (matches the reference example). */
#define TEXT_X            30U
#define TEXT_WIDTH_PX     200U

#define VAL_X             118U
#define HI_Y              110U
#define LO_Y              130U
#define CNT_Y             150U

#define CHAR32_X          (TEXT_X + 176U)
#define CHAR32_Y          200U
#define CHAR24_X          (TEXT_X + 132U)
#define CHAR24_Y          232U
#define CHAR16_X          (TEXT_X + 144U)
#define CHAR16_Y          256U
#define CHAR12_X          (TEXT_X + 108U)
#define CHAR12_Y          272U

/* Sample / label strings (GBK, exact bytes from the reference example). */
#define S_LINE0 "\xD5\xFD\xB5\xE3\xD4\xAD\xD7\xD3STM32\xBF\xAA\xB7\xA2\xB0\xE5"
#define S_LINE1 "GBK\xD7\xD6\xBF\xE2\xB2\xE2\xCA\xD4\xB3\xCC\xD0\xF2"
#define S_LINE2 "\xD5\xFD\xB5\xE3\xD4\xAD\xD7\xD3@ALIENTEK"
#define S_LINE3 "\xB0\xB4KEY0,\xB8\xFC\xD0\xC2\xD7\xD6\xBF\xE2"
#define S_HI    "\xC4\xDA\xC2\xEB\xB8\xDF\xD7\xD6\xBD\xDA:"
#define S_LO    "\xC4\xDA\xC2\xEB\xB5\xCD\xD7\xD6\xBD\xDA:"
#define S_CNT   "\xBA\xBA\xD7\xD6\xBC\xC6\xCA\xFD\xC6\xF7:"
#define S_CH32  "\xB6\xD4\xD3\xA6\xBA\xBA\xD7\xD6\xCE\xAA:"
#define S_CH16  "\xB6\xD4\xD3\xA6\xBA\xBA\xD7\xD6(16*16)\xCE\xAA:"
#define S_CH12  "\xB6\xD4\xD3\xA6\xBA\xBA\xD7\xD6(12*12)\xCE\xAA:"

static void app_show_samples(void)
{
    lcd_clear(WHITE);

    text_show_string(TEXT_X, 30U, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, (char *)S_LINE0, FONT_SIZE_PIXEL, 0, RED);
    text_show_string(TEXT_X, 50U, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, (char *)S_LINE1, FONT_SIZE_PIXEL, 0, RED);
    text_show_string(TEXT_X, 70U, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, (char *)S_LINE2, FONT_SIZE_PIXEL, 0, RED);
    text_show_string(TEXT_X, 90U, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, (char *)S_LINE3, FONT_SIZE_PIXEL, 0, RED);

    text_show_string(TEXT_X, HI_Y, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, (char *)S_HI, FONT_SIZE_PIXEL, 0, BLUE);
    text_show_string(TEXT_X, LO_Y, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, (char *)S_LO, FONT_SIZE_PIXEL, 0, BLUE);
    text_show_string(TEXT_X, CNT_Y, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, (char *)S_CNT, FONT_SIZE_PIXEL, 0, BLUE);

    text_show_string(TEXT_X, CHAR32_Y, TEXT_WIDTH_PX, 32U, (char *)S_CH32, 32U, 0, BLUE);
    text_show_string(TEXT_X, CHAR24_Y, TEXT_WIDTH_PX, 24U, (char *)S_CH32, 24U, 0, BLUE);
    text_show_string(TEXT_X, CHAR16_Y, TEXT_WIDTH_PX, 16U, (char *)S_CH16, 16U, 0, BLUE);
    text_show_string(TEXT_X, CHAR12_Y, TEXT_WIDTH_PX, 12U, (char *)S_CH12, 12U, 0, BLUE);
}

/** @brief  Rebuild the GBK font store in the NOR flash from the SD card, with
 *  on-screen progress. Returns 0 when the store is valid afterwards. */
static uint8_t app_font_rebuild(void)
{
    FRESULT res;
    uint8_t res2;

    printf("font update: start\r\n");
    lcd_clear(WHITE);
    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, LCD_FONT_SIZE_16, "STM32", RED);

    if (sdio_init() != 0U)
    {
        printf("font update: SD init failed\r\n");
        lcd_show_string(TEXT_X, 50U, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, LCD_FONT_SIZE_16, "SD Card Failed!", RED);
        delay_ms(200U);
        return 1U;
    }

    (void)exfuns_init();
    res = f_mount(fs[0], DRIVE, 1);

    if (res != FR_OK)
    {
        printf("font update: mount failed (%d)\r\n", (int)res);
        lcd_show_string(TEXT_X, 50U, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, LCD_FONT_SIZE_16, "SD Mount Failed!", RED);
        delay_ms(200U);
        return 1U;
    }

    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, LCD_FONT_SIZE_16, "SD Card OK", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, LCD_FONT_SIZE_16, "Font Updating...", RED);

    res2 = fonts_update_font(20U, 90U, FONT_SIZE_PIXEL, (uint8_t *)DRIVE, RED);

    if (res2 != 0U)
    {
        printf("font update: failed (%u)\r\n", (unsigned int)res2);
        lcd_show_string(TEXT_X, 90U, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, LCD_FONT_SIZE_16, "Font Update Failed!", RED);
        delay_ms(1000U);
        return 1U;
    }

    printf("font update: success\r\n");
    lcd_show_string(TEXT_X, 90U, TEXT_WIDTH_PX, FONT_SIZE_PIXEL, LCD_FONT_SIZE_16, "Font Update Success!", RED);
    delay_ms(1000U);

    return (fonts_init() == 0U) ? 0U : 1U;
}

/** @brief  Sweep the whole GBK range, drawing every code at four sizes with
 *  the lead/trail bytes and a running count. Returns when KEY0 is pressed. */
static void app_sweep(void)
{
    uint8_t  fontx[2];
    uint8_t  i;
    uint8_t  j;
    uint8_t  t;
    uint32_t fontcnt = 0U;

    for (i = GBK_LO_FIRST; i < GBK_LO_END; i++)
    {
        lcd_show_num(VAL_X, HI_Y, i, 3U, LCD_FONT_SIZE_16, BLUE);

        for (j = GBK_HI_FIRST; j < GBK_HI_END; j++)
        {
            if (j == GBK_HI_SKIP)
            {
                continue;
            }

            fontcnt++;
            fontx[0] = i;
            fontx[1] = j;

            lcd_show_num(VAL_X, LO_Y, j, 3U, LCD_FONT_SIZE_16, BLUE);
            lcd_show_num(VAL_X, CNT_Y, fontcnt, 5U, LCD_FONT_SIZE_16, BLUE);

            text_show_font(CHAR32_X, CHAR32_Y, fontx, 32U, 0U, BLUE);
            text_show_font(CHAR24_X, CHAR24_Y, fontx, 24U, 0U, BLUE);
            text_show_font(CHAR16_X, CHAR16_Y, fontx, 16U, 0U, BLUE);
            text_show_font(CHAR12_X, CHAR12_Y, fontx, 12U, 0U, BLUE);

            t = 200U;
            while (t-- != 0U)
            {
                delay_ms(1U);

                if (key_scan(false) == KEY0)
                {
                    return;   /* KEY0: refresh the font store */
                }
            }

            led_toggle(LED0);
        }
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");
    sdram_init();
    lcd_init();
    lcd_clear(WHITE);

    printf("43_font ready\r\n");

    for (;;)
    {
        /* Chinese needs the GBK store; build it from the card when missing. */
        while (fonts_init() != 0U)
        {
            (void)app_font_rebuild();
        }

        app_show_samples();
        app_sweep();

        /* KEY0 pressed in the sweep: refresh the font store from the SD card. */
        (void)app_font_rebuild();
    }
}
