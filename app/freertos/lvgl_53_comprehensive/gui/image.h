/**
 * @file    image.h
 * @brief   SPI-NOR image library (update / init).
 */

#ifndef IMAGE_H
#define IMAGE_H

#include "bsp.h"
#include "nor.h"

/* image info header, right after the font store (NOR_IMAGE_BASE) */
#define IMAGEINFOADDR NOR_IMAGE_BASE

/* Image library descriptor: imageok flag, then an (address, size) pair per
 * image (atk01..atk08). */
typedef struct __attribute__((packed))
{
    uint8_t imageok;             /* 0xAA when the library is valid */

    uint32_t lvgl_atk01addr;
    uint32_t lvgl_atk01size;

    uint32_t lvgl_atk02addr;
    uint32_t lvgl_atk02size;

    uint32_t lvgl_atk03addr;
    uint32_t lvgl_atk03size;

    uint32_t lvgl_atk04addr;
    uint32_t lvgl_atk04size;

    uint32_t lvgl_atk05addr;
    uint32_t lvgl_atk05size;

    uint32_t lvgl_atk06addr;
    uint32_t lvgl_atk06size;

    uint32_t lvgl_atk07addr;
    uint32_t lvgl_atk07size;

    uint32_t lvgl_atk08addr;
    uint32_t lvgl_atk08size;
} _image_info;

extern _image_info g_ftinfo;

uint8_t images_update_image(uint16_t x, uint16_t y, uint8_t size, uint8_t *src, uint16_t color);
uint8_t images_init(void);

#endif
