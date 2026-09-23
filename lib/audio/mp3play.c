/**
 * @file    mp3play.c
 * @brief   MP3 playback using the Helix fixed-point decoder.
 *
 * The decoder runs in the main loop, not in the DMA interrupt: decoded PCM is
 * cached and copied into the SAI half-buffers, exactly like the WAV path. The
 * shared audio half-transfer callback only flags the buffer boundary.
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "ff.h"
#include "malloc.h"
#include "mp3dec.h"
#include "audio.h"
#include "mp3play.h"

#define MP3_IN_BUF_SIZE     4096U
#define MP3_MAX_OUT_SAMPS   (MAX_NGRAN * MAX_NSAMP * MAX_NCHAN)   /* 2304 shorts */

static FIL               s_file;
static HMP3Decoder       s_dec;
static MP3FrameInfo      s_info;

static unsigned char    *s_in;          /* in SDRAM: keeps the internal SRAM free */
static int               s_in_len;
static int               s_in_pos;
static uint8_t           s_eof;

static short            *s_pcm;         /* in SDRAM */
static int               s_pcm_len;     /* bytes valid in s_pcm  */
static int               s_pcm_pos;     /* bytes already handed out */

static uint32_t          s_bitrate;     /* bits / second */

/** @brief  Compact the input buffer and pull more bytes from the file. */
static void mp3_read_input(void)
{
    UINT br = 0;

    if (s_in_pos > 0)
    {
        memmove(s_in, s_in + s_in_pos, (size_t)(s_in_len - s_in_pos));
        s_in_len -= s_in_pos;
        s_in_pos = 0;
    }

    if ((s_eof == 0U) && ((uint32_t)s_in_len < MP3_IN_BUF_SIZE))
    {
        if (f_read(&s_file, s_in + s_in_len, MP3_IN_BUF_SIZE - (UINT)s_in_len, &br) != FR_OK)
        {
            s_eof = 1U;
        }
        else if (br == 0U)
        {
            s_eof = 1U;
        }
        else
        {
            s_in_len += (int)br;
        }
    }
}

/**
 * @brief  Decode frames until the PCM cache holds data. Sets s_pcm_len = 0
 *         once the input is exhausted.
 */
static void mp3_decode_frame(void)
{
    s_pcm_len = 0;
    s_pcm_pos = 0;

    for (;;)
    {
        const unsigned char *inbuf;
        size_t bytes_left;
        int    offset;
        int    err;
        int    remaining;

        if (s_in_pos >= s_in_len)
        {
            if (s_eof != 0U)
            {
                return;     /* nothing left */
            }
            mp3_read_input();
            continue;
        }

        offset = MP3FindSyncWord(s_in + s_in_pos, s_in_len - s_in_pos);

        if (offset < 0)
        {
            s_in_pos = s_in_len;    /* no sync in the current data */
            continue;
        }

        s_in_pos += offset;
        remaining = s_in_len - s_in_pos;
        inbuf = s_in + s_in_pos;
        bytes_left = (size_t)remaining;

        err = MP3Decode(s_dec, &inbuf, &bytes_left, s_pcm, 0);
        s_in_pos += remaining - (int)bytes_left;

        if (err == 0)
        {
            MP3GetLastFrameInfo(s_dec, &s_info);
            s_bitrate = (uint32_t)s_info.bitrate;
            s_pcm_len = s_info.outputSamps * (int)sizeof(short);
            s_pcm_pos = 0;
            return;
        }
        else if (err == ERR_MP3_INDATA_UNDERFLOW)
        {
            if (s_eof != 0U)
            {
                return;
            }
            mp3_read_input();
        }
        else
        {
            if (s_in_pos < s_in_len)
            {
                s_in_pos++;     /* bad byte, resync */
            }
        }
    }
}

/** @brief  Fill dst with len bytes of PCM (silence past the end of stream).
 *  @return 1 when the end of stream was reached, 0 otherwise. */
static uint8_t mp3_fill(uint8_t *dst, uint16_t len)
{
    uint16_t done = 0;
    uint8_t  eos = 0U;

    while (done < len)
    {
        int n;

        if (s_pcm_pos >= s_pcm_len)
        {
            mp3_decode_frame();

            if (s_pcm_len == 0)
            {
                memset(dst + done, 0, (size_t)(len - done));
                eos = 1U;
                break;
            }
        }

        n = s_pcm_len - s_pcm_pos;

        if (n > (int)(len - done))
        {
            n = (int)(len - done);
        }

        memcpy(dst + done, (uint8_t *)s_pcm + s_pcm_pos, (size_t)n);
        s_pcm_pos += n;
        done = (uint16_t)(done + n);
    }

    return eos;
}

audio_nav_t mp3_play_song(char *fname)
{
    uint8_t  key;
    uint8_t  t = 0;
    audio_nav_t res = AUDIO_STOP;
    uint32_t totsec;
    uint32_t cursec;

    g_audiodev.saibuf1 = mymalloc(SRAMIN, AUDIO_SAI_TX_BUF_SIZE);
    g_audiodev.saibuf2 = mymalloc(SRAMIN, AUDIO_SAI_TX_BUF_SIZE);
    s_in = mymalloc(SRAMEX, MP3_IN_BUF_SIZE);
    s_pcm = mymalloc(SRAMEX, MP3_MAX_OUT_SAMPS * sizeof(short));

    if ((g_audiodev.saibuf1 == NULL) || (g_audiodev.saibuf2 == NULL) ||
        (s_in == NULL) || (s_pcm == NULL))
    {
        res = AUDIO_ERROR;
    }
    else if (f_open(&s_file, (TCHAR *)fname, FA_READ) != FR_OK)
    {
        res = AUDIO_ERROR;
    }
    else
    {
        s_dec = MP3InitDecoder();

        if (s_dec == NULL)
        {
            res = AUDIO_ERROR;
        }
        else
        {
            s_in_len = 0;
            s_in_pos = 0;
            s_eof = 0U;
            s_bitrate = 128000U;

            mp3_decode_frame();     /* first frame provides the stream format */

            if (s_pcm_len == 0)
            {
                res = AUDIO_ERROR;
            }
            else
            {
                es8388_sai_cfg(0, 3);           /* standard I2S, 16-bit */
                sai1_saia_init(SAI_MODEMASTER_TX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_16);
                (void)sai1_samplerate_set((uint32_t)s_info.samprate);
                sai1_tx_dma_init(g_audiodev.saibuf1, g_audiodev.saibuf2, AUDIO_SAI_TX_BUF_SIZE / 2, 1);
                sai_tx_callback = audio_sai_tx_callback;
                audio_stop();

                (void)mp3_fill(g_audiodev.saibuf1, AUDIO_SAI_TX_BUF_SIZE);
                (void)mp3_fill(g_audiodev.saibuf2, AUDIO_SAI_TX_BUF_SIZE);
                audio_start();

                for (;;)
                {
                    uint8_t eos;

                    while (!audio_transfer_end)
                    {
                        /* wait for a half-buffer to finish */
                    }
                    audio_transfer_end = false;

                    if (audio_witch_buf)
                    {
                        eos = mp3_fill(g_audiodev.saibuf2, AUDIO_SAI_TX_BUF_SIZE);
                    }
                    else
                    {
                        eos = mp3_fill(g_audiodev.saibuf1, AUDIO_SAI_TX_BUF_SIZE);
                    }

                    if (eos != 0U)
                    {
                        res = AUDIO_STOP;
                        break;
                    }

                    key = (uint8_t)key_scan(false);

                    if (key == KEY_WKUP)                /* pause / resume */
                    {
                        if (g_audiodev.status & 0x01)
                        {
                            g_audiodev.status &= (uint8_t)~(1 << 0);
                        }
                        else
                        {
                            g_audiodev.status |= 0x01;
                        }
                    }
                    else if (key == KEY2)
                    {
                        res = AUDIO_PREV;
                        break;
                    }
                    else if (key == KEY0)
                    {
                        res = AUDIO_NEXT;
                        break;
                    }
                    else
                    {
                        /* no key */
                    }

                    totsec = (uint32_t)(((uint64_t)f_size(&s_file) * 8U) / s_bitrate);
                    cursec = (uint32_t)(((uint64_t)s_file.fptr * 8U) / s_bitrate);
                    audio_msg_show(totsec, cursec, s_bitrate);

                    t++;
                    if (t == 20)
                    {
                        t = 0;
                        led_toggle(LED0);
                    }

                    if ((g_audiodev.status & 0x01) == 0)
                    {
                        delay_ms(10);
                    }
                }

                audio_stop();
            }

            MP3FreeDecoder(s_dec);
            s_dec = NULL;
        }

        (void)f_close(&s_file);
    }

    myfree(SRAMIN, g_audiodev.saibuf1);
    myfree(SRAMIN, g_audiodev.saibuf2);
    myfree(SRAMEX, s_in);
    myfree(SRAMEX, s_pcm);

    return res;
}
