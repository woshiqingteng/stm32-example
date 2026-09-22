/**
 * @file    avi.h
 * @brief   AVI (Motion JPEG + PCM/WAV) container parser.
 */

#ifndef LIB_MJPEG_AVI_H
#define LIB_MJPEG_AVI_H

#include <stdint.h>

/* Maximum frame size accepted; bounds the video read buffer. */
#define AVI_MAX_FRAME_SIZE   (60 * 1024)

/** @brief  AVI parse result. */
typedef enum
{
    AVI_OK = 0,
    AVI_RIFF_ERR,
    AVI_AVI_ERR,
    AVI_LIST_ERR,
    AVI_HDRL_ERR,
    AVI_AVIH_ERR,
    AVI_STRL_ERR,
    AVI_STRH_ERR,
    AVI_STRF_ERR,
    AVI_MOVI_ERR,
    AVI_FORMAT_ERR,
    AVI_STREAM_ERR
} AVISTATUS;

#define AVI_RIFF_ID         0x46464952U
#define AVI_AVI_ID          0x20495641U
#define AVI_LIST_ID         0x5453494CU
#define AVI_HDRL_ID         0x6C726468U      /* "hdrl" */
#define AVI_MOVI_ID         0x69766F6DU      /* "movi" */
#define AVI_STRL_ID         0x6C727473U      /* "strl" */

#define AVI_AVIH_ID         0x68697661U      /* "avih" */
#define AVI_STRH_ID         0x68727473U      /* "strh" */
#define AVI_STRF_ID         0x66727473U      /* "strf" */

#define AVI_VIDS_STREAM     0x73646976U      /* "vids" */
#define AVI_AUDS_STREAM     0x73647561U      /* "auds" */

#define AVI_VIDS_FLAG       0x6463U          /* "dc" */
#define AVI_AUDS_FLAG       0x7762U          /* "wb" */

#define AVI_FORMAT_MJPG     0x47504A4DU      /* "MJPG" */

/** @brief  Parsed AVI stream information. */
typedef struct __attribute__((packed))
{
    uint32_t SecPerFrame;       /* frame period (us) */
    uint32_t TotalFrame;        /* total frame count */
    uint32_t Width;
    uint32_t Height;
    uint32_t SampleRate;
    uint16_t Channels;
    uint16_t AudioBufSize;
    uint16_t AudioType;         /* 0x0001 PCM, 0x0050 MP2, 0x0055 MP3 */
    uint16_t StreamID;          /* current stream id: "dc" / "wb" */
    uint32_t StreamSize;        /* current chunk size */
    char    *VideoFLAG;         /* e.g. "00dc" */
    char    *AudioFLAG;         /* e.g. "01wb" */
} AVI_INFO;

extern AVI_INFO g_avix;

typedef struct
{
    uint32_t RiffID;
    uint32_t FileSize;
    uint32_t AviID;
} AVI_HEADER;

typedef struct
{
    uint32_t FrameID;
    uint32_t FrameSize;
} FRAME_HEADER;

typedef struct
{
    uint32_t ListID;
    uint32_t BlockSize;
    uint32_t ListType;
} LIST_HEADER;

typedef struct
{
    uint32_t BlockID;
    uint32_t BlockSize;
    uint32_t SecPerFrame;
    uint32_t MaxByteSec;
    uint32_t PaddingGranularity;
    uint32_t Flags;
    uint32_t TotalFrame;
    uint32_t InitFrames;
    uint32_t Streams;
    uint32_t RefBufSize;
    uint32_t Width;
    uint32_t Height;
    uint32_t Reserved[4];
} AVIH_HEADER;

typedef struct
{
    uint32_t BlockID;
    uint32_t BlockSize;
    uint32_t StreamType;
    uint32_t Handler;
    uint32_t Flags;
    uint16_t Priority;
    uint16_t Language;
    uint32_t InitFrames;
    uint32_t Scale;
    uint32_t Rate;
    uint32_t Start;
    uint32_t Length;
    uint32_t RefBufSize;
    uint32_t Quality;
    uint32_t SampleSize;
    struct
    {
        short Left;
        short Top;
        short Right;
        short Bottom;
    } Frame;
} STRH_HEADER;

typedef struct
{
    uint32_t BmpSize;
    long     Width;
    long     Height;
    uint16_t Planes;
    uint16_t BitCount;
    uint32_t Compression;
    uint32_t SizeImage;
    long     XpixPerMeter;
    long     YpixPerMeter;
    uint32_t ClrUsed;
    uint32_t ClrImportant;
} BMP_HEADER;

typedef struct
{
    uint8_t rgbBlue;
    uint8_t rgbGreen;
    uint8_t rgbRed;
    uint8_t rgbReserved;
} AVIRGBQUAD;

typedef struct
{
    uint32_t BlockID;
    uint32_t BlockSize;
    BMP_HEADER bmiHeader;
    AVIRGBQUAD bmColors[1];
} STRF_BMPHEADER;

typedef struct
{
    uint32_t BlockID;
    uint32_t BlockSize;
    uint16_t FormatTag;
    uint16_t Channels;
    uint32_t SampleRate;
    uint32_t BaudRate;
    uint16_t BlockAlign;
    uint16_t Size;
} STRF_WAVHEADER;

#define MAKEWORD(ptr)   (uint16_t)(((uint16_t)*((uint8_t *)(ptr)) << 8) | (uint16_t)*(uint8_t *)((ptr) + 1))
#define MAKEDWORD(ptr)  (uint32_t)(((uint16_t)*(uint8_t *)(ptr) | (((uint16_t)*(uint8_t *)((ptr) + 1)) << 8) | \
                                   (((uint16_t)*(uint8_t *)((ptr) + 2)) << 16) | (((uint16_t)*(uint8_t *)((ptr) + 3)) << 24)))

/** @brief  Parse the AVI header. */
AVISTATUS avi_init(uint8_t *buf, uint32_t size);

/** @brief  Find a 4-character chunk id; returns its offset or 0. */
uint32_t avi_srarch_id(uint8_t *buf, uint32_t size, char *id);

/** @brief  Parse the next stream chunk header. */
AVISTATUS avi_get_streaminfo(uint8_t *buf);

#endif /* LIB_MJPEG_AVI_H */
