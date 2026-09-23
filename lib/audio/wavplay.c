/**
 * @file    wavplay.c
 * @brief   WAV playback (16-bit / 24-bit PCM) over the ES8388 + SAI path.
 *
 * The file header is parsed first, then the SAI is configured for the source
 * bit depth and sample rate. Two half buffers feed the circular TX DMA; the
 * half-transfer callback is shared with the audio device (audio.c).
 */

#include <stdio.h>
#include <stdbool.h>
#include "bsp.h"
#include "ff.h"
#include "malloc.h"
#include "wavplay.h"

/* Self-contained playback device: ES8388 + SAI double-buffered TX path. */
typedef struct
{
    uint8_t *saibuf1;       /* SAI TX half-buffer 1 */
    uint8_t *saibuf2;       /* SAI TX half-buffer 2 */
    uint8_t *tbuf;          /* scratch buffer (24-bit WAV repacking) */
    void    *file;          /* file being played (FatFs FIL *) */
    uint8_t  status;        /* bit0: 0 paused, 1 playing; bit1: 0 stopped, 1 running */
} audio_dev_t;

static audio_dev_t   s_dev;
static volatile bool s_transfer_end;    /* true when a half-buffer finished */
static volatile bool s_witch_buf;       /* false: buf1 served, true: buf2 served */

__wavctrl wavctrl;      /* parsed WAV control block */

void audio_hw_init(void)
{
    es8388_init();
    es8388_adda_cfg(1, 0);      /* enable DAC, disable ADC */
    es8388_output_cfg(1, 1);    /* enable output channels 1 and 2 */
    es8388_hpvol_set(25);
    es8388_spkvol_set(25);
}

static void audio_start(void)
{
    s_dev.status = 3 << 0;      /* running + playing */
    sai1_play_start();
}

static void audio_stop(void)
{
    s_dev.status = 0;
    sai1_play_stop();
}

static void audio_sai_tx_callback(void)
{
    uint16_t i;

    if (sai1_tx_dma_target() != 0U)
    {
        s_witch_buf = false;

        if ((s_dev.status & 0x01) == 0)     /* paused: silence buf1 */
        {
            for (i = 0; i < AUDIO_SAI_TX_BUF_SIZE; i++)
            {
                s_dev.saibuf1[i] = 0;
            }
        }
    }
    else
    {
        s_witch_buf = true;

        if ((s_dev.status & 0x01) == 0)     /* paused: silence buf2 */
        {
            for (i = 0; i < AUDIO_SAI_TX_BUF_SIZE; i++)
            {
                s_dev.saibuf2[i] = 0;
            }
        }
    }

    s_transfer_end = true;
}

static void audio_msg_show(uint32_t totsec, uint32_t cursec, uint32_t bitrate)
{
    static uint16_t playtime = 0xFFFF;

    if (playtime != cursec)
    {
        playtime = (uint16_t)cursec;

        printf("time %02lu:%02lu/%02lu:%02lu %lu Kbps\r\n",
               (unsigned long)(cursec / 60U), (unsigned long)(cursec % 60U),
               (unsigned long)(totsec / 60U), (unsigned long)(totsec % 60U),
               (unsigned long)(bitrate / 1000U));
    }
}

wav_status_t wav_decode_init(char *fname, __wavctrl *wavx)
{
    FIL      *ftemp;
    uint8_t  *buf;
    uint32_t  br = 0;
    wav_status_t res = WAV_OK;

    ChunkRIFF *riff;
    ChunkFMT  *fmt;
    ChunkFACT *fact;
    ChunkDATA *data;

    ftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    buf = mymalloc(SRAMIN, 512);

    if ((ftemp != NULL) && (buf != NULL))
    {
        res = f_open(ftemp, (TCHAR *)fname, FA_READ);

        if (res == FR_OK)
        {
            (void)f_read(ftemp, buf, 512, (UINT *)&br);
            riff = (ChunkRIFF *)buf;

            if (riff->Format == 0x45564157)                 /* "WAVE" */
            {
                fmt = (ChunkFMT *)(buf + 12);
                fact = (ChunkFACT *)(buf + 12 + 8 + fmt->ChunkSize);

                if ((fact->ChunkID == 0x74636166) || (fact->ChunkID == 0x5453494C))
                {
                    wavx->datastart = 12 + 8 + fmt->ChunkSize + 8 + fact->ChunkSize;
                }
                else
                {
                    wavx->datastart = 12 + 8 + fmt->ChunkSize;
                }

                data = (ChunkDATA *)(buf + wavx->datastart);

                if (data->ChunkID == 0x61746164)            /* "data" */
                {
                    wavx->audioformat = fmt->AudioFormat;
                    wavx->nchannels = fmt->NumOfChannels;
                    wavx->samplerate = fmt->SampleRate;
                    wavx->bitrate = fmt->ByteRate * 8;
                    wavx->blockalign = fmt->BlockAlign;
                    wavx->bps = fmt->BitsPerSample;

                    wavx->datasize = data->ChunkSize;
                    wavx->datastart = wavx->datastart + 8;

                    printf("wav:%u ch, %u Hz, %u bps, %u bit\r\n",
                           (unsigned int)wavx->nchannels,
                           (unsigned int)wavx->samplerate,
                           (unsigned int)wavx->bitrate,
                           (unsigned int)wavx->bps);
                }
                else
                {
                    res = WAV_ERR_DATA;        /* data chunk not found */
                }
            }
            else
            {
                res = WAV_ERR_FORMAT;          /* not a WAV file */
            }
        }
        else
        {
            res = WAV_ERR_OPEN;                /* open failed */
        }
    }

    if (ftemp != NULL)
    {
        (void)f_close(ftemp);
        myfree(SRAMIN, ftemp);
    }
    myfree(SRAMIN, buf);

    return res;
}

uint32_t wav_buffill(uint8_t *buf, uint16_t size, uint8_t bits)
{
    uint16_t  readlen = 0;
    uint32_t  bread;
    uint16_t  i;

    if (bits == 24)                 /* 24-bit: repack 3 bytes into 4-byte slots */
    {
        uint32_t *pbuf;

        readlen = (uint16_t)((size / 4) * 3);
        (void)f_read(s_dev.file, s_dev.tbuf, readlen, (UINT *)&bread);
        pbuf = (uint32_t *)buf;

        for (i = 0; i < (size / 4); i++)
        {
            const uint8_t *b = s_dev.tbuf + (i * 3);

            pbuf[i] = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16);
        }

        bread = (bread * 4) / 3;
    }
    else
    {
        (void)f_read(s_dev.file, buf, size, (UINT *)&bread);

        if (bread < size)           /* pad the tail with silence */
        {
            for (i = (uint16_t)bread; i < size; i++)
            {
                buf[i] = 0;
            }
        }
    }

    return bread;
}

static void wav_get_curtime(FIL *fx, __wavctrl *wavx)
{
    long long fpos;

    wavx->totsec = wavx->datasize / (wavx->bitrate / 8);
    fpos = (long long)fx->fptr - (long long)wavx->datastart;
    wavx->cursec = (uint32_t)(fpos * (long long)wavx->totsec / (long long)wavx->datasize);
}

audio_nav_t wav_play_song(char *fname)
{
    FRESULT  fres;
    uint8_t  key;
    uint8_t  t = 0;
    audio_nav_t res = AUDIO_STOP;
    uint8_t  quit;
    uint32_t fillnum;

    s_dev.file = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    s_dev.saibuf1 = mymalloc(SRAMIN, AUDIO_SAI_TX_BUF_SIZE);
    s_dev.saibuf2 = mymalloc(SRAMIN, AUDIO_SAI_TX_BUF_SIZE);
    s_dev.tbuf = mymalloc(SRAMIN, AUDIO_SAI_TX_BUF_SIZE);

    if ((s_dev.file == NULL) || (s_dev.saibuf1 == NULL) ||
        (s_dev.saibuf2 == NULL) || (s_dev.tbuf == NULL))
    {
        res = AUDIO_ERROR;
    }
    else if (wav_decode_init(fname, &wavctrl) != 0)
    {
        res = AUDIO_ERROR;
    }
    else if (wavctrl.bps == 16)
    {
        es8388_sai_cfg(0, 3);           /* standard I2S, 16-bit */
        sai1_saia_init(SAI_MODEMASTER_TX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_16);
        sai1_tx_dma_init(s_dev.saibuf1, s_dev.saibuf2, AUDIO_SAI_TX_BUF_SIZE / 2, 1);
    }
    else if (wavctrl.bps == 24)
    {
        es8388_sai_cfg(0, 0);           /* standard I2S, 24-bit */
        sai1_saia_init(SAI_MODEMASTER_TX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_24);
        sai1_tx_dma_init(s_dev.saibuf1, s_dev.saibuf2, AUDIO_SAI_TX_BUF_SIZE / 4, 2);
    }
    else
    {
        res = AUDIO_ERROR;
    }

    if (res == AUDIO_STOP)
    {
        (void)sai1_samplerate_set(wavctrl.samplerate);
        sai_tx_callback = audio_sai_tx_callback;
        audio_stop();

        fres = f_open(s_dev.file, (TCHAR *)fname, FA_READ);

        if (fres != FR_OK)
        {
            res = AUDIO_ERROR;
        }
        else
        {
            (void)f_lseek(s_dev.file, wavctrl.datastart);
            fillnum = wav_buffill(s_dev.saibuf1, AUDIO_SAI_TX_BUF_SIZE, (uint8_t)wavctrl.bps);
            fillnum = wav_buffill(s_dev.saibuf2, AUDIO_SAI_TX_BUF_SIZE, (uint8_t)wavctrl.bps);
            audio_start();

            for (;;)
            {
                while (!s_transfer_end)
                {
                    /* wait for a half-buffer to finish */
                }
                s_transfer_end = false;

                if (fillnum != AUDIO_SAI_TX_BUF_SIZE)   /* end of stream */
                {
                    res = AUDIO_STOP;
                    break;
                }

                if (s_witch_buf)
                {
                    fillnum = wav_buffill(s_dev.saibuf2, AUDIO_SAI_TX_BUF_SIZE, (uint8_t)wavctrl.bps);
                }
                else
                {
                    fillnum = wav_buffill(s_dev.saibuf1, AUDIO_SAI_TX_BUF_SIZE, (uint8_t)wavctrl.bps);
                }

                quit = 0;

                while (quit == 0)
                {
                    key = (uint8_t)key_scan(false);

                    if (key == KEY_WKUP)            /* pause / resume */
                    {
                        if (s_dev.status & 0x01)
                        {
                            s_dev.status &= (uint8_t)~(1 << 0);
                        }
                        else
                        {
                            s_dev.status |= 0x01;
                        }
                    }

                    if (key == KEY2)
                    {
                        res = AUDIO_PREV;
                        quit = 1;
                        break;
                    }

                    if (key == KEY0)
                    {
                        res = AUDIO_NEXT;
                        quit = 1;
                        break;
                    }

                    wav_get_curtime(s_dev.file, &wavctrl);
                    audio_msg_show(wavctrl.totsec, wavctrl.cursec, wavctrl.bitrate);

                    t++;
                    if (t == 20)
                    {
                        t = 0;
                        led_toggle(LED0);
                    }

                    if ((s_dev.status & 0x01) == 0)
                    {
                        delay_ms(10);
                    }
                    else
                    {
                        break;
                    }
                }

                if (quit != 0)
                {
                    break;
                }
            }

            audio_stop();
        }
    }

    myfree(SRAMIN, s_dev.tbuf);
    myfree(SRAMIN, s_dev.saibuf1);
    myfree(SRAMIN, s_dev.saibuf2);
    myfree(SRAMIN, s_dev.file);

    return res;
}
