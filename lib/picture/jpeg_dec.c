/**
 * @file    jpeg_dec.c
 * @brief   JPEG decode / encode helpers built on the LibJPEG module. The file
 *          I/O goes through FatFs; drawing goes through the piclib primitives.
 */

#include <string.h>
#include <stdint.h>
#include "jpeglib.h"
#include "jerror.h"
#include "jpeg_dec.h"
#include "piclib.h"
#include "ff.h"

#define JPEG_IO_BUF_SIZE 4096U

/* ------------------------------------------------------------------------- */
/* FatFs-backed decompression source manager                                  */
/* ------------------------------------------------------------------------- */

typedef struct
{
    struct jpeg_source_mgr pub;
    FIL     *file;
    uint8_t  buffer[JPEG_IO_BUF_SIZE];
    boolean  start_of_file;
} jpeg_fs_src_t;

static void jpeg_src_init(j_decompress_ptr cinfo)
{
    jpeg_fs_src_t *src = (jpeg_fs_src_t *)cinfo->src;

    src->start_of_file = TRUE;
}

static boolean jpeg_src_fill(j_decompress_ptr cinfo)
{
    jpeg_fs_src_t *src = (jpeg_fs_src_t *)cinfo->src;
    UINT n = 0U;

    if (f_read(src->file, src->buffer, JPEG_IO_BUF_SIZE, &n) != FR_OK)
    {
        ERREXIT(cinfo, JERR_INPUT_EOF);
    }

    if (n == 0U)
    {
        src->buffer[0] = 0xFFU;
        src->buffer[1] = (uint8_t)JPEG_EOI;
        n = 2U;
    }

    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = n;
    src->start_of_file = FALSE;

    return TRUE;
}

static void jpeg_src_skip(j_decompress_ptr cinfo, long num_bytes)
{
    jpeg_fs_src_t *src = (jpeg_fs_src_t *)cinfo->src;

    while (num_bytes > 0)
    {
        if ((size_t)num_bytes <= src->pub.bytes_in_buffer)
        {
            src->pub.bytes_in_buffer -= (size_t)num_bytes;
            src->pub.next_input_byte += num_bytes;
            num_bytes = 0;
        }
        else
        {
            num_bytes -= (long)src->pub.bytes_in_buffer;
            src->pub.bytes_in_buffer = 0U;
            (void)jpeg_src_fill(cinfo);
        }
    }
}

static void jpeg_src_term(j_decompress_ptr cinfo)
{
    (void)cinfo;
}

static void jpeg_src_attach(j_decompress_ptr cinfo, jpeg_fs_src_t *src, FIL *file)
{
    src->file = file;
    src->pub.init_source       = jpeg_src_init;
    src->pub.fill_input_buffer = jpeg_src_fill;
    src->pub.skip_input_data   = jpeg_src_skip;
    src->pub.resync_to_restart = jpeg_resync_to_restart;
    src->pub.term_source       = jpeg_src_term;
    src->pub.bytes_in_buffer   = 0U;
    src->pub.next_input_byte   = NULL;
    cinfo->src = (struct jpeg_source_mgr *)src;
}

/* ------------------------------------------------------------------------- */
/* FatFs-backed compression destination manager                               */
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
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    jpeg_fs_src_t src;
    FIL file;
    uint8_t *rowbuf;
    uint32_t src_w, src_h, dest_w, dest_h;
    uint32_t xoff, yoff;
    uint32_t sy, dy, dx;
    float scale;
    uint8_t res = 0U;

    (void)fast;

    if (f_open(&file, (const TCHAR *)filename, FA_READ) != FR_OK)
    {
        return PIC_FORMAT_ERR;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_src_attach(&cinfo, &src, &file);

    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK)
    {
        jpeg_destroy_decompress(&cinfo);
        (void)f_close(&file);
        return PIC_FORMAT_ERR;
    }

    cinfo.out_color_space = JCS_RGB;
    (void)jpeg_start_decompress(&cinfo);

    src_w = cinfo.output_width;
    src_h = cinfo.output_height;

    if ((src_w == 0U) || (src_h == 0U) || (picinfo.S_Width == 0U) || (picinfo.S_Height == 0U))
    {
        jpeg_destroy_decompress(&cinfo);
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

    rowbuf = (uint8_t *)piclib_mem_malloc(src_w * 3U);

    if (rowbuf == NULL)
    {
        jpeg_destroy_decompress(&cinfo);
        (void)f_close(&file);
        return PIC_MEM_ERR;
    }

    for (sy = 0U; sy < src_h; sy++)
    {
        JSAMPROW rowptr[1];
        uint32_t r0;
        uint32_t r1;

        rowptr[0] = (JSAMPROW)rowbuf;
        (void)jpeg_read_scanlines(&cinfo, rowptr, 1U);

        r0 = (uint32_t)((float)sy * (float)dest_h / (float)src_h);
        r1 = (uint32_t)((float)(sy + 1U) * (float)dest_h / (float)src_h);

        if (r1 <= r0)
        {
            r1 = r0 + 1U;
        }

        for (dy = r0; (dy < r1) && (dy < dest_h); dy++)
        {
            for (dx = 0U; dx < dest_w; dx++)
            {
                uint32_t sx = (uint32_t)((float)dx * (float)src_w / (float)dest_w);
                uint8_t *p = &rowbuf[sx * 3U];
                uint16_t color = (uint16_t)(((uint16_t)(p[0] >> 3) << 11) |
                                            ((uint16_t)(p[1] >> 2) << 5) |
                                            (uint16_t)(p[2] >> 3));

                pic_phy.draw_point((uint16_t)(xoff + dx), (uint16_t)(yoff + dy), color);
            }
        }
    }

    piclib_mem_free(rowbuf);

    (void)jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    (void)f_close(&file);

    return res;
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
