/**
 * @file    dcmi.c
 * @brief   DCMI camera interface driver (see dcmi.h).
 *
 * Ported from the ALIENTEK camera experiment, keeping the vendor API
 * (dcmi_init / dcmi_dma_init / dcmi_start / dcmi_stop / dcmi_cr_set /
 * dcmi_set_window) and the two application hooks. The application owns the
 * buffer handling, exactly as in the vendor code.
 */

#include "stm32f4xx_hal.h"
#include "dcmi.h"
#include "ov5640.h"

#define DCMI_IRQ_PREEMPT_PRIO   2U
#define DCMI_IRQ_SUB_PRIO       2U
#define DCMI_DMA_PREEMPT_PRIO   2U
#define DCMI_DMA_SUB_PRIO       2U

DCMI_HandleTypeDef g_dcmi_handle;
DMA_HandleTypeDef  g_dma_dcmi_handle;

void (*dcmi_rx_callback)(void);
void (*dcmi_frame_callback)(void);

volatile uint32_t g_dcmi_frame_count;

void dcmi_init(void)
{
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

void dcmi_dma_init(uint32_t mem0, uint32_t mem1, uint16_t memsize, uint32_t memblen, uint32_t meminc)
{
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_LINKDMA(&g_dcmi_handle, DMA_Handle, g_dma_dcmi_handle);
    __HAL_DMA_DISABLE_IT(&g_dma_dcmi_handle, DMA_IT_TC);

    g_dma_dcmi_handle.Instance                 = DMA2_Stream1;
    g_dma_dcmi_handle.Init.Channel             = DMA_CHANNEL_1;
    g_dma_dcmi_handle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    g_dma_dcmi_handle.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_dma_dcmi_handle.Init.MemInc              = meminc;
    g_dma_dcmi_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    g_dma_dcmi_handle.Init.MemDataAlignment    = memblen;
    g_dma_dcmi_handle.Init.Mode                = DMA_CIRCULAR;
    g_dma_dcmi_handle.Init.Priority            = DMA_PRIORITY_HIGH;
    g_dma_dcmi_handle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    g_dma_dcmi_handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
    g_dma_dcmi_handle.Init.MemBurst            = DMA_MBURST_SINGLE;
    g_dma_dcmi_handle.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    (void)HAL_DMA_DeInit(&g_dma_dcmi_handle);
    (void)HAL_DMA_Init(&g_dma_dcmi_handle);

    __HAL_UNLOCK(&g_dma_dcmi_handle);

    if (mem1 == 0U)
    {
        (void)HAL_DMA_Start(&g_dma_dcmi_handle, (uint32_t)&DCMI->DR, mem0, memsize);
    }
    else
    {
        (void)HAL_DMAEx_MultiBufferStart(&g_dma_dcmi_handle, (uint32_t)&DCMI->DR, mem0, mem1, memsize);

        __HAL_DMA_ENABLE_IT(&g_dma_dcmi_handle, DMA_IT_TC);
        HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, DCMI_DMA_PREEMPT_PRIO, DCMI_DMA_SUB_PRIO);
        HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
    }
}

void dcmi_start(void)
{
    __HAL_DMA_ENABLE(&g_dma_dcmi_handle);
    SET_BIT(DCMI->CR, DCMI_CR_CAPTURE);
}

void dcmi_stop(void)
{
    CLEAR_BIT(DCMI->CR, DCMI_CR_CAPTURE);

    while (READ_BIT(DCMI->CR, DCMI_CR_CAPTURE) != 0U)
    {
    }

    __HAL_DMA_DISABLE(&g_dma_dcmi_handle);
}

void dcmi_cr_set(uint8_t pclk, uint8_t hsync, uint8_t vsync)
{
    (void)HAL_DCMI_DeInit(&g_dcmi_handle);

    g_dcmi_handle.Instance             = DCMI;
    g_dcmi_handle.Init.SynchroMode     = DCMI_SYNCHRO_HARDWARE;
    g_dcmi_handle.Init.PCKPolarity     = (uint32_t)pclk << 5;
    g_dcmi_handle.Init.VSPolarity      = (uint32_t)vsync << 7;
    g_dcmi_handle.Init.HSPolarity      = (uint32_t)hsync << 6;
    g_dcmi_handle.Init.CaptureRate     = DCMI_CR_ALL_FRAME;
    g_dcmi_handle.Init.ExtendedDataMode= DCMI_EXTEND_DATA_8B;

    (void)HAL_DCMI_Init(&g_dcmi_handle);
    g_dcmi_handle.Instance->CR |= DCMI_MODE_CONTINUOUS;
}

void dcmi_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    /* The vendor version also clears the panel and sets an LCD GRAM window;
     * the RGB panel has no GRAM window, so capture is re-armed with a new
     * sensor output window only. */
    (void)sx;
    (void)sy;

    dcmi_stop();
    (void)ov5640_outsize_set(0U, 0U, width, height);
    dcmi_start();
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
    (void)hdcmi;

    __HAL_DCMI_CLEAR_FLAG(&g_dcmi_handle, DCMI_FLAG_FRAMERI);

    g_dcmi_frame_count++;

    if (dcmi_frame_callback != 0)
    {
        dcmi_frame_callback();
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
    if (__HAL_DMA_GET_FLAG(&g_dma_dcmi_handle, DMA_FLAG_TCIF1_5) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(&g_dma_dcmi_handle, DMA_FLAG_TCIF1_5);

        if (dcmi_rx_callback != 0)
        {
            dcmi_rx_callback();
        }
    }
}
