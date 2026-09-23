/**
 * @file    main.c
 * @brief   47_recorder: WAV recorder. Captures the ES8388 ADC through SAI into
 *          0:/RECORDER/RECxxxxx.wav. KEY0 = record / pause, KEY2 = stop and
 *          save, WK_UP = play the last recording. The elapsed time and bit rate
 *          are reported on USART1.
 */

#include <stdio.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "malloc.h"
#include "wavplay.h"
#include "recorder.h"

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

    printf("47_recorder ready\r\n");

    audio_hw_init();

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
            wav_recorder();
        }
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(LOOP_DELAY_MS);
    }
}
