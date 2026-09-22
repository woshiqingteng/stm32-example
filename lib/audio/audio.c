/**
 * @file    audio.c
 * @brief   ALIENTEK AUDIOCODEC middleware: playback device, SAI half-transfer
 *          handler and the 0:/MUSIC playlist state machine.
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "text.h"
#include "malloc.h"
#include "audio.h"
#include "wavplay.h"
#include "mp3play.h"

#define AUDIO_MUSIC_DIR     "0:/MUSIC"
#define AUDIO_MAX_FILES     64U
#define AUDIO_NAME_LEN      64U

audiodev_t g_audiodev;

volatile uint8_t audio_transfer_end = 0;
volatile uint8_t audio_witch_buf = 0;

static char     g_audio_names[AUDIO_MAX_FILES][AUDIO_NAME_LEN];
static uint16_t g_audio_count;

void audio_hw_init(void)
{
    es8388_init();
    es8388_adda_cfg(1, 0);      /* enable DAC, disable ADC */
    es8388_output_cfg(1, 1);    /* enable output channels 1 and 2 */
    es8388_hpvol_set(25);
    es8388_spkvol_set(25);
}

void audio_start(void)
{
    g_audiodev.status = 3 << 0; /* running + playing */
    sai1_play_start();
}

void audio_stop(void)
{
    g_audiodev.status = 0;
    sai1_play_stop();
}

void audio_sai_tx_callback(void)
{
    uint16_t i;

    if (sai1_tx_dma_target() != 0U)
    {
        audio_witch_buf = 0;

        if ((g_audiodev.status & 0x01) == 0)     /* paused: silence buf1 */
        {
            for (i = 0; i < AUDIO_SAI_TX_BUF_SIZE; i++)
            {
                g_audiodev.saibuf1[i] = 0;
            }
        }
    }
    else
    {
        audio_witch_buf = 1;

        if ((g_audiodev.status & 0x01) == 0)     /* paused: silence buf2 */
        {
            for (i = 0; i < AUDIO_SAI_TX_BUF_SIZE; i++)
            {
                g_audiodev.saibuf2[i] = 0;
            }
        }
    }

    audio_transfer_end = 1;
}

uint16_t audio_get_tnum(const char *path)
{
    DIR     tdir;
    FILINFO tfileinfo;
    uint16_t rval = 0;

    if (f_opendir(&tdir, (const TCHAR *)path) != FR_OK)
    {
        return 0;
    }

    for (;;)
    {
        if ((f_readdir(&tdir, &tfileinfo) != FR_OK) || (tfileinfo.fname[0] == 0))
        {
            break;
        }

        if ((exfuns_file_type(tfileinfo.fname) & 0xF0U) == 0x40U)
        {
            rval++;
        }
    }

    (void)f_closedir(&tdir);

    return rval;
}

static void audio_scan(void)
{
    DIR     dir;
    FILINFO fno;

    g_audio_count = 0;

    if (f_opendir(&dir, AUDIO_MUSIC_DIR) != FR_OK)
    {
        printf("opendir %s failed\r\n", AUDIO_MUSIC_DIR);
        return;
    }

    for (;;)
    {
        if ((f_readdir(&dir, &fno) != FR_OK) || (fno.fname[0] == 0))
        {
            break;
        }

        if (((exfuns_file_type(fno.fname) & 0xF0U) == 0x40U) && (g_audio_count < AUDIO_MAX_FILES))
        {
            (void)strncpy(g_audio_names[g_audio_count], fno.fname, AUDIO_NAME_LEN - 1U);
            g_audio_names[g_audio_count][AUDIO_NAME_LEN - 1U] = '\0';
            g_audio_count++;
        }
    }

    (void)f_closedir(&dir);

    printf("found %u track(s)\r\n", (unsigned int)g_audio_count);
}

void audio_index_show(uint16_t index, uint16_t total)
{
    lcd_show_num(30, 230, index, 3, LCD_FONT_SIZE_16, RED);
    lcd_show_char(30 + 24, 230, '/', LCD_FONT_SIZE_16, LCD_TEXT_BG_OVERWRITE, RED);
    lcd_show_num(30 + 32, 230, total, 3, LCD_FONT_SIZE_16, RED);
}

void audio_msg_show(uint32_t totsec, uint32_t cursec, uint32_t bitrate)
{
    static uint16_t playtime = 0xFFFF;

    if (playtime != cursec)
    {
        playtime = (uint16_t)cursec;

        lcd_show_num(30, 210, playtime / 60, 2, LCD_FONT_SIZE_16, RED);
        lcd_show_char(30 + 16, 210, ':', LCD_FONT_SIZE_16, LCD_TEXT_BG_OVERWRITE, RED);
        lcd_show_num(30 + 24, 210, playtime % 60, 2, LCD_FONT_SIZE_16, RED);
        lcd_show_char(30 + 40, 210, '/', LCD_FONT_SIZE_16, LCD_TEXT_BG_OVERWRITE, RED);

        lcd_show_num(30 + 48, 210, totsec / 60, 2, LCD_FONT_SIZE_16, RED);
        lcd_show_char(30 + 64, 210, ':', LCD_FONT_SIZE_16, LCD_TEXT_BG_OVERWRITE, RED);
        lcd_show_num(30 + 72, 210, totsec % 60, 2, LCD_FONT_SIZE_16, RED);

        lcd_show_num(30 + 110, 210, bitrate / 1000, 4, LCD_FONT_SIZE_16, RED);
        lcd_show_string(30 + 110 + 32, 210, 200, 16, LCD_FONT_SIZE_16, "Kbps", RED);
    }
}

uint8_t audio_play_song(char *fname)
{
    uint8_t res = exfuns_file_type(fname);

    switch (res)
    {
        case T_WAV:
            res = wav_play_song(fname);
            break;

        case T_MP3:
            res = mp3_play_song(fname);
            break;

        default:
            printf("can't play:%s\r\n", fname);
            res = AUDIO_NEXT;     /* skip to the next track */
            break;
    }

    return res;
}

void audio_play(void)
{
    uint16_t index = 0;
    char     path[AUDIO_NAME_LEN + 16];
    uint8_t  key;

    audio_hw_init();

    for (;;)
    {
        audio_scan();

        if (g_audio_count == 0U)
        {
            text_show_string(30, 190, 240, 16, "No music files!", 16, 0, BLUE);
            delay_ms(500);
            continue;
        }

        break;
    }

    for (;;)
    {
        (void)sprintf(path, "%s/%s", AUDIO_MUSIC_DIR, g_audio_names[index]);

        lcd_fill(30, 190, lcd_get_width() - 1, 190 + 16, WHITE);
        text_show_string(30, 190, lcd_get_width() - 60, 16, g_audio_names[index], 16, 0, BLUE);
        audio_index_show((uint16_t)(index + 1U), g_audio_count);

        key = audio_play_song(path);

        if (key == AUDIO_PREV)
        {
            index = (index == 0U) ? (uint16_t)(g_audio_count - 1U) : (uint16_t)(index - 1U);
        }
        else if (key == AUDIO_NEXT)
        {
            index = (uint16_t)((index + 1U) % g_audio_count);
        }
        else
        {
            break;
        }
    }
}
