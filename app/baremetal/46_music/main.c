/**
 * @file    main.c
 * @brief   46_music: WAV / MP3 player. Mounts the SD card, loads the GBK font
 *          store, then scans 0:/MUSIC and plays every WAV and MP3 track through
 *          the ES8388 + SAI audio path. KEY0 = next, KEY2 = previous,
 *          WK_UP = pause / resume.
 */

#include <stdio.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "text.h"
#include "malloc.h"
#include "audio.h"

#define DRIVE       "0:"
#define TEXT_X      30U
#define TEXT_WIDTH  320U
#define FONT_SIZE   16U

static void app_font_prepare(void)
{
    if (fonts_init() == 0U)
    {
        printf("font store ready\r\n");
        return;
    }

    printf("font store missing, updating from SD\r\n");
    lcd_show_string(TEXT_X, 60U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16,
                    "Font Updating...", RED);

    if (fonts_update_font(TEXT_X, 90U, FONT_SIZE, (uint8_t *)DRIVE, RED) == 0U)
    {
        (void)fonts_init();
        printf("font update done\r\n");
    }
    else
    {
        lcd_show_string(TEXT_X, 90U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16,
                        "Font Update Failed!", RED);
    }
}

int main(void)
{
    FRESULT res;

    bsp_init();
    sdram_init();
    lcd_init();
    lcd_clear(BLACK);

    my_mem_init(SRAMIN);
    my_mem_init(SRAMEX);
    my_mem_init(SRAMCCM);

    printf("46_music ready\r\n");

    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, "STM32 MUSIC", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, "KEY0:NEXT KEY2:PREV", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, "WK_UP:PAUSE/PLAY", RED);

    if (sdio_init() != 0U)
    {
        lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, "SD Card Error!", RED);
        printf("SD init failed\r\n");
    }
    else
    {
        (void)exfuns_init();
        res = f_mount(fs[0], DRIVE, 1);

        if (res != FR_OK)
        {
            printf("mount failed (%d)\r\n", (int)res);
            lcd_show_string(TEXT_X, 110U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, "Mount failed", RED);
        }
        else
        {
            printf("SD mounted\r\n");
            app_font_prepare();
            audio_play();
        }
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(500);
    }
}
