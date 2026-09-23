/**
 * @file    main.c
 * @brief   46_music: WAV / MP3 player. Mounts the SD card, then scans
 *          0:/MUSIC and plays every WAV and MP3 track through the ES8388 + SAI
 *          audio path. KEY0 = next, KEY2 = previous, WK_UP = pause / resume.
 */

#include <stdio.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "malloc.h"
#include "audioplay.h"

#define DRIVE       "0:"
#define LOOP_DELAY_MS 500U

int main(void)
{
    FRESULT res;

    bsp_init();
    sdram_init();

    my_mem_init(SRAMIN);
    my_mem_init(SRAMEX);
    my_mem_init(SRAMCCM);

    printf("46_music ready\r\n");

    if (sdio_init() != 0U)
    {
        printf("SD init failed\r\n");
    }
    else
    {
        (void)exfuns_init();
        res = f_mount(fs[0], DRIVE, 1);

        if (res != FR_OK)
        {
            printf("mount failed (%d)\r\n", (int)res);
        }
        else
        {
            printf("SD mounted\r\n");
            audioplay_play();
        }
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(LOOP_DELAY_MS);
    }
}
