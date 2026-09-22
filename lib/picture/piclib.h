/**
 * @file    piclib.h
 * @brief   Image viewer middleware (ALIENTEK PICTURE), ported to the RGB panel.
 *          BMP and GIF decoders are bundled; JPEG decoding goes through the
 *          LibJPEG module.
 */

#ifndef LIB_PICTURE_PICLIB_H
#define LIB_PICTURE_PICLIB_H

#include <stdint.h>
#include "bmp.h"
#include "gif.h"
#include "jpeg_dec.h"

#define PIC_FORMAT_ERR      0x27    /* unsupported image format */
#define PIC_SIZE_ERR        0x28    /* image too large */
#define PIC_WINDOW_ERR      0x29    /* window out of range */
#define PIC_MEM_ERR         0x11    /* out of memory */

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

/** @brief  Drawing primitives the decoders render through. */
typedef struct
{
    uint32_t (*read_point)(uint16_t, uint16_t);
    void     (*draw_point)(uint16_t, uint16_t, uint32_t);
    void     (*fill)(uint16_t, uint16_t, uint16_t, uint16_t, uint32_t);
    void     (*draw_hline)(uint16_t, uint16_t, uint16_t, uint16_t);
    void     (*fillcolor)(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t *);
} _pic_phy;

extern _pic_phy pic_phy;

/** @brief  Current image and window geometry. */
typedef struct
{
    uint16_t lcdwidth;
    uint16_t lcdheight;
    uint32_t ImgWidth;
    uint32_t ImgHeight;
    uint32_t Div_Fac;       /* scaling factor, times 8192 */
    uint32_t S_Height;      /* requested window */
    uint32_t S_Width;
    uint32_t S_XOFF;        /* window offset */
    uint32_t S_YOFF;
    uint32_t staticx;       /* last drawn point, for the fast path */
    uint32_t staticy;
} _pic_info;

extern _pic_info picinfo;

void    *piclib_mem_malloc(uint32_t size);
void     piclib_mem_free(void *paddr);

void     piclib_init(void);
void     piclib_ai_draw_init(void);
uint16_t piclib_alpha_blend(uint16_t src, uint16_t dst, uint8_t alpha);
uint8_t  piclib_is_element_ok(uint16_t x, uint16_t y, uint8_t chg);
uint8_t  piclib_ai_load_picfile(char *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t fast);

#endif /* LIB_PICTURE_PICLIB_H */
