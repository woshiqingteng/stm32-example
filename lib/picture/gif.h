/**
 * @file    gif.h
 * @brief   GIF decoder (ALIENTEK PICTURE middleware).
 */

#ifndef LIB_PICTURE_GIF_H
#define LIB_PICTURE_GIF_H

#include <stdint.h>
#include "ff.h"

#define GIF_USE_MALLOC          1

#define LCD_MAX_LOG_COLORS      256
#define MAX_NUM_LWZ_BITS        12

#define GIF_INTRO_TERMINATOR    ';'
#define GIF_INTRO_EXTENSION     '!'
#define GIF_INTRO_IMAGE         ','

#define GIF_COMMENT             0xFE
#define GIF_APPLICATION         0xFF
#define GIF_PLAINTEXT           0x01
#define GIF_GRAPHICCTL          0xF9

/** @brief  LZW decoder state. */
typedef struct
{
    uint8_t  aBuffer[258];
    short    aCode[(1 << MAX_NUM_LWZ_BITS)];
    uint8_t  aPrefix[(1 << MAX_NUM_LWZ_BITS)];
    uint8_t  aDecompBuffer[3000];
    uint8_t *sp;
    int      CurBit;
    int      LastBit;
    int      GetDone;
    int      LastByte;
    int      ReturnClear;
    int      CodeSize;
    int      SetCodeSize;
    int      MaxCode;
    int      MaxCodeSize;
    int      ClearCode;
    int      EndCode;
    int      FirstCode;
    int      OldCode;
} LZW_INFO;

/** @brief  Logical screen descriptor. */
typedef struct __attribute__((packed))
{
    uint16_t width;
    uint16_t height;
    uint8_t  flag;
    uint8_t  bkcindex;
    uint8_t  pixratio;
} LogicalScreenDescriptor;

/** @brief  Image screen descriptor. */
typedef struct __attribute__((packed))
{
    uint16_t xoff;
    uint16_t yoff;
    uint16_t width;
    uint16_t height;
    uint8_t  flag;
} ImageScreenDescriptor;

/** @brief  GIF state. */
typedef struct
{
    LogicalScreenDescriptor gifLSD;
    ImageScreenDescriptor   gifISD;
    uint16_t                colortbl[256];
    uint16_t                bkpcolortbl[256];
    uint16_t                numcolors;
    uint16_t                delay;
    LZW_INFO               *lzw;
} gif89a;

extern uint8_t g_gif_decoding;

void    gif_quit(void);
uint8_t gif_getinfo(FIL *file, gif89a *gif);
uint8_t gif_decode(const char *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height);

#endif /* LIB_PICTURE_GIF_H */
