/**
 * @file    videoplayer.c
 * @brief   MJPEG / AVI video player with synchronised ES8388 + SAI audio.
 *
 * The AVI container interleaves "dc" (MJPEG video) and "wb" (PCM audio)
 * chunks. Video frames are decoded and drawn by the MJPEG middleware; audio
 * chunks feed a four-deep SAI ring. A TIM7 tick paces the video frames to the
 * rate declared by the AVI header.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "text.h"
#include "malloc.h"
#include "audio.h"
#include "mjpeg.h"
#include "avi.h"
#include "videoplayer.h"

#define VIDEO_DIR           "0:/VIDEO"
#define VIDEO_MAX_FILES     32U
#define VIDEO_NAME_LEN      64U

/* TIM7 video frame pacing. */
static TIM_HandleTypeDef g_vtim_handle;
uint16_t                 g_avi_frame;
volatile bool            g_avi_frameup;

/* Audio ring shared with the SAI DMA callback. */
volatile uint8_t g_avi_sai_playbuf;
uint8_t         *p_avi_sai_buf[AVI_AUDIO_BUF_NUM];

static char g_video_names[VIDEO_MAX_FILES][VIDEO_NAME_LEN];
static uint16_t g_video_count;

static void vtimer_init(uint16_t arr, uint16_t psc)
{
    __HAL_RCC_TIM7_CLK_ENABLE();

    g_vtim_handle.Instance = TIM7;
    g_vtim_handle.Init.Prescaler = psc;
    g_vtim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_vtim_handle.Init.Period = arr;
    g_vtim_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    (void)HAL_TIM_Base_Init(&g_vtim_handle);

    HAL_NVIC_SetPriority(TIM7_IRQn, 0, 3);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);
    (void)HAL_TIM_Base_Start_IT(&g_vtim_handle);
}

static void vtimer_stop(void)
{
    __HAL_TIM_DISABLE(&g_vtim_handle);
}

void TIM7_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_vtim_handle, TIM_FLAG_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&g_vtim_handle, TIM_FLAG_UPDATE);
        g_avi_frameup = true;
        led_toggle(LED1);
    }
}

void audio_sai_dma_callback(void)
{
    g_avi_sai_playbuf++;

    if (g_avi_sai_playbuf >= AVI_AUDIO_BUF_NUM)
    {
        g_avi_sai_playbuf = 0;
    }

    sai1_tx_dma_set_inactive_buffer(p_avi_sai_buf[g_avi_sai_playbuf]);
}

uint16_t video_get_tnum(char *path)
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

        if ((exfuns_file_type(tfileinfo.fname) & 0xF0U) == 0x60U)
        {
            rval++;
        }
    }

    (void)f_closedir(&tdir);

    return rval;
}

static void video_scan(void)
{
    DIR     dir;
    FILINFO fno;

    g_video_count = 0;

    if (f_opendir(&dir, VIDEO_DIR) != FR_OK)
    {
        printf("opendir %s failed\r\n", VIDEO_DIR);
        return;
    }

    for (;;)
    {
        if ((f_readdir(&dir, &fno) != FR_OK) || (fno.fname[0] == 0))
        {
            break;
        }

        if (((exfuns_file_type(fno.fname) & 0xF0U) == 0x60U) && (g_video_count < VIDEO_MAX_FILES))
        {
            (void)strncpy(g_video_names[g_video_count], fno.fname, VIDEO_NAME_LEN - 1U);
            g_video_names[g_video_count][VIDEO_NAME_LEN - 1U] = '\0';
            g_video_count++;
        }
    }

    (void)f_closedir(&dir);
    printf("found %u video(s)\r\n", (unsigned int)g_video_count);
}

void video_time_show(void *favi, AVI_INFO *aviinfo)
{
    FIL            *file = (FIL *)favi;
    static uint32_t oldsec;
    char            buf[48];
    uint32_t        totsec;
    uint32_t        cursec;

    totsec = (aviinfo->SecPerFrame / 1000U) * aviinfo->TotalFrame;
    totsec /= 1000U;
    cursec = (uint32_t)(((double)file->fptr / (double)file->obj.objsize) * (double)totsec);

    if (oldsec != cursec)
    {
        oldsec = cursec;
        (void)sprintf(buf, "time:%02u:%02u:%02u/%02u:%02u:%02u",
                      (unsigned int)(cursec / 3600U), (unsigned int)((cursec % 3600U) / 60U), (unsigned int)(cursec % 60U),
                      (unsigned int)(totsec / 3600U), (unsigned int)((totsec % 3600U) / 60U), (unsigned int)(totsec % 60U));
        text_show_string(10, 90, lcd_get_width() - 10, 16, buf, 16, 0, RED);
    }
}

void video_info_show(AVI_INFO *aviinfo)
{
    char buf[48];

    (void)sprintf(buf, "audio:%u, rate:%u", (unsigned int)aviinfo->Channels,
                  (unsigned int)(aviinfo->SampleRate * 10U));
    text_show_string(10, 50, lcd_get_width() - 10, 16, buf, 16, 0, RED);

    (void)sprintf(buf, "fps:%u", (unsigned int)(1000U / (aviinfo->SecPerFrame / 1000U)));
    text_show_string(10, 70, lcd_get_width() - 10, 16, buf, 16, 0, RED);
}

void video_bmsg_show(char *name, uint16_t index, uint16_t total)
{
    char buf[80];

    (void)sprintf(buf, "file:%s", name);
    text_show_string(10, 10, lcd_get_width() - 10, 16, buf, 16, 0, RED);

    (void)sprintf(buf, "index:%u/%u", (unsigned int)index, (unsigned int)total);
    text_show_string(10, 30, lcd_get_width() - 10, 16, buf, 16, 0, RED);
}

void video_play(void)
{
    uint16_t index = 0;
    char     path[VIDEO_NAME_LEN + 16];
    uint8_t  key;

    for (;;)
    {
        video_scan();

        if (g_video_count == 0U)
        {
            text_show_string(60, 190, 240, 16, "No video files!", 16, 0, RED);
            delay_ms(500);
            continue;
        }

        break;
    }

    for (;;)
    {
        (void)sprintf(path, "%s/%s", VIDEO_DIR, g_video_names[index]);

        lcd_clear(BLACK);
        video_bmsg_show(g_video_names[index], (uint16_t)(index + 1U), g_video_count);

        key = video_play_mjpeg(path);

        if (key == AUDIO_PREV)
        {
            index = (index == 0U) ? (uint16_t)(g_video_count - 1U) : (uint16_t)(index - 1U);
        }
        else if (key == AUDIO_NEXT)
        {
            index = (uint16_t)((index + 1U) % g_video_count);
        }
        else
        {
            break;
        }
    }
}

uint8_t video_play_mjpeg(char *pname)
{
    uint8_t   *framebuf;
    uint8_t   *pbuf;
    FIL       *favi;
    FRESULT    fres;
    uint8_t    res = AUDIO_STOP;
    uint8_t    i;
    uint16_t   offset;
    uint32_t   nr;
    uint8_t    key;
    uint8_t    saisavebuf = 0;

    for (i = 0; i < AVI_AUDIO_BUF_NUM; i++)
    {
        p_avi_sai_buf[i] = mymalloc(SRAMIN, AVI_AUDIO_BUF_SIZE);

        if (p_avi_sai_buf[i] == NULL)
        {
            break;
        }

        memset(p_avi_sai_buf[i], 0, AVI_AUDIO_BUF_SIZE);
    }

    favi = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    framebuf = mymalloc(SRAMIN, AVI_VIDEO_BUF_SIZE);

    if ((framebuf == NULL) || (favi == NULL))
    {
        printf("memory error!\r\n");
        res = AUDIO_ERROR;
    }
    else
    {
        fres = f_open(favi, (const TCHAR *)pname, FA_READ);

        if (fres != FR_OK)
        {
            res = AUDIO_ERROR;
        }
        else
        {
            pbuf = framebuf;
            fres = f_read(favi, pbuf, AVI_VIDEO_BUF_SIZE, (UINT *)&nr);

            if (fres != FR_OK)
            {
                printf("fread error:%d\r\n", (int)fres);
                res = AUDIO_ERROR;
            }
            else if (avi_init(pbuf, AVI_VIDEO_BUF_SIZE) != AVI_OK)
            {
                printf("avi format error\r\n");
                res = AUDIO_ERROR;
            }
            else
            {
                video_info_show(&g_avix);
                vtimer_init((uint16_t)(g_avix.SecPerFrame / 100U - 1U), 9000U - 1U);

                offset = (uint16_t)avi_srarch_id(pbuf, AVI_VIDEO_BUF_SIZE, "movi");
                (void)avi_get_streaminfo(pbuf + offset + 4);
                (void)f_lseek(favi, offset + 12);       /* skip to the first chunk payload */
                (void)mjpegdec_init((uint16_t)((lcd_get_width() - g_avix.Width) / 2U),
                                    (uint16_t)(110U + (lcd_get_height() - 110U - g_avix.Height) / 2U));

                if (g_avix.SampleRate != 0U)            /* initialise audio playback */
                {
                    es8388_sai_cfg(0, 3);               /* standard I2S, 16-bit */
                    sai1_saia_init(SAI_MODEMASTER_TX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_16);
                    (void)sai1_samplerate_set(g_avix.SampleRate);
                    sai1_tx_dma_init(p_avi_sai_buf[0], p_avi_sai_buf[1], g_avix.AudioBufSize / 2U, 1);
                    sai_tx_callback = audio_sai_dma_callback;
                    g_avi_sai_playbuf = 0;
                    saisavebuf = 0;
                    sai1_play_start();
                }

                for (;;)
                {
                    if (g_avix.StreamID == AVI_VIDS_FLAG)   /* video chunk */
                    {
                        pbuf = framebuf;
                        (void)f_read(favi, pbuf, g_avix.StreamSize + 8U, (UINT *)&nr);
                        (void)mjpegdec_decode(pbuf, g_avix.StreamSize);

                        while (!g_avi_frameup)
                        {
                            /* wait for the frame period */
                        }
                        g_avi_frameup = false;
                        g_avi_frame++;
                    }
                    else                                    /* audio chunk */
                    {
                        video_time_show(favi, &g_avix);
                        saisavebuf++;

                        if (saisavebuf > 3U)
                        {
                            saisavebuf = 0;
                        }

                        do
                        {
                            nr = g_avi_sai_playbuf;

                            if (nr != 0U)
                            {
                                nr--;
                            }
                            else
                            {
                                nr = 3U;
                            }
                        } while (saisavebuf == (uint8_t)nr);    /* avoid overwriting the active buffer */

                        (void)f_read(favi, p_avi_sai_buf[saisavebuf], g_avix.StreamSize + 8U, (UINT *)&nr);
                        pbuf = p_avi_sai_buf[saisavebuf];
                    }

                    key = (uint8_t)key_scan(false);

                    if ((key == KEY0) || (key == KEY2))         /* next / previous file */
                    {
                        res = (key == KEY0) ? AUDIO_NEXT : AUDIO_PREV;
                        break;
                    }
                    else if ((key == KEY1) || (key == KEY_WKUP))/* seek */
                    {
                        sai1_play_stop();
                        (void)video_seek(favi, &g_avix, framebuf);
                        pbuf = framebuf;
                        sai1_play_start();
                    }
                    else
                    {
                        /* no key */
                    }

                    if (avi_get_streaminfo(pbuf + g_avix.StreamSize) != AVI_OK)
                    {
                        printf("frame error\r\n");
                        res = AUDIO_NEXT;
                        break;
                    }
                }

                sai1_play_stop();
                vtimer_stop();
                mjpegdec_free();
            }

            (void)f_close(favi);
        }
    }

    for (i = 0; i < AVI_AUDIO_BUF_NUM; i++)
    {
        myfree(SRAMIN, p_avi_sai_buf[i]);
    }
    myfree(SRAMIN, framebuf);
    myfree(SRAMIN, favi);

    return res;
}

uint8_t video_seek(void *favi, AVI_INFO *aviinfo, uint8_t *mbuf)
{
    FIL      *file = (FIL *)favi;
    uint32_t fpos = (uint32_t)file->fptr;
    uint8_t *pbuf;
    uint16_t offset;
    uint32_t br;
    uint32_t delta;
    uint32_t totsec;
    uint8_t  key;

    totsec = (aviinfo->SecPerFrame / 1000U) * aviinfo->TotalFrame;
    totsec /= 1000U;
    delta = (uint32_t)((file->obj.objsize / totsec) * 5U);     /* ~5 s of data */

    for (;;)
    {
        key = (uint8_t)key_scan(true);

        if (key == KEY_WKUP)                    /* fast forward */
        {
            if (fpos < (uint32_t)file->obj.objsize)
            {
                fpos += delta;
            }

            if (fpos > ((uint32_t)file->obj.objsize - AVI_VIDEO_BUF_SIZE))
            {
                fpos = (uint32_t)file->obj.objsize - AVI_VIDEO_BUF_SIZE;
            }
        }
        else if (key == KEY1)                   /* rewind */
        {
            if (fpos > delta)
            {
                fpos -= delta;
            }
            else
            {
                fpos = 0;
            }
        }
        else
        {
            break;
        }

        (void)f_lseek(favi, fpos);
        (void)f_read(favi, mbuf, AVI_VIDEO_BUF_SIZE, (UINT *)&br);
        pbuf = mbuf;

        if (fpos == 0U)
        {
            offset = (uint16_t)avi_srarch_id(pbuf, AVI_VIDEO_BUF_SIZE, "movi");
        }
        else
        {
            offset = 0;
        }

        offset = (uint16_t)(offset + avi_srarch_id(pbuf + offset, AVI_VIDEO_BUF_SIZE, g_avix.VideoFLAG));
        (void)avi_get_streaminfo(pbuf + offset);
        (void)f_lseek(favi, fpos + offset + 8U);

        if (g_avix.StreamID == AVI_VIDS_FLAG)
        {
            (void)f_read(favi, mbuf, g_avix.StreamSize + 8U, (UINT *)&br);
            (void)mjpegdec_decode(mbuf, g_avix.StreamSize);
        }

        video_time_show(favi, &g_avix);
    }

    return 0;
}
