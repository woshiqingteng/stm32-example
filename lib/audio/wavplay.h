/**
 * @file    wavplay.h
 * @brief   WAV playback (16-bit / 24-bit PCM) over the ES8388 + SAI path.
 */

#ifndef LIB_AUDIO_WAVPLAY_H
#define LIB_AUDIO_WAVPLAY_H

#include <stdint.h>

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

/** @brief  Parse the WAV header and fill the control block. @return 0 on success. */
uint8_t wav_decode_init(char *fname, __wavctrl *wavx);

/** @brief  Read up to size bytes of PCM into buf (24-bit is expanded to 32). */
uint32_t wav_buffill(uint8_t *buf, uint16_t size, uint8_t bits);

/** @brief  Play one WAV file. @return KEY0 (next), KEY2 (previous) or other. */
uint8_t wav_play_song(char *fname);

#endif /* LIB_AUDIO_WAVPLAY_H */
