/**
 * @file    mjpeg.c
 * @brief   Motion JPEG frame decoder.
 *
 * The frame lives in a caller supplied memory buffer (a chunk of the AVI
 * "movi" stream). LibJPEG decompresses it scanline by scanline; each RGB
 * scanline is packed to RGB565 and pushed to the panel with lcd_color_fill().
 */

#include <stdio.h>
#include <setjmp.h>
#include "jpeglib.h"
#include "jerror.h"
#include "lcd.h"
#include "malloc.h"
#include "mjpeg.h"

/* LibJPEG calls exit() from its default error handler; route it through a
 * longjmp instead so a bad frame does not kill the player. */
struct my_error_mgr
{
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

static struct jpeg_decompress_struct s_cinfo;
static struct my_error_mgr            s_jerr;
static uint16_t                      *p_linebuf;    /* one RGB565 scanline */
static uint16_t                       g_imgoffx;
static uint16_t                       g_imgoffy;

/* Memory backed LibJPEG source manager. */
typedef struct
{
    struct jpeg_source_mgr pub;
    uint8_t *buf;
    uint32_t size;
} mem_src_t;

static mem_src_t s_src;
static uint8_t    s_eoi[2];

static void my_error_exit(j_common_ptr cinfo)
{
    struct my_error_mgr *myerr = (struct my_error_mgr *)cinfo->err;

    longjmp(myerr->setjmp_buffer, 1);
}

static void mem_src_init(j_decompress_ptr cinfo)
{
    (void)cinfo;
}

static boolean mem_src_fill(j_decompress_ptr cinfo)
{
    mem_src_t *src = (mem_src_t *)cinfo->src;

    if (src->size == 0U)                /* input exhausted: inject EOI */
    {
        s_eoi[0] = 0xFF;
        s_eoi[1] = (uint8_t)JPEG_EOI;
        src->pub.next_input_byte = s_eoi;
        src->pub.bytes_in_buffer = 2;
    }
    else
    {
        src->pub.next_input_byte = src->buf;
        src->pub.bytes_in_buffer = src->size;
        src->size = 0;
    }

    return TRUE;
}

static void mem_src_skip(j_decompress_ptr cinfo, long num_bytes)
{
    mem_src_t *src = (mem_src_t *)cinfo->src;

    if (num_bytes > 0)
    {
        while (num_bytes > (long)src->pub.bytes_in_buffer)
        {
            num_bytes -= (long)src->pub.bytes_in_buffer;
            (void)src->pub.fill_input_buffer(cinfo);
        }

        src->pub.next_input_byte += (size_t)num_bytes;
        src->pub.bytes_in_buffer -= (size_t)num_bytes;
    }
}

static void mem_src_term(j_decompress_ptr cinfo)
{
    (void)cinfo;
}

static void mem_src_attach(j_decompress_ptr cinfo, uint8_t *buf, uint32_t size)
{
    s_src.buf = buf;
    s_src.size = size;
    s_src.pub.init_source = mem_src_init;
    s_src.pub.fill_input_buffer = mem_src_fill;
    s_src.pub.skip_input_data = mem_src_skip;
    s_src.pub.resync_to_restart = jpeg_resync_to_restart;
    s_src.pub.term_source = mem_src_term;
    s_src.pub.bytes_in_buffer = 0;
    s_src.pub.next_input_byte = NULL;
    cinfo->src = &s_src.pub;
}

uint8_t mjpegdec_init(uint16_t offx, uint16_t offy)
{
    p_linebuf = mymalloc(SRAMIN, lcd_get_width() * 2U);

    if (p_linebuf == NULL)
    {
        return 1;
    }

    g_imgoffx = offx;
    g_imgoffy = offy;

    return 0;
}

void mjpegdec_free(void)
{
    myfree(SRAMIN, p_linebuf);
    p_linebuf = NULL;
}

uint8_t mjpegdec_decode(uint8_t *buf, uint32_t bsize)
{
    uint8_t  *rowbuf;
    uint32_t  row;
    uint32_t  x;
    uint32_t  w;
    uint32_t  h;

    if ((bsize == 0U) || (p_linebuf == NULL))
    {
        return 1;
    }

    s_cinfo.err = jpeg_std_error(&s_jerr.pub);
    s_jerr.pub.error_exit = my_error_exit;

    if (setjmp(s_jerr.setjmp_buffer))       /* decoder error */
    {
        jpeg_abort_decompress(&s_cinfo);
        jpeg_destroy_decompress(&s_cinfo);
        return 2;
    }

    jpeg_create_decompress(&s_cinfo);
    mem_src_attach(&s_cinfo, buf, bsize);
    (void)jpeg_read_header(&s_cinfo, TRUE);

    s_cinfo.dct_method = JDCT_IFAST;
    s_cinfo.do_fancy_upsampling = 0;
    s_cinfo.out_color_space = JCS_RGB;
    (void)jpeg_start_decompress(&s_cinfo);

    w = s_cinfo.output_width;
    h = s_cinfo.output_height;

    if ((uint32_t)g_imgoffx + w > lcd_get_width())
    {
        w = (uint32_t)lcd_get_width() - g_imgoffx;
    }

    if ((uint32_t)g_imgoffy + h > lcd_get_height())
    {
        h = (uint32_t)lcd_get_height() - g_imgoffy;
    }

    rowbuf = mymalloc(SRAMIN, s_cinfo.output_width * 3U);

    if (rowbuf == NULL)
    {
        jpeg_finish_decompress(&s_cinfo);
        jpeg_destroy_decompress(&s_cinfo);
        return 3;
    }

    for (row = 0; row < h; row++)
    {
        JSAMPROW rowptr[1];

        rowptr[0] = (JSAMPROW)rowbuf;
        (void)jpeg_read_scanlines(&s_cinfo, rowptr, 1U);

        for (x = 0; x < w; x++)
        {
            uint8_t *p = &rowbuf[x * 3U];

            p_linebuf[x] = (uint16_t)(((uint16_t)(p[0] >> 3) << 11) |
                                      ((uint16_t)(p[1] >> 2) << 5) |
                                      (uint16_t)(p[2] >> 3));
        }

        lcd_color_fill(g_imgoffx, (uint16_t)(g_imgoffy + row),
                       (uint16_t)(g_imgoffx + w - 1U), (uint16_t)(g_imgoffy + row),
                       p_linebuf);
    }

    myfree(SRAMIN, rowbuf);

    (void)jpeg_finish_decompress(&s_cinfo);
    jpeg_destroy_decompress(&s_cinfo);

    return 0;
}
