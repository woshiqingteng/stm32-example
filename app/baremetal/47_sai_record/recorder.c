/**
 * @file    recorder.c
 * @brief   WAV recorder state machine.
 *
 * The capture DMA writes into two half-buffers. A small circular FIFO decouples
 * the interrupt from the (possibly slow) FatFs writes: the ISR pushes completed
 * half-buffers, the main loop drains them into the file. Recording, pause,
 * stop-and-save and play-back are driven by the on-board keys.
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "ff.h"
#include "malloc.h"
#include "wavplay.h"
#include "recorder.h"

static uint8_t *p_sai_recbuf1;      /* capture DMA half-buffer 1 */
static uint8_t *p_sai_recbuf2;      /* capture DMA half-buffer 2 */

/* Record FIFO: the ISR pushes completed buffers, the main loop drains them. */
static volatile uint8_t g_sai_recfifo_rdpos = 0;
static volatile uint8_t g_sai_recfifo_wrpos = 0;
static uint8_t *p_sai_recfifo_buf[REC_SAI_RX_FIFO_SIZE];

static uint32_t g_wav_size;         /* recorded PCM size, excluding the header */
static uint8_t  g_rec_sta = 0;      /* bit7 recording, bit0 paused */

/* Two zero samples keep the SAI TX master clock running while recording. */
static const uint16_t SAI_PLAY_BUF[2] = { 0x0000, 0x0000 };

uint8_t recoder_sai_fifo_read(uint8_t **buf)
{
    if (g_sai_recfifo_rdpos == g_sai_recfifo_wrpos)     /* empty */
    {
        return 0;
    }

    g_sai_recfifo_rdpos++;

    if (g_sai_recfifo_rdpos >= REC_SAI_RX_FIFO_SIZE)
    {
        g_sai_recfifo_rdpos = 0;
    }

    *buf = p_sai_recfifo_buf[g_sai_recfifo_rdpos];

    return 1;
}

uint8_t recoder_sai_fifo_write(uint8_t *buf)
{
    uint16_t i;
    uint8_t  temp = g_sai_recfifo_wrpos;

    g_sai_recfifo_wrpos++;

    if (g_sai_recfifo_wrpos >= REC_SAI_RX_FIFO_SIZE)
    {
        g_sai_recfifo_wrpos = 0;
    }

    if (g_sai_recfifo_wrpos == g_sai_recfifo_rdpos)     /* full */
    {
        g_sai_recfifo_wrpos = temp;
        return 1;
    }

    for (i = 0; i < REC_SAI_RX_DMA_BUF_SIZE; i++)
    {
        p_sai_recfifo_buf[g_sai_recfifo_wrpos][i] = buf[i];
    }

    return 0;
}

void recoder_sai_dma_rx_callback(void)
{
    if (g_rec_sta == 0x80)                          /* record mode */
    {
        if (sai1_rx_dma_target() != 0U)
        {
            (void)recoder_sai_fifo_write(p_sai_recbuf1);
        }
        else
        {
            (void)recoder_sai_fifo_write(p_sai_recbuf2);
        }
    }
}

void recoder_enter_rec_mode(void)
{
    sai1_tx_dma_irq_disable();          /* no TX IRQ while injecting zeros */

    es8388_adda_cfg(0, 1);              /* enable ADC */
    es8388_input_cfg(0);                /* channel 1, MIC input */
    es8388_mic_gain(8);                 /* maximum MIC gain */
    es8388_alc_ctrl(3, 4, 4);           /* ALC on, for a stable recording level */
    es8388_output_cfg(0, 0);            /* outputs off */
    es8388_spkvol_set(0);
    es8388_sai_cfg(0, 3);               /* standard I2S, 16-bit */

    sai1_saia_init(SAI_MODEMASTER_TX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_16);
    sai1_saib_init(SAI_MODESLAVE_RX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_16);
    (void)sai1_samplerate_set(REC_SAMPLERATE);

    sai1_tx_dma_init((uint8_t *)&SAI_PLAY_BUF[0], (uint8_t *)&SAI_PLAY_BUF[1], 1, 1);
    sai1_tx_dma_irq_disable();
    sai1_rx_dma_init(p_sai_recbuf1, p_sai_recbuf2, REC_SAI_RX_DMA_BUF_SIZE / 2, 1);

    sai_rx_callback = recoder_sai_dma_rx_callback;

    sai1_play_start();
    sai1_rec_start();

    recoder_remindmsg_show(0);
}

void recoder_enter_play_mode(void)
{
    es8388_adda_cfg(1, 0);      /* enable DAC */
    es8388_output_cfg(1, 1);    /* enable outputs */
    es8388_spkvol_set(28);
    sai1_play_stop();
    sai1_rec_stop();

    recoder_remindmsg_show(1);
}

void recoder_wav_init(__WaveHeader *wavhead)
{
    wavhead->riff.ChunkID = 0x46464952;                  /* "RIFF" */
    wavhead->riff.ChunkSize = 0;                         /* filled on stop */
    wavhead->riff.Format = 0x45564157;                   /* "WAVE" */
    wavhead->fmt.ChunkID = 0x20746D66;                   /* "fmt " */
    wavhead->fmt.ChunkSize = 16;
    wavhead->fmt.AudioFormat = 0x01;                     /* PCM */
    wavhead->fmt.NumOfChannels = 2;                      /* stereo */
    wavhead->fmt.SampleRate = REC_SAMPLERATE;
    wavhead->fmt.ByteRate = REC_SAMPLERATE * 4U;         /* rate * channels * 2 */
    wavhead->fmt.BlockAlign = 4;
    wavhead->fmt.BitsPerSample = 16;
    wavhead->data.ChunkID = 0x61746164;                  /* "data" */
    wavhead->data.ChunkSize = 0;                         /* filled on stop */
}

void recoder_msg_show(uint32_t tsec, uint32_t kbps)
{
    printf("TIME %02lu:%02lu KPBS %lu\r\n",
           (unsigned long)(tsec / 60U), (unsigned long)(tsec % 60U),
           (unsigned long)(kbps / 1000U));
}

void recoder_remindmsg_show(uint8_t mode)
{
    if (mode == 0)      /* record mode */
    {
        printf("KEY0:REC/PAUSE  KEY2:STOP&SAVE  WK_UP:PLAY\r\n");
    }
    else                /* playback mode */
    {
        printf("KEY0:STOP Play  WK_UP:PLAY/PAUSE\r\n");
    }
}

void recoder_new_pathname(char *pname)
{
    uint8_t  res;
    uint16_t index = 0;
    FIL     *ftemp;

    ftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));

    if (ftemp == NULL)
    {
        pname[0] = '\0';
        return;
    }

    while (index < 0xFFFFU)
    {
        (void)sprintf(pname, "0:RECORDER/REC%05d.wav", index);
        res = (uint8_t)f_open(ftemp, (const TCHAR *)pname, FA_READ);

        if (res == FR_NO_FILE)
        {
            break;
        }

        index++;
    }

    myfree(SRAMIN, ftemp);
}

void wav_recorder(void)
{
    uint8_t  res;
    uint8_t  i;
    uint8_t  key;
    uint8_t  rval = 0;
    UINT     bw;

    __WaveHeader *wavhead;
    DIR           recdir;
    FIL          *f_rec = NULL;

    uint8_t  *pdatabuf;
    char     *pname = NULL;
    uint8_t   timecnt = 0;
    uint32_t  recsec = 0;

    if (f_opendir(&recdir, "0:/RECORDER") != FR_OK)
    {
        (void)f_mkdir("0:/RECORDER");
    }
    else
    {
        (void)f_closedir(&recdir);
    }

    for (i = 0; i < REC_SAI_RX_FIFO_SIZE; i++)
    {
        p_sai_recfifo_buf[i] = mymalloc(SRAMIN, REC_SAI_RX_DMA_BUF_SIZE);

        if (p_sai_recfifo_buf[i] == NULL)
        {
            break;
        }
    }

    p_sai_recbuf1 = mymalloc(SRAMIN, REC_SAI_RX_DMA_BUF_SIZE);
    p_sai_recbuf2 = mymalloc(SRAMIN, REC_SAI_RX_DMA_BUF_SIZE);
    f_rec = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    wavhead = (__WaveHeader *)mymalloc(SRAMIN, sizeof(__WaveHeader));
    pname = (char *)mymalloc(SRAMIN, 30);

    if ((p_sai_recbuf1 == NULL) || (p_sai_recbuf2 == NULL) ||
        (f_rec == NULL) || (wavhead == NULL) || (pname == NULL))
    {
        rval = 1;
    }

    if (rval == 0)
    {
        recoder_enter_rec_mode();
        pname[0] = '\0';

        while (rval == 0)
        {
            key = (uint8_t)key_scan(false);

            switch (key)
            {
                case KEY2:      /* stop and save */
                    if (g_rec_sta & 0x80)
                    {
                        g_rec_sta = 0;
                        wavhead->riff.ChunkSize = g_wav_size + 36U;
                        wavhead->data.ChunkSize = g_wav_size;
                        (void)f_lseek(f_rec, 0);
                        (void)f_write(f_rec, (const void *)wavhead, sizeof(__WaveHeader), &bw);
                        (void)f_close(f_rec);
                        g_wav_size = 0;
                    }

                    g_rec_sta = 0;
                    recsec = 0;
                    led_off(LED1);
                    break;

                case KEY0:      /* record / pause */
                    if (g_rec_sta & 0x01)               /* paused: resume */
                    {
                        g_rec_sta &= (uint8_t)0xFE;
                    }
                    else if (g_rec_sta & 0x80)          /* recording: pause */
                    {
                        g_rec_sta |= 0x01;
                    }
                    else                                /* start a new recording */
                    {
                        recsec = 0;
                        recoder_new_pathname(pname);
                        printf("rec: %s\r\n", pname + 11);
                        recoder_wav_init(wavhead);

                        res = (uint8_t)f_open(f_rec, (const TCHAR *)pname, FA_CREATE_ALWAYS | FA_WRITE);

                        if (res != 0U)
                        {
                            g_rec_sta = 0;
                            rval = 0xFE;
                        }
                        else
                        {
                            (void)f_write(f_rec, (const void *)wavhead, sizeof(__WaveHeader), &bw);
                            recoder_msg_show(0, 0);
                            g_rec_sta |= 0x80;
                        }
                    }

                    if (g_rec_sta & 0x01)
                    {
                        led_on(LED1);       /* pause indicator */
                    }
                    else
                    {
                        led_off(LED1);
                    }
                    break;

                case KEY_WKUP:      /* play the last recording */
                    if (g_rec_sta != 0x80)
                    {
                        if (pname[0] != '\0')
                        {
                            printf("play: %s\r\n", pname + 11);
                            recoder_enter_play_mode();
                            (void)wav_play_song(pname);
                            recoder_enter_rec_mode();
                        }
                    }
                    break;

                default:
                    break;
            }

            if (recoder_sai_fifo_read(&pdatabuf) != 0U)     /* drain one FIFO buffer */
            {
                res = (uint8_t)f_write(f_rec, pdatabuf, REC_SAI_RX_DMA_BUF_SIZE, &bw);

                if (res != 0U)
                {
                    printf("write error:%d\r\n", (int)res);
                }

                g_wav_size += REC_SAI_RX_DMA_BUF_SIZE;
            }
            else
            {
                delay_ms(5);
            }

            timecnt++;

            if ((timecnt % 20U) == 0U)
            {
                led_toggle(LED0);
            }

            if (recsec != (g_wav_size / wavhead->fmt.ByteRate))
            {
                led_toggle(LED1);
                recsec = g_wav_size / wavhead->fmt.ByteRate;
                recoder_msg_show(recsec,
                                 (uint32_t)wavhead->fmt.SampleRate * wavhead->fmt.NumOfChannels * wavhead->fmt.BitsPerSample);
            }
        }
    }

    for (i = 0; i < REC_SAI_RX_FIFO_SIZE; i++)
    {
        myfree(SRAMIN, p_sai_recfifo_buf[i]);
    }

    myfree(SRAMIN, p_sai_recbuf1);
    myfree(SRAMIN, p_sai_recbuf2);
    myfree(SRAMIN, f_rec);
    myfree(SRAMIN, wavhead);
    myfree(SRAMIN, pname);
}
