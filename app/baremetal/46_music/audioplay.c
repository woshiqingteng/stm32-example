/**
 * @file    audioplay.c
 * @brief   46_music player: scans 0:/MUSIC and plays WAV tracks with KEY
 *          navigation, using the WAVPLAY middleware for decoding/playback.
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "text.h"
#include "malloc.h"
#include "wavplay.h"
#include "audioplay.h"

#define AUDIO_MAX_FILES 64U
#define AUDIO_NAME_LEN  64U

static char     g_audio_names[AUDIO_MAX_FILES][AUDIO_NAME_LEN];
static uint16_t g_audio_count;

uint16_t audioplay_get_tnum(const char *path)
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

        if (exfuns_file_type(tfileinfo.fname) == T_WAV)
        {
            rval++;
        }
    }

    (void)f_closedir(&tdir);

    return rval;
}

static void audioplay_scan(void)
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

        if ((exfuns_file_type(fno.fname) == T_WAV) && (g_audio_count < AUDIO_MAX_FILES))
        {
            (void)strncpy(g_audio_names[g_audio_count], fno.fname, AUDIO_NAME_LEN - 1U);
            g_audio_names[g_audio_count][AUDIO_NAME_LEN - 1U] = '\0';
            g_audio_count++;
        }
    }

    (void)f_closedir(&dir);

    printf("found %u track(s)\r\n", (unsigned int)g_audio_count);
}

void audioplay_index_show(uint16_t index, uint16_t total)
{
    lcd_show_num(30, 230, index, 3, LCD_FONT_SIZE_16, RED);
    lcd_show_char(30 + 24, 230, '/', LCD_FONT_SIZE_16, LCD_TEXT_BG_OVERWRITE, RED);
    lcd_show_num(30 + 32, 230, total, 3, LCD_FONT_SIZE_16, RED);
}

static audio_nav_t audioplay_play_song(char *fname)
{
    if (exfuns_file_type(fname) == T_WAV)
    {
        return wav_play_song(fname);
    }

    printf("can't play:%s\r\n", fname);
    return AUDIO_NEXT;      /* skip to the next track */
}

void audioplay_play(void)
{
    uint16_t index = 0;
    char     path[AUDIO_NAME_LEN + 16];
    audio_nav_t key;

    audio_hw_init();

    for (;;)
    {
        audioplay_scan();

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
        audioplay_index_show((uint16_t)(index + 1U), g_audio_count);

        key = audioplay_play_song(path);

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
