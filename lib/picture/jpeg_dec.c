/**
 * @file    jpeg_dec.c
 * @brief   JPEG decode / encode helpers. Decoding uses the TJpgDec module,
 *          encoding uses LibJPEG. File I/O goes through FatFs; drawing goes
 *          through the piclib primitives.
 */

#include <string.h>
#include <stdint.h>

#include "jpeglib.h"
#include "jerror.h"
#include "tjpgd.h"
#include "jpeg_dec.h"
#include "piclib.h"
#include "ff.h"

#define JPEG_IO_BUF_SIZE 4096U

/* Working pool for a TJpgDec session: input buffer (512) + quantizer/huffman
 * tables + IDCT/MCU buffers comfortably fit in 8 KB. */
#define JPEG_TJPGD_POOL_SIZE 8192U

/* ------------------------------------------------------------------------- */
/* TJpgDec session: FatFs input and piclib output                             */
/* ------------------------------------------------------------------------- */

typedef struct
{
    uint16_t src_w;
    uint16_t src_h;
    uint16_t dest_w;
    uint16_t dest_h;
    uint16_t xoff;
    uint16_t yoff;
    uint8_t  fast;
} jpeg_out_ctx_t;

static jpeg_out_ctx_t s_out;

static size_t jpeg_in_func(JDEC *jd, uint8_t *buff, size_t nbyte)
{
    FIL *file = (FIL *)jd->device;

    if (buff != NULL)
    {
        UINT br = 0U;

        if (f_read(file, buff, (UINT)nbyte, &br) != FR_OK)
        {
            return 0U;
        }
        return (size_t)br;
    }

    if (f_lseek(file, f_tell(file) + (FSIZE_t)nbyte) != FR_OK)
    {
        return 0U;
    }
    return nbyte;
}

static int jpeg_out_func(JDEC *jd, void *bitmap, JRECT *rect)
{
    uint16_t *src = (uint16_t *)bitmap;

    (void)jd;

    if (s_out.fast != 0U)
    {
        /* Fast mode: fill each MCU's destination rectangle with one colour
         * through the DMA2D colour fill (blocky but far faster). */
        uint32_t x0 = (picinfo.Div_Fac * (uint32_t)rect->left) >> 13;
        uint32_t y0 = (picinfo.Div_Fac * (uint32_t)rect->top) >> 13;
        uint32_t x1 = (picinfo.Div_Fac * (uint32_t)rect->right) >> 13;
        uint32_t y1 = (picinfo.Div_Fac * (uint32_t)rect->bottom) >> 13;
        uint16_t color = src[0];

        if ((x0 < s_out.dest_w) && (y0 < s_out.dest_h))
        {
            if (x1 >= s_out.dest_w) { x1 = (uint32_t)(s_out.dest_w - 1U); }
            if (y1 >= s_out.dest_h) { y1 = (uint32_t)(s_out.dest_h - 1U); }

            pic_phy.fillcolor((uint16_t)(s_out.xoff + x0), (uint16_t)(s_out.yoff + y0),
                              (uint16_t)(x1 - x0 + 1U), (uint16_t)(y1 - y0 + 1U), &color);
        }

        return 1;
    }

    for (uint16_t y = rect->top; y <= rect->bottom; y++)
    {
        uint32_t dy = (picinfo.Div_Fac * (uint32_t)y) >> 13;
        uint16_t x;

        for (x = rect->left; x <= rect->right; x++)
        {
            uint32_t dx = (picinfo.Div_Fac * (uint32_t)x) >> 13;
            uint16_t color = *src++;

            if ((dx < s_out.dest_w) && (dy < s_out.dest_h))
            {
                pic_phy.draw_point((uint16_t)(s_out.xoff + dx),
                                   (uint16_t)(s_out.yoff + dy), color);
            }
        }
    }

    return 1;
}

/* ------------------------------------------------------------------------- */
/* FatFs-backed compression destination manager (LibJPEG encode)              */
/* ------------------------------------------------------------------------- */

typedef struct
{
    struct jpeg_destination_mgr pub;
    FIL     *file;
    uint8_t  buffer[JPEG_IO_BUF_SIZE];
} jpeg_fs_dst_t;

static void jpeg_dst_init(j_compress_ptr cinfo)
{
    jpeg_fs_dst_t *dst = (jpeg_fs_dst_t *)cinfo->dest;

    dst->pub.next_output_byte = dst->buffer;
    dst->pub.free_in_buffer   = JPEG_IO_BUF_SIZE;
}

static boolean jpeg_dst_empty(j_compress_ptr cinfo)
{
    jpeg_fs_dst_t *dst = (jpeg_fs_dst_t *)cinfo->dest;
    UINT bw = 0U;

    if (f_write(dst->file, dst->buffer, JPEG_IO_BUF_SIZE, &bw) != FR_OK)
    {
        ERREXIT(cinfo, JERR_FILE_WRITE);
    }

    dst->pub.next_output_byte = dst->buffer;
    dst->pub.free_in_buffer   = JPEG_IO_BUF_SIZE;

    return TRUE;
}

static void jpeg_dst_term(j_compress_ptr cinfo)
{
    jpeg_fs_dst_t *dst = (jpeg_fs_dst_t *)cinfo->dest;
    size_t n = JPEG_IO_BUF_SIZE - dst->pub.free_in_buffer;

    if (n > 0U)
    {
        UINT bw = 0U;
        (void)f_write(dst->file, dst->buffer, (UINT)n, &bw);
    }
}

static void jpeg_dst_attach(j_compress_ptr cinfo, jpeg_fs_dst_t *dst, FIL *file)
{
    dst->file = file;
    dst->pub.init_destination    = jpeg_dst_init;
    dst->pub.empty_output_buffer = jpeg_dst_empty;
    dst->pub.term_destination    = jpeg_dst_term;
    cinfo->dest = (struct jpeg_destination_mgr *)dst;
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                 */
/* ------------------------------------------------------------------------- */

uint8_t jpg_decode(const char *filename, uint8_t fast)
{
    FIL file;
    JDEC jd;
    void *pool;
    uint32_t src_w, src_h, dest_w, dest_h;
    uint32_t xoff, yoff;
    float scale;
    JRESULT jr;

    if (f_open(&file, (const TCHAR *)filename, FA_READ) != FR_OK)
    {
        return PIC_FORMAT_ERR;
    }

    pool = piclib_mem_malloc(JPEG_TJPGD_POOL_SIZE);
    if (pool == NULL)
    {
        (void)f_close(&file);
        return PIC_MEM_ERR;
    }

    jr = jd_prepare(&jd, jpeg_in_func, pool, JPEG_TJPGD_POOL_SIZE, &file);
    if (jr != JDR_OK)
    {
        piclib_mem_free(pool);
        (void)f_close(&file);
        return PIC_FORMAT_ERR;
    }

    src_w = jd.width;
    src_h = jd.height;

    if ((src_w == 0U) || (src_h == 0U) || (picinfo.S_Width == 0U) || (picinfo.S_Height == 0U))
    {
        piclib_mem_free(pool);
        (void)f_close(&file);
        return PIC_SIZE_ERR;
    }

    scale = (float)picinfo.S_Width / (float)src_w;
    if (((float)picinfo.S_Height / (float)src_h) < scale)
    {
        scale = (float)picinfo.S_Height / (float)src_h;
    }
    if (scale > 1.0f)
    {
        scale = 1.0f;
    }

    dest_w = (uint32_t)((float)src_w * scale);
    dest_h = (uint32_t)((float)src_h * scale);

    if (dest_w == 0U)
    {
        dest_w = 1U;
    }
    if (dest_h == 0U)
    {
        dest_h = 1U;
    }

    xoff = picinfo.S_XOFF + (picinfo.S_Width - dest_w) / 2U;
    yoff = picinfo.S_YOFF + (picinfo.S_Height - dest_h) / 2U;

    picinfo.ImgWidth  = src_w;
    picinfo.ImgHeight = src_h;
    picinfo.Div_Fac   = (uint32_t)(scale * 8192.0f);

    s_out.src_w  = (uint16_t)src_w;
    s_out.src_h  = (uint16_t)src_h;
    s_out.dest_w = (uint16_t)dest_w;
    s_out.dest_h = (uint16_t)dest_h;
    s_out.xoff   = (uint16_t)xoff;
    s_out.yoff   = (uint16_t)yoff;
    s_out.fast   = fast;

    jr = jd_decomp(&jd, jpeg_out_func, 0U);

    piclib_mem_free(pool);
    (void)f_close(&file);

    return (jr == JDR_OK) ? 0U : PIC_FORMAT_ERR;
}

uint8_t jpg_encode(const char *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    jpeg_fs_dst_t dst;
    FIL file;
    uint8_t *rowbuf;
    uint16_t col;
    uint8_t res = 0U;

    if ((width == 0U) || (height == 0U))
    {
        return PIC_WINDOW_ERR;
    }

    if (f_open(&file, (const TCHAR *)filename, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        return PIC_FORMAT_ERR;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_dst_attach(&cinfo, &dst, &file);

    cinfo.image_width      = width;
    cinfo.image_height     = height;
    cinfo.input_components = 3;
    cinfo.in_color_space   = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 85, TRUE);
    (void)jpeg_start_compress(&cinfo, TRUE);

    rowbuf = (uint8_t *)piclib_mem_malloc((uint32_t)width * 3U);

    if (rowbuf == NULL)
    {
        jpeg_destroy_compress(&cinfo);
        (void)f_close(&file);
        return PIC_MEM_ERR;
    }

    while (cinfo.next_scanline < cinfo.image_height)
    {
        JSAMPROW rowptr[1];
        uint16_t row = (uint16_t)cinfo.next_scanline;

        for (col = 0U; col < width; col++)
        {
            uint32_t c = pic_phy.read_point((uint16_t)(x + col), (uint16_t)(y + row));
            uint8_t r = (uint8_t)((c >> 11) & 0x1FU);
            uint8_t g = (uint8_t)((c >> 5) & 0x3FU);
            uint8_t b = (uint8_t)(c & 0x1FU);

            rowbuf[col * 3U + 0U] = (uint8_t)((r << 3) | (r >> 2));
            rowbuf[col * 3U + 1U] = (uint8_t)((g << 2) | (g >> 4));
            rowbuf[col * 3U + 2U] = (uint8_t)((b << 3) | (b >> 2));
        }

        rowptr[0] = (JSAMPROW)rowbuf;
        (void)jpeg_write_scanlines(&cinfo, rowptr, 1U);
    }

    piclib_mem_free(rowbuf);

    (void)jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    (void)f_close(&file);

    return res;
}
