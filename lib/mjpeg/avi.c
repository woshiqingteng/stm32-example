/**
 * @file    avi.c
 * @brief   AVI (Motion JPEG + PCM/WAV) container parser.
 */

#include <stdio.h>
#include "avi.h"

AVI_INFO g_avix;

static char *const AVI_VIDS_FLAG_TBL[2] = { "00dc", "01dc" };
static char *const AVI_AUDS_FLAG_TBL[2] = { "00wb", "01wb" };

AVISTATUS avi_init(uint8_t *buf, uint32_t size)
{
    uint16_t      offset;
    uint8_t      *tbuf;
    AVISTATUS     res = AVI_OK;
    AVI_HEADER   *aviheader;
    LIST_HEADER  *listheader;
    AVIH_HEADER  *avihheader;
    STRH_HEADER  *strhheader;
    STRF_BMPHEADER *bmpheader;
    STRF_WAVHEADER *wavheader;

    tbuf = buf;
    aviheader = (AVI_HEADER *)buf;

    if (aviheader->RiffID != AVI_RIFF_ID)
    {
        return AVI_RIFF_ERR;
    }

    if (aviheader->AviID != AVI_AVI_ID)
    {
        return AVI_AVI_ERR;
    }

    buf += sizeof(AVI_HEADER);
    listheader = (LIST_HEADER *)(buf);

    if (listheader->ListID != AVI_LIST_ID)
    {
        return AVI_LIST_ERR;
    }

    if (listheader->ListType != AVI_HDRL_ID)
    {
        return AVI_HDRL_ERR;
    }

    buf += sizeof(LIST_HEADER);
    avihheader = (AVIH_HEADER *)(buf);

    if (avihheader->BlockID != AVI_AVIH_ID)
    {
        return AVI_AVIH_ERR;
    }

    g_avix.SecPerFrame = avihheader->SecPerFrame;
    g_avix.TotalFrame = avihheader->TotalFrame;
    buf += avihheader->BlockSize + 8;
    listheader = (LIST_HEADER *)(buf);

    if (listheader->ListID != AVI_LIST_ID)
    {
        return AVI_LIST_ERR;
    }

    if (listheader->ListType != AVI_STRL_ID)
    {
        return AVI_STRL_ERR;
    }

    strhheader = (STRH_HEADER *)(buf + 12);

    if (strhheader->BlockID != AVI_STRH_ID)
    {
        return AVI_STRH_ERR;
    }

    if (strhheader->StreamType == AVI_VIDS_STREAM)  /* video stream first */
    {
        if (strhheader->Handler != AVI_FORMAT_MJPG)
        {
            return AVI_FORMAT_ERR;
        }

        g_avix.VideoFLAG = AVI_VIDS_FLAG_TBL[0];
        g_avix.AudioFLAG = AVI_AUDS_FLAG_TBL[1];
        bmpheader = (STRF_BMPHEADER *)(buf + 12 + strhheader->BlockSize + 8);

        if (bmpheader->BlockID != AVI_STRF_ID)
        {
            return AVI_STRF_ERR;
        }

        g_avix.Width = (uint32_t)bmpheader->bmiHeader.Width;
        g_avix.Height = (uint32_t)bmpheader->bmiHeader.Height;
        buf += listheader->BlockSize + 8;
        listheader = (LIST_HEADER *)(buf);

        if (listheader->ListID != AVI_LIST_ID)      /* no audio stream */
        {
            g_avix.SampleRate = 0;
            g_avix.Channels = 0;
            g_avix.AudioType = 0;
        }
        else
        {
            if (listheader->ListType != AVI_STRL_ID)
            {
                return AVI_STRL_ERR;
            }

            strhheader = (STRH_HEADER *)(buf + 12);

            if (strhheader->BlockID != AVI_STRH_ID)
            {
                return AVI_STRH_ERR;
            }

            if (strhheader->StreamType != AVI_AUDS_STREAM)
            {
                return AVI_FORMAT_ERR;
            }

            wavheader = (STRF_WAVHEADER *)(buf + 12 + strhheader->BlockSize + 8);

            if (wavheader->BlockID != AVI_STRF_ID)
            {
                return AVI_STRF_ERR;
            }

            g_avix.SampleRate = wavheader->SampleRate;
            g_avix.Channels = wavheader->Channels;
            g_avix.AudioType = wavheader->FormatTag;
        }
    }
    else if (strhheader->StreamType == AVI_AUDS_STREAM)     /* audio stream first */
    {
        g_avix.VideoFLAG = AVI_VIDS_FLAG_TBL[1];
        g_avix.AudioFLAG = AVI_AUDS_FLAG_TBL[0];
        wavheader = (STRF_WAVHEADER *)(buf + 12 + strhheader->BlockSize + 8);

        if (wavheader->BlockID != AVI_STRF_ID)
        {
            return AVI_STRF_ERR;
        }

        g_avix.SampleRate = wavheader->SampleRate;
        g_avix.Channels = wavheader->Channels;
        g_avix.AudioType = wavheader->FormatTag;
        buf += listheader->BlockSize + 8;
        listheader = (LIST_HEADER *)(buf);

        if (listheader->ListID != AVI_LIST_ID)
        {
            return AVI_LIST_ERR;
        }

        if (listheader->ListType != AVI_STRL_ID)
        {
            return AVI_STRL_ERR;
        }

        strhheader = (STRH_HEADER *)(buf + 12);

        if (strhheader->BlockID != AVI_STRH_ID)
        {
            return AVI_STRH_ERR;
        }

        if (strhheader->StreamType != AVI_VIDS_STREAM)
        {
            return AVI_FORMAT_ERR;
        }

        bmpheader = (STRF_BMPHEADER *)(buf + 12 + strhheader->BlockSize + 8);

        if (bmpheader->BlockID != AVI_STRF_ID)
        {
            return AVI_STRF_ERR;
        }

        if (bmpheader->bmiHeader.Compression != AVI_FORMAT_MJPG)
        {
            return AVI_FORMAT_ERR;
        }

        g_avix.Width = (uint32_t)bmpheader->bmiHeader.Width;
        g_avix.Height = (uint32_t)bmpheader->bmiHeader.Height;
    }
    else
    {
        return AVI_FORMAT_ERR;
    }

    offset = (uint16_t)avi_srarch_id(tbuf, size, "movi");

    if (offset == 0)
    {
        return AVI_MOVI_ERR;
    }

    if (g_avix.SampleRate != 0U)        /* has audio */
    {
        tbuf += offset;
        offset = (uint16_t)avi_srarch_id(tbuf, size, g_avix.AudioFLAG);

        if (offset == 0)
        {
            return AVI_STREAM_ERR;
        }

        tbuf += offset + 4;
        g_avix.AudioBufSize = *((uint16_t *)tbuf);
    }

    printf("avi=%ux%u %ufps aud=%uHz ch=%u\r\n",
           (unsigned int)g_avix.Width, (unsigned int)g_avix.Height,
           (unsigned int)(1000000U / g_avix.SecPerFrame),
           (unsigned int)g_avix.SampleRate, (unsigned int)g_avix.Channels);

    return res;
}

uint32_t avi_srarch_id(uint8_t *buf, uint32_t size, char *id)
{
    uint32_t i;
    uint32_t idsize = 0;

    size -= 4;

    for (i = 0; i < size; i++)
    {
        if ((buf[i] == (uint8_t)id[0]) &&
            (buf[i + 1] == (uint8_t)id[1]) &&
            (buf[i + 2] == (uint8_t)id[2]) &&
            (buf[i + 3] == (uint8_t)id[3]))
        {
            idsize = MAKEDWORD(buf + i + 4);

            if (idsize > 0x10U)
            {
                return i;
            }
        }
    }

    return 0;
}

AVISTATUS avi_get_streaminfo(uint8_t *buf)
{
    g_avix.StreamID = MAKEWORD(buf + 2);
    g_avix.StreamSize = MAKEDWORD(buf + 4);

    if (g_avix.StreamSize > AVI_MAX_FRAME_SIZE)
    {
        printf("frame size over:%u\r\n", (unsigned int)g_avix.StreamSize);
        g_avix.StreamSize = 0;
        return AVI_STREAM_ERR;
    }

    if (g_avix.StreamSize % 2U)
    {
        g_avix.StreamSize++;        /* chunks are word aligned */
    }

    if ((g_avix.StreamID == AVI_VIDS_FLAG) || (g_avix.StreamID == AVI_AUDS_FLAG))
    {
        return AVI_OK;
    }

    return AVI_STREAM_ERR;
}
