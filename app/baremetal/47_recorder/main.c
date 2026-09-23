/**
 * @file    main.c
 * @brief   47_recorder: WAV recorder. Captures the ES8388 ADC through SAI into
 *          0:/RECORDER/RECxxxxx.wav. KEY0 = record / pause, KEY2 = stop and
 *          save, WK_UP = play the last recording. The elapsed time and bit rate
 *          are shown on the RGB panel.
 */

#include <stdio.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "text.h"
#include "malloc.h"
#include "wavplay.h"
#include "recorder.h"

#define DRIVE       "0:"
#define TEXT_X      30U
#define TEXT_WIDTH  320U
#define FONT_SIZE   16U
#define LOOP_DELAY_MS 500U

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
    lcd_clear(WHITE);

    my_mem_init(SRAMIN);
    my_mem_init(SRAMEX);
    my_mem_init(SRAMCCM);

    printf("47_recorder ready\r\n");

    lcd_show_string(TEXT_X, 30U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, "STM32 RECORDER", RED);
    lcd_show_string(TEXT_X, 50U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, "KEY0:REC/PAUSE", RED);
    lcd_show_string(TEXT_X, 70U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, "KEY2:STOP&SAVE", RED);
    lcd_show_string(TEXT_X, 90U, TEXT_WIDTH, FONT_SIZE, LCD_FONT_SIZE_16, "WK_UP:PLAY", RED);

    audio_hw_init();

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
            wav_recorder();
        }
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(LOOP_DELAY_MS);
    }
}
