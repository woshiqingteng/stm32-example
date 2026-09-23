/**
 * @file    wavplay.h
 * @brief   WAV playback (16-bit / 24-bit PCM) over the ES8388 + SAI path.
 */

#ifndef LIB_AUDIO_WAVPLAY_H
#define LIB_AUDIO_WAVPLAY_H

#include <stdint.h>

/** @brief  Playback DMA half-buffer size (bytes). 8192 avoids drop-outs at
 *  192 kbps / 24-bit. */
#define AUDIO_SAI_TX_BUF_SIZE 8192U

/** @brief  Player navigation result codes (kept out of the FRESULT range). */
typedef enum
{
    AUDIO_STOP  = 0,      /*!< stop / finished       */
    AUDIO_NEXT  = 1,      /*!< KEY0: next track      */
    AUDIO_PREV  = 2,      /*!< KEY2: previous track  */
    AUDIO_ERROR = 0xFF,   /*!< playback error        */
} audio_nav_t;

/** @brief  Configure the codec for DAC playback and set a default volume. */
void audio_hw_init(void);

/** @brief  WAV header decode result codes. */
typedef enum
{
    WAV_OK         = 0,   /*!< parsed successfully */
    WAV_ERR_OPEN   = 1,   /*!< file could not be opened */
    WAV_ERR_FORMAT = 2,   /*!< not a WAVE file */
    WAV_ERR_DATA   = 3,   /*!< data chunk not found */
} wav_status_t;

typedef struct __attribute__((packed))
{
    uint32_t ChunkID;           /* "RIFF" */
    uint32_t ChunkSize;         /* file size - 8 */
    uint32_t Format;            /* "WAVE" */
} ChunkRIFF;

typedef struct __attribute__((packed))
{
    uint32_t ChunkID;           /* "fmt " */
    uint32_t ChunkSize;
    uint16_t AudioFormat;       /* 0x01 = PCM */
    uint16_t NumOfChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
} ChunkFMT;

typedef struct __attribute__((packed))
{
    uint32_t ChunkID;           /* "fact" */
    uint32_t ChunkSize;
    uint32_t NumOfSamples;
} ChunkFACT;

typedef struct __attribute__((packed))
{
    uint32_t ChunkID;           /* "LIST" */
    uint32_t ChunkSize;
} ChunkLIST;

typedef struct __attribute__((packed))
{
    uint32_t ChunkID;           /* "data" */
    uint32_t ChunkSize;
} ChunkDATA;

typedef struct __attribute__((packed))
{
    ChunkRIFF riff;
    ChunkFMT  fmt;
    ChunkDATA data;
} __WaveHeader;

typedef struct __attribute__((packed))
{
    uint16_t audioformat;
    uint16_t nchannels;
    uint16_t blockalign;
    uint32_t datasize;

    uint32_t totsec;
    uint32_t cursec;

    uint32_t bitrate;
    uint32_t samplerate;
    uint16_t bps;
    uint32_t datastart;
} __wavctrl;

/** @brief  Parse the WAV header and fill the control block. @return WAV_OK on success. */
wav_status_t wav_decode_init(char *fname, __wavctrl *wavx);

/** @brief  Read up to size bytes of PCM into buf (24-bit is expanded to 32). */
uint32_t wav_buffill(uint8_t *buf, uint16_t size, uint8_t bits);

/** @brief  Play one WAV file. @return AUDIO_* navigation code. */
audio_nav_t wav_play_song(char *fname);

#endif /* LIB_AUDIO_WAVPLAY_H */
