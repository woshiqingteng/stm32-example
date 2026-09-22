/**
 * @file    wavplay.c
 * @brief   WAV playback (16-bit / 24-bit PCM) over the ES8388 + SAI path.
 *
 * The file header is parsed first, then the SAI is configured for the source
 * bit depth and sample rate. Two half buffers feed the circular TX DMA; the
 * half-transfer callback is shared with the audio device (audio.c).
 */

#include <stdio.h>
#include "bsp.h"
#include "ff.h"
#include "malloc.h"
#include "audio.h"
#include "wavplay.h"

__wavctrl wavctrl;      /* parsed WAV control block */

uint8_t wav_decode_init(char *fname, __wavctrl *wavx)
{
    FIL      *ftemp;
    uint8_t  *buf;
    uint32_t  br = 0;
    uint8_t   res = 0;

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
                    res = 3;        /* data chunk not found */
                }
            }
            else
            {
                res = 2;            /* not a WAV file */
            }
        }
        else
        {
            res = 1;                /* open failed */
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
        (void)f_read(g_audiodev.file, g_audiodev.tbuf, readlen, (UINT *)&bread);
        pbuf = (uint32_t *)buf;

        for (i = 0; i < (size / 4); i++)
        {
            const uint8_t *b = g_audiodev.tbuf + (i * 3);

            pbuf[i] = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16);
        }

        bread = (bread * 4) / 3;
    }
    else
    {
        (void)f_read(g_audiodev.file, buf, size, (UINT *)&bread);

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

uint8_t wav_play_song(char *fname)
{
    FRESULT  fres;
    uint8_t  key;
    uint8_t  t = 0;
    uint8_t  res = AUDIO_STOP;
    uint8_t  quit;
    uint32_t fillnum;

    g_audiodev.file = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    g_audiodev.saibuf1 = mymalloc(SRAMIN, AUDIO_SAI_TX_BUF_SIZE);
    g_audiodev.saibuf2 = mymalloc(SRAMIN, AUDIO_SAI_TX_BUF_SIZE);
    g_audiodev.tbuf = mymalloc(SRAMIN, AUDIO_SAI_TX_BUF_SIZE);

    if ((g_audiodev.file == NULL) || (g_audiodev.saibuf1 == NULL) ||
        (g_audiodev.saibuf2 == NULL) || (g_audiodev.tbuf == NULL))
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
        sai1_tx_dma_init(g_audiodev.saibuf1, g_audiodev.saibuf2, AUDIO_SAI_TX_BUF_SIZE / 2, 1);
    }
    else if (wavctrl.bps == 24)
    {
        es8388_sai_cfg(0, 0);           /* standard I2S, 24-bit */
        sai1_saia_init(SAI_MODEMASTER_TX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_24);
        sai1_tx_dma_init(g_audiodev.saibuf1, g_audiodev.saibuf2, AUDIO_SAI_TX_BUF_SIZE / 4, 2);
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

        fres = f_open(g_audiodev.file, (TCHAR *)fname, FA_READ);

        if (fres != FR_OK)
        {
            res = AUDIO_ERROR;
        }
        else
        {
            (void)f_lseek(g_audiodev.file, wavctrl.datastart);
            fillnum = wav_buffill(g_audiodev.saibuf1, AUDIO_SAI_TX_BUF_SIZE, (uint8_t)wavctrl.bps);
            fillnum = wav_buffill(g_audiodev.saibuf2, AUDIO_SAI_TX_BUF_SIZE, (uint8_t)wavctrl.bps);
            audio_start();

            for (;;)
            {
                while (audio_transfer_end == 0)
                {
                    /* wait for a half-buffer to finish */
                }
                audio_transfer_end = 0;

                if (fillnum != AUDIO_SAI_TX_BUF_SIZE)   /* end of stream */
                {
                    res = AUDIO_STOP;
                    break;
                }

                if (audio_witch_buf)
                {
                    fillnum = wav_buffill(g_audiodev.saibuf2, AUDIO_SAI_TX_BUF_SIZE, (uint8_t)wavctrl.bps);
                }
                else
                {
                    fillnum = wav_buffill(g_audiodev.saibuf1, AUDIO_SAI_TX_BUF_SIZE, (uint8_t)wavctrl.bps);
                }

                quit = 0;

                while (quit == 0)
                {
                    key = (uint8_t)key_scan(false);

                    if (key == KEY_WKUP)            /* pause / resume */
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

                    wav_get_curtime(g_audiodev.file, &wavctrl);
                    audio_msg_show(wavctrl.totsec, wavctrl.cursec, wavctrl.bitrate);

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

    myfree(SRAMIN, g_audiodev.tbuf);
    myfree(SRAMIN, g_audiodev.saibuf1);
    myfree(SRAMIN, g_audiodev.saibuf2);
    myfree(SRAMIN, g_audiodev.file);

    return res;
}
