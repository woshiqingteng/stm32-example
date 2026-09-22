/**
 * @file    dcmi.c
 * @brief   DCMI camera interface with line-buffered double-buffer DMA.
 *
 * DMA2 stream 1 (channel 1) collects one line at a time. The OV5640 outputs
 * two RGB565 pixels per 32-bit DCMI word, so the DMA is configured with a
 * 32-bit peripheral port and a 16-bit memory port. Each transfer buffer holds
 * one line (width/2 words); when a line is complete the line ISR copies it to
 * the framebuffer and then re-arms automatically through double buffering.
 */

#include "stm32f4xx_hal.h"
#include "dcmi.h"

#define DCMI_DMA_STREAM         DMA2_Stream1
#define DCMI_DMA_CHANNEL        DMA_CHANNEL_1
#define DCMI_DMA_IRQN           DMA2_Stream1_IRQn

#define DCMI_IRQ_PREEMPT_PRIO   2U
#define DCMI_IRQ_SUB_PRIO       2U
#define DCMI_DMA_PREEMPT_PRIO   2U
#define DCMI_DMA_SUB_PRIO       2U

#define DCMI_LINE_WORDS         (DCMI_MAX_LINE_PIXELS / 2U)

static DCMI_HandleTypeDef g_dcmi_handle;
static DMA_HandleTypeDef  g_dcmi_dma_handle;

static uint32_t g_line_buf[2][DCMI_LINE_WORDS];

static uint16_t *g_fb;
static uint16_t  g_fb_width;
static uint16_t  g_fb_height;

static uint16_t  g_rect_x;
static uint16_t  g_rect_y;
static uint16_t  g_rect_w;
static uint16_t  g_rect_h;

static volatile uint16_t g_line_index;

static dcmi_line_cb_t  g_line_cb;
static dcmi_frame_cb_t g_frame_cb;

volatile uint32_t g_dcmi_frame_count;

void dcmi_init(uint16_t *buf, uint16_t width, uint16_t height)
{
    g_fb        = buf;
    g_fb_width  = width;
    g_fb_height = height;

    g_rect_x = 0U;
    g_rect_y = 0U;
    g_rect_w = width;
    g_rect_h = height;
    g_line_index = 0U;

    /* ---- MSP begin: DCMI clock + PA6/PB7-9/PC6-9,11/PD3/PH8 ---- */
    {
        GPIO_InitTypeDef gpio_init = {0};

        __HAL_RCC_DCMI_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_GPIOH_CLK_ENABLE();

        gpio_init.Mode      = GPIO_MODE_AF_PP;
        gpio_init.Pull      = GPIO_PULLUP;
        gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = GPIO_AF13_DCMI;

        gpio_init.Pin = GPIO_PIN_6;
        HAL_GPIO_Init(GPIOA, &gpio_init);                       /* PCLK        */

        gpio_init.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
        HAL_GPIO_Init(GPIOB, &gpio_init);                       /* VSYNC, D6, D7 */

        gpio_init.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11;
        HAL_GPIO_Init(GPIOC, &gpio_init);                       /* D0..D4      */

        gpio_init.Pin = GPIO_PIN_3;
        HAL_GPIO_Init(GPIOD, &gpio_init);                       /* D5          */

        gpio_init.Pin = GPIO_PIN_8;
        HAL_GPIO_Init(GPIOH, &gpio_init);                       /* HREF        */
    }
    /* ---- MSP end ---- */

    g_dcmi_handle.Instance = DCMI;
    g_dcmi_handle.Init.SynchroMode      = DCMI_SYNCHRO_HARDWARE;
    g_dcmi_handle.Init.PCKPolarity      = DCMI_PCKPOLARITY_RISING;
    g_dcmi_handle.Init.VSPolarity       = DCMI_VSPOLARITY_LOW;
    g_dcmi_handle.Init.HSPolarity       = DCMI_HSPOLARITY_LOW;
    g_dcmi_handle.Init.CaptureRate      = DCMI_CR_ALL_FRAME;
    g_dcmi_handle.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
    g_dcmi_handle.Init.SyncroCode.FrameStartCode = 0U;
    g_dcmi_handle.Init.SyncroCode.LineStartCode  = 0U;
    g_dcmi_handle.Init.SyncroCode.LineEndCode    = 0U;
    g_dcmi_handle.Init.SyncroCode.FrameEndCode   = 0U;
    g_dcmi_handle.Init.JPEGMode         = DCMI_MODE_CONTINUOUS;

    (void)HAL_DCMI_Init(&g_dcmi_handle);

    __HAL_DCMI_DISABLE_IT(&g_dcmi_handle, DCMI_IT_LINE | DCMI_IT_VSYNC | DCMI_IT_ERR | DCMI_IT_OVR);
    __HAL_DCMI_ENABLE_IT(&g_dcmi_handle, DCMI_IT_FRAME);
    __HAL_DCMI_ENABLE(&g_dcmi_handle);

    HAL_NVIC_SetPriority(DCMI_IRQn, DCMI_IRQ_PREEMPT_PRIO, DCMI_IRQ_SUB_PRIO);
    HAL_NVIC_EnableIRQ(DCMI_IRQn);
}

void dcmi_config(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    if ((width == 0U) || (width > DCMI_MAX_LINE_PIXELS))
    {
        return;
    }

    if (((uint32_t)x + width) > g_fb_width)
    {
        return;
    }

    if (((uint32_t)y + height) > g_fb_height)
    {
        return;
    }

    g_rect_x = x;
    g_rect_y = y;
    g_rect_w = width;
    g_rect_h = height;
    g_line_index = 0U;
}

void dcmi_register_line_callback(dcmi_line_cb_t cb)
{
    g_line_cb = cb;
}

void dcmi_register_frame_callback(dcmi_frame_cb_t cb)
{
    g_frame_cb = cb;
}

static void dcmi_dma_start(void)
{
    uint32_t len = (uint32_t)((g_rect_w + 1U) / 2U);

    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_LINKDMA(&g_dcmi_handle, DMA_Handle, g_dcmi_dma_handle);

    g_dcmi_dma_handle.Instance                 = DCMI_DMA_STREAM;
    g_dcmi_dma_handle.Init.Channel             = DCMI_DMA_CHANNEL;
    g_dcmi_dma_handle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    g_dcmi_dma_handle.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_dcmi_dma_handle.Init.MemInc              = DMA_MINC_ENABLE;
    g_dcmi_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    g_dcmi_dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    g_dcmi_dma_handle.Init.Mode                = DMA_CIRCULAR;
    g_dcmi_dma_handle.Init.Priority            = DMA_PRIORITY_HIGH;
    g_dcmi_dma_handle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    g_dcmi_dma_handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
    g_dcmi_dma_handle.Init.MemBurst            = DMA_MBURST_SINGLE;
    g_dcmi_dma_handle.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    (void)HAL_DMA_DeInit(&g_dcmi_dma_handle);
    (void)HAL_DMA_Init(&g_dcmi_dma_handle);

    __HAL_UNLOCK(&g_dcmi_dma_handle);

    (void)HAL_DMAEx_MultiBufferStart(&g_dcmi_dma_handle, (uint32_t)&DCMI->DR,
                                     (uint32_t)g_line_buf[0], (uint32_t)g_line_buf[1], len);

    __HAL_DMA_ENABLE_IT(&g_dcmi_dma_handle, DMA_IT_TC);
    HAL_NVIC_SetPriority(DCMI_DMA_IRQN, DCMI_DMA_PREEMPT_PRIO, DCMI_DMA_SUB_PRIO);
    HAL_NVIC_EnableIRQ(DCMI_DMA_IRQN);
}

/* Capture engine control; the line DMA is armed separately by dcmi_dma_start(). */
static void dcmi_capture_enable(void)
{
    SET_BIT(DCMI->CR, DCMI_CR_CAPTURE);
}

static void dcmi_capture_disable(void)
{
    CLEAR_BIT(DCMI->CR, DCMI_CR_CAPTURE);

    while (READ_BIT(DCMI->CR, DCMI_CR_CAPTURE) != 0U)
    {
    }
}

void dcmi_start(void)
{
    g_line_index = 0U;

    dcmi_dma_start();

    __HAL_DCMI_ENABLE(&g_dcmi_handle);
    dcmi_capture_enable();
}

void dcmi_stop(void)
{
    dcmi_capture_disable();

    __HAL_DMA_DISABLE(&g_dcmi_dma_handle);
}

uint16_t dcmi_width(void)
{
    return g_rect_w;
}

uint16_t dcmi_height(void)
{
    return g_rect_h;
}

static void dcmi_line_complete(void)
{
    uint32_t *src;
    uint32_t i;
    uint16_t y;

    /* In double-buffer mode CT already points at the buffer for the next
     * transfer, so CT set means buffer 0 has just finished. */
    if ((g_dcmi_dma_handle.Instance->CR & DMA_SxCR_CT) != 0U)
    {
        src = g_line_buf[0];
    }
    else
    {
        src = g_line_buf[1];
    }

    y = (uint16_t)(g_rect_y + g_line_index);

    if ((g_fb != 0) && (y < g_fb_height))
    {
        uint16_t *dst  = g_fb + ((uint32_t)y * g_fb_width) + g_rect_x;
        uint16_t *psrc = (uint16_t *)src;

        for (i = 0U; i < g_rect_w; i++)
        {
            dst[i] = psrc[i];
        }
    }

    if (g_line_cb != 0)
    {
        g_line_cb((uint16_t *)src, g_line_index);
    }

    if (g_line_index < g_rect_h)
    {
        g_line_index++;
    }
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
    (void)hdcmi;

    __HAL_DCMI_CLEAR_FLAG(&g_dcmi_handle, DCMI_FLAG_FRAMERI);

    g_line_index = 0U;
    g_dcmi_frame_count++;

    if (g_frame_cb != 0)
    {
        g_frame_cb();
    }

    /* HAL_DCMI_IRQHandler() disables the frame interrupt, re-enable it. */
    __HAL_DCMI_ENABLE_IT(&g_dcmi_handle, DCMI_IT_FRAME);
}

void DCMI_IRQHandler(void)
{
    HAL_DCMI_IRQHandler(&g_dcmi_handle);
}

void DMA2_Stream1_IRQHandler(void)
{
    if (__HAL_DMA_GET_FLAG(&g_dcmi_dma_handle, DMA_FLAG_TCIF1_5) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(&g_dcmi_dma_handle, DMA_FLAG_TCIF1_5);
        dcmi_line_complete();
    }
}

