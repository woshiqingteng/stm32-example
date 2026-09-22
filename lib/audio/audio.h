/**
 * @file    audio.h
 * @brief   ALIENTEK AUDIOCODEC middleware: ES8388 + SAI playback and capture
 *          state machine, shared by the music player, the recorder and the
 *          video player. All rendering goes through the RGB panel (lcd.h).
 */

#ifndef LIB_AUDIO_AUDIO_H
#define LIB_AUDIO_AUDIO_H

#include <stdint.h>
#include "ff.h"

/** @brief  Playback DMA buffer size (bytes). 8192 avoids drop-outs at
 *  192 kbps / 24-bit. */
#define AUDIO_SAI_TX_BUF_SIZE    8192U

/* Player return codes (kept out of the FRESULT range, which has FR_OK == 0). */
#define AUDIO_STOP               0U   /* stop / finished */
#define AUDIO_NEXT               1U   /* KEY0: next track          */
#define AUDIO_PREV               2U   /* KEY2: previous track      */
#define AUDIO_ERROR              0xFFU

/** @brief  Shared audio device state. */
typedef struct
{
    uint8_t *saibuf1;       /* SAI TX half-buffer 1 */
    uint8_t *saibuf2;       /* SAI TX half-buffer 2 */
    uint8_t *tbuf;          /* scratch buffer (24-bit WAV repacking) */
    FIL     *file;          /* file being played */

    uint8_t  status;        /* bit0: 0 paused, 1 playing; bit1: 0 stopped, 1 running */
} audiodev_t;

extern audiodev_t g_audiodev;

/** @brief  SAI TX half-transfer flags shared with the format players. */
extern volatile uint8_t audio_transfer_end;   /* 1 when a half-buffer finished */
extern volatile uint8_t audio_witch_buf;      /* 0: buf1 served, 1: buf2 served */

/** @brief  Configure the codec for DAC playback and set a default volume. */
void audio_hw_init(void);

/** @brief  Start playback (status = play + running). */
void audio_start(void);

/** @brief  Stop playback (status = stopped). */
void audio_stop(void);

/** @brief  SAI TX half-transfer handler used by both WAV and MP3 playback. */
void audio_sai_tx_callback(void);

/** @brief  Count the playable WAV/MP3 files in a directory. */
uint16_t audio_get_tnum(const char *path);

/** @brief  Render the current track index (e.g. "3/12"). */
void audio_index_show(uint16_t index, uint16_t total);

/** @brief  Render elapsed / total time and the bit rate. */
void audio_msg_show(uint32_t totsec, uint32_t cursec, uint32_t bitrate);

/** @brief  Scan 0:/MUSIC and play all WAV/MP3 tracks with KEY navigation. */
void audio_play(void);

/** @brief  Play one file, dispatching on the extension (WAV or MP3). */
uint8_t audio_play_song(char *fname);

#endif /* LIB_AUDIO_AUDIO_H */
