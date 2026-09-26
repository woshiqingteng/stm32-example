/**
 * @file    cam_jpeg.c
 * @brief   One-shot JPEG frame capture over the DCMI. A double-buffered line
 *          DMA accumulates the JPEG stream into the caller buffer; the frame
 *          (VSYNC) callback marks the frame complete.
 */

#include "stm32f4xx_hal.h"
#include "dcmi.h"
#include "sys.h"
#include "cam_jpeg.h"

/* Per-line DMA staging buffer (32-bit words). */
#define CAM_JPEG_LINE_WORDS  512U

static uint32_t          s_line[2][CAM_JPEG_LINE_WORDS];
static uint32_t         *s_dst;
static uint32_t          s_max_words;
static volatile uint32_t s_len;
static volatile cam_jpeg_state_t s_state = CAM_JPEG_IDLE;

static const uint32_t *cam_jpeg_active_line(void)
{
    /* The buffer not currently being filled by the DMA. */
    return ((g_dma_dcmi_handle.Instance->CR & DMA_SxCR_CT) != 0U) ? s_line[0] : s_line[1];
}

static void cam_jpeg_rx_cb(void)
{
    const uint32_t *src = cam_jpeg_active_line();
    uint32_t       *dst;
    uint32_t        i;

    if (s_state != CAM_JPEG_CAPTURING)
    {
        return;
    }

    dst = s_dst + s_len;

    for (i = 0U; i < CAM_JPEG_LINE_WORDS; i++)
    {
        if ((s_len + i) >= s_max_words)
        {
            break;
        }

        dst[i] = src[i];
    }

    s_len += CAM_JPEG_LINE_WORDS;

    if (s_len >= s_max_words)
    {
        s_state = CAM_JPEG_READY;
    }
}

static void cam_jpeg_frame_cb(void)
{
    const uint32_t *src;
    uint32_t       *dst;
    uint16_t        rlen;
    uint16_t        i;

    if (s_state != CAM_JPEG_CAPTURING)
    {
        return;
    }

    __HAL_DMA_DISABLE(&g_dma_dcmi_handle);

    rlen = (uint16_t)(CAM_JPEG_LINE_WORDS - __HAL_DMA_GET_COUNTER(&g_dma_dcmi_handle));
    dst  = s_dst + s_len;
    src  = ((g_dma_dcmi_handle.Instance->CR & DMA_SxCR_CT) != 0U) ? s_line[1] : s_line[0];

    for (i = 0U; i < rlen; i++)
    {
        if ((s_len + i) >= s_max_words)
        {
            break;
        }

        dst[i] = src[i];
    }

    s_len   += rlen;
    s_state  = CAM_JPEG_READY;
}

void cam_jpeg_init(uint32_t *dst, uint32_t max_words)
{
    s_dst       = dst;
    s_max_words = max_words;
    s_len       = 0U;
    s_state     = CAM_JPEG_IDLE;
}

void cam_jpeg_begin(void)
{
    s_len   = 0U;
    s_state = CAM_JPEG_CAPTURING;

    dcmi_dma_init((uint32_t)s_line[0], (uint32_t)s_line[1], CAM_JPEG_LINE_WORDS,
                  DMA_MDATAALIGN_WORD, DMA_MINC_ENABLE);
    dcmi_rx_callback    = cam_jpeg_rx_cb;
    dcmi_frame_callback = cam_jpeg_frame_cb;

    dcmi_start();
}

void cam_jpeg_end(void)
{
    dcmi_stop();

    if (s_state == CAM_JPEG_CAPTURING)
    {
        s_state = CAM_JPEG_IDLE;
    }
}

cam_jpeg_state_t cam_jpeg_state(void)
{
    return s_state;
}

uint32_t cam_jpeg_words(void)
{
    return s_len;
}

bool cam_jpeg_capture(uint32_t timeout_ms)
{
    uint32_t t0;

    cam_jpeg_begin();

    t0 = sys_get_tick();

    while ((s_state == CAM_JPEG_CAPTURING) && ((sys_get_tick() - t0) < timeout_ms))
    {
        /* wait for the frame */
    }

    cam_jpeg_end();

    return (s_state == CAM_JPEG_READY);
}
