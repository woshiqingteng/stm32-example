/**
 * @file    bmp.h
 * @brief   BMP decoder / encoder (ALIENTEK PICTURE middleware).
 */

#ifndef LIB_PICTURE_BMP_H
#define LIB_PICTURE_BMP_H

#include <stdint.h>

#define BMP_USE_MALLOC      1           /* use the middleware allocator */
#define BMP_DBUF_SIZE       2048        /* BMP data buffer size */

/** @brief  BITMAPINFOHEADER. */
typedef struct __attribute__((packed))
{
    uint32_t biSize;
    long     biWidth;
    long     biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    long     biXPelsPerMeter;
    long     biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;

/** @brief  BITMAPFILEHEADER. */
typedef struct __attribute__((packed))
{
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

/** @brief  RGB palette entry. */
typedef struct __attribute__((packed))
{
    uint8_t rgbBlue;
    uint8_t rgbGreen;
    uint8_t rgbRed;
    uint8_t rgbReserved;
} RGBQUAD;

/** @brief  BMP header bundle. */
typedef struct __attribute__((packed))
{
    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bmiHeader;
    uint32_t         RGB_MASK[3];
} BITMAPINFO;

typedef RGBQUAD *LPRGBQUAD;

#define BI_RGB          0
#define BI_RLE8         1
#define BI_RLE4         2
#define BI_BITFIELDS    3

uint8_t stdbmp_decode(const char *filename);
uint8_t minibmp_decode(uint8_t *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t acolor, uint8_t mode);
uint8_t bmp_encode(uint8_t *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t mode);

#endif /* LIB_PICTURE_BMP_H */
