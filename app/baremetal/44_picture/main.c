/**
 * @file    main.c
 * @brief   44_picture: image viewer. Mounts the SD card, scans 0:/PICTURE for
 *          BMP/JPEG/GIF files and renders them on the RGB panel through the
 *          PICTURE middleware. KEY0 = next, KEY1 = previous, KEY2 = reload the
 *          directory, WK_UP = pause / resume the slideshow.
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "text.h"
#include "piclib.h"

#define PIC_DIR         "0:/PICTURE"
#define MAX_PICS        64U
#define NAME_LEN        64U
#define SLIDESHOW_MS    2000U

static char     g_names[MAX_PICS][NAME_LEN];
static uint16_t g_count;
static char     g_path[NAME_LEN + 16];

/** @brief  Collect the picture file names from the SD card. */
static void pic_scan(void)
{
    DIR     dir;
    FILINFO fno;
    FRESULT res;

    g_count = 0U;

    res = f_opendir(&dir, PIC_DIR);

    if (res != FR_OK)
    {
        printf("opendir %s failed (%d)\r\n", PIC_DIR, (int)res);
        return;
    }

    for (;;)
    {
        res = f_readdir(&dir, &fno);

        if ((res != FR_OK) || (fno.fname[0] == 0))
        {
            break;
        }

        if ((exfuns_file_type(fno.fname) & 0xF0U) == 0x50U)
        {
            if (g_count < MAX_PICS)
            {
                (void)strncpy(g_names[g_count], fno.fname, NAME_LEN - 1U);
                g_names[g_count][NAME_LEN - 1U] = '\0';
                g_count++;
            }
        }
    }

    (void)f_closedir(&dir);

    printf("found %u picture(s)\r\n", (unsigned int)g_count);
}

static void pic_show(uint16_t index)
{
    uint8_t res;

    if (g_count == 0U)
    {
        lcd_clear(BLACK);
        lcd_show_string(20U, 20U, 400U, 16U, LCD_FONT_SIZE_16, "No picture files", RED);
        return;
    }

    (void)sprintf(g_path, PIC_DIR "/%s", g_names[index]);

    lcd_clear(BLACK);
    res = piclib_ai_load_picfile(g_path, 0U, 0U, lcd_get_width(), lcd_get_height(), 1U);

    if (res != 0U)
    {
        printf("decode %s failed (%u)\r\n", g_path, (unsigned int)res);
        lcd_show_string(20U, 20U, 400U, 16U, LCD_FONT_SIZE_16, "Decode failed", RED);
    }

    text_show_string(2U, 2U, lcd_get_width(), 16U, g_path, 16U, 1U, RED);
    printf("show %s (%u/%u)\r\n", g_path, (unsigned int)(index + 1U), (unsigned int)g_count);
}

int main(void)
{
    FRESULT  res;
    uint16_t index = 0U;
    uint8_t  paused = 0U;
    uint8_t  reload = 1U;

    bsp_init();
    sdram_init();
    lcd_init();
    lcd_clear(BLACK);
    piclib_init();

    lcd_show_string(20U, 20U, 400U, 16U, LCD_FONT_SIZE_16, "STM32 PICTURE", RED);
    printf("44_picture ready\r\n");

    if (sdio_init() != 0U)
    {
        lcd_show_string(20U, 40U, 400U, 16U, LCD_FONT_SIZE_16, "SD Card Error!", RED);
        printf("SD init failed\r\n");
    }
    else
    {
        (void)exfuns_init();
        res = f_mount(fs[0], "0:", 1);

        if (res != FR_OK)
        {
            printf("mount failed (%d)\r\n", (int)res);
            lcd_show_string(20U, 40U, 400U, 16U, LCD_FONT_SIZE_16, "Mount failed", RED);
        }
        else
        {
            printf("SD mounted\r\n");
        }
    }

    for (;;)
    {
        key_id_t key = key_scan(false);
        uint8_t  keyed = 1U;

        if (reload != 0U)
        {
            reload = 0U;
            pic_scan();
            index = 0U;
            pic_show(index);
        }

        if (key == KEY0)
        {
            if (g_count != 0U)
            {
                index = (uint16_t)((index + 1U) % g_count);
            }
            pic_show(index);
        }
        else if (key == KEY1)
        {
            if (g_count != 0U)
            {
                index = (index == 0U) ? (uint16_t)(g_count - 1U) : (uint16_t)(index - 1U);
            }
            pic_show(index);
        }
        else if (key == KEY2)
        {
            reload = 1U;
        }
        else if (key == KEY_WKUP)
        {
            paused = (paused == 0U) ? 1U : 0U;
            printf("slideshow %s\r\n", (paused != 0U) ? "paused" : "running");
        }
        else
        {
            keyed = 0U;
        }

        if (keyed != 0U)
        {
            led_toggle(LED0);
            delay_ms(100U);
        }
        else if (paused == 0U)
        {
            delay_ms(SLIDESHOW_MS);

            if (g_count != 0U)
            {
                index = (uint16_t)((index + 1U) % g_count);
            }
            pic_show(index);
        }
        else
        {
            led_toggle(LED0);
            delay_ms(100U);
        }
    }
}
