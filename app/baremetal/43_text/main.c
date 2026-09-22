/**
 * @file    main.c
 * @brief   43_text: GBK text display. Mounts the SD card through FatFs, makes
 *          sure the GBK font store exists in the NOR flash (updating it from
 *          the card when necessary) and then renders Chinese and ASCII text on
 *          the RGB panel with the TEXT middleware. KEY0/KEY1 cycle the samples.
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "text.h"

#define DRIVE           "0:"
#define TEXT_X          30U
#define TEXT_WIDTH      320U
#define FONT_SIZE       16U
#define LOOP_DELAY_MS   50U

/* Four sample pages; the Chinese entries are GBK byte sequences. */
static const char *const g_sample_title[] =
{
    "\xBA\xBA\xD7\xD6\xCF\xD4\xCA\xBE",                 /* han zi xian shi */
    "\xCD\xBC\xC6\xAC\xCF\xD4\xCA\xBE",                 /* tu pian xian shi */
    "\xC4\xE3\xBA\xC3\xA3\xACSTM32",                    /* ni hao, STM32    */
    "\xD5\xFD\xB5\xE3\xD4\xAD\xD7\xD3",                 /* zheng dian yuan zi */
};

static const char *const g_sample_line1[] =
{
    "\xC4\xE3\xBA\xC3\xA3\xA1",                         /* ni hao!          */
    "\xBF\xAA\xB7\xA2\xB0\xE5",                         /* kai fa ban       */
    "ATOM@ALIENTEK",
    "\xD7\xEE\xD0\xC2\xB9\xCC\xBC\xFE",                 /* newest firmware   */
};

static const char *const g_sample_line2[] =
{
    "GBK FONT OK",
    "RGB 800x480",
    "FATFS 0:",
    "TEXT MIDDLEWARE",
};

static BYTE  g_work[FF_MAX_SS];
static char  g_line[64];

/** @brief  Prepare the font store: reuse the NOR flash copy when valid, else
 *  copy it from the SD card. Returns 0 on success. */
static uint8_t app_font_prepare(void)
{
    uint8_t key;

    if (fonts_init() == 0U)
    {
        printf("font store ready\r\n");
        return 0U;
    }

    printf("font store missing, updating from SD\r\n");
    lcd_show_string(TEXT_X, 60U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16,
                    "Font Updating...", RED);

    key = fonts_update_font(TEXT_X, 90U, FONT_SIZE, (uint8_t *)DRIVE, RED);

    if (key != 0U)
    {
        printf("font update failed (%u)\r\n", (unsigned int)key);
        lcd_show_string(TEXT_X, 90U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16,
                        "Font Update Failed!", RED);
        return 1U;
    }

    (void)fonts_init();
    printf("font update done\r\n");
    return 0U;
}

static void app_show_page(uint8_t page)
{
    lcd_clear(WHITE);

    (void)sprintf(g_line, "43_text page %u", (unsigned int)page);
    lcd_show_string(TEXT_X, 10U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, g_line, BLUE);

    text_show_string(TEXT_X, 40U, TEXT_WIDTH, FONT_SIZE, (char *)g_sample_title[page], FONT_SIZE, 0, RED);
    text_show_string(TEXT_X, 70U, TEXT_WIDTH, FONT_SIZE, (char *)g_sample_line1[page], FONT_SIZE, 0, BLUE);
    text_show_string(TEXT_X, 100U, TEXT_WIDTH, FONT_SIZE, (char *)g_sample_line2[page], FONT_SIZE, 0, GREEN);

    text_show_string(TEXT_X, 150U, TEXT_WIDTH, 24U, (char *)"\xBA\xBA", 24U, 0, MAGENTA);
    text_show_string(TEXT_X, 200U, TEXT_WIDTH, 32U, (char *)"\xD7\xD6", 32U, 0, BLACK);

    printf("page %u: %s\r\n", (unsigned int)page, g_sample_line2[page]);
}

int main(void)
{
    FRESULT res;
    uint8_t page = 0U;

    bsp_init();
    sdram_init();
    lcd_init();
    lcd_clear(WHITE);

    lcd_show_string(TEXT_X, 10U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, "STM32", RED);

    printf("43_text ready\r\n");

    if (sdio_init() != 0U)
    {
        lcd_show_string(TEXT_X, 40U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, "SD Card Error!", RED);
        printf("SD init failed\r\n");
    }
    else
    {
        (void)exfuns_init();
        res = f_mount(fs[0], DRIVE, 1);

        if (res == FR_NO_FILESYSTEM)
        {
            printf("no filesystem, formatting %s\r\n", DRIVE);
            res = f_mkfs(DRIVE, 0, g_work, sizeof(g_work));

            if (res == FR_OK)
            {
                res = f_mount(fs[0], DRIVE, 1);
            }
        }

        if (res != FR_OK)
        {
            (void)sprintf(g_line, "mount failed (%d)", (int)res);
            lcd_show_string(TEXT_X, 40U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, g_line, RED);
            printf("%s\r\n", g_line);
        }
        else
        {
            printf("SD mounted\r\n");

            if (app_font_prepare() != 0U)
            {
                lcd_show_string(TEXT_X, 130U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16,
                                "Font Error!", RED);
            }

            app_show_page(page);
        }
    }

    for (;;)
    {
        key_id_t key = key_scan(false);

        if (key == KEY0)
        {
            page = (uint8_t)((page + 1U) % 4U);
            app_show_page(page);
        }
        else if (key == KEY1)
        {
            page = (uint8_t)((page == 0U) ? 3U : (page - 1U));
            app_show_page(page);
        }
        else if (key == KEY_WKUP)
        {
            printf("font store fontok=0x%02X\r\n", (unsigned int)ftinfo.fontok);
        }
        else
        {
            /* no key */
        }

        led_toggle(LED0);
        delay_ms(LOOP_DELAY_MS);
    }
}
