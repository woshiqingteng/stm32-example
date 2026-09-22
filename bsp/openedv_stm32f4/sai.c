/**
 * @file    sai.c
 * @brief   SAI1 audio interface: block A playback and block B capture.
 */

#include "sai.h"
#include "delay.h"

SAI_HandleTypeDef g_sai1_a_handle;        /* SAI1 block A */
SAI_HandleTypeDef g_sai1_b_handle;        /* SAI1 block B */
DMA_HandleTypeDef g_sai1_tx_dma_handle;   /* playback DMA */
DMA_HandleTypeDef g_sai1_rx_dma_handle;   /* capture DMA  */

void sai1_saia_init(uint8_t mode, uint8_t cpol, uint8_t datalen)
{
    HAL_SAI_DeInit(&g_sai1_a_handle);                            /* clear previous config */

    g_sai1_a_handle.Instance = SAI1_Block_A;
    g_sai1_a_handle.Init.AudioMode = mode;
    g_sai1_a_handle.Init.Synchro = SAI_ASYNCHRONOUS;
    g_sai1_a_handle.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
    g_sai1_a_handle.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
    g_sai1_a_handle.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
    g_sai1_a_handle.Init.ClockSource = SAI_CLKSOURCE_PLLI2S;
    g_sai1_a_handle.Init.MonoStereoMode = SAI_STEREOMODE;
    g_sai1_a_handle.Init.Protocol = SAI_FREE_PROTOCOL;
    g_sai1_a_handle.Init.DataSize = datalen;
    g_sai1_a_handle.Init.FirstBit = SAI_FIRSTBIT_MSB;
    g_sai1_a_handle.Init.ClockStrobing = cpol;

    /* Frame description. */
    g_sai1_a_handle.FrameInit.FrameLength = 64;                  /* 2 channels x 32 SCK */
    g_sai1_a_handle.FrameInit.ActiveFrameLength = 32;
    g_sai1_a_handle.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
    g_sai1_a_handle.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
    g_sai1_a_handle.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;

    /* Slot description. */
    g_sai1_a_handle.SlotInit.FirstBitOffset = 0;
    g_sai1_a_handle.SlotInit.SlotSize = SAI_SLOTSIZE_32B;
    g_sai1_a_handle.SlotInit.SlotNumber = 2;
    g_sai1_a_handle.SlotInit.SlotActive = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1;

    HAL_SAI_Init(&g_sai1_a_handle);
    __HAL_SAI_ENABLE(&g_sai1_a_handle);
}

void sai1_saib_init(uint8_t mode, uint8_t cpol, uint8_t datalen)
{
    HAL_SAI_DeInit(&g_sai1_b_handle);                           /* clear previous config */
    g_sai1_b_handle.Instance = SAI1_Block_B;
    g_sai1_b_handle.Init.AudioMode = mode;
    g_sai1_b_handle.Init.Synchro = SAI_SYNCHRONOUS;
    g_sai1_b_handle.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
    g_sai1_b_handle.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
    g_sai1_b_handle.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
    g_sai1_b_handle.Init.ClockSource = SAI_CLKSOURCE_PLLI2S;
    g_sai1_b_handle.Init.MonoStereoMode = SAI_STEREOMODE;
    g_sai1_b_handle.Init.Protocol = SAI_FREE_PROTOCOL;
    g_sai1_b_handle.Init.DataSize = datalen;
    g_sai1_b_handle.Init.FirstBit = SAI_FIRSTBIT_MSB;
    g_sai1_b_handle.Init.ClockStrobing = cpol;

    /* Frame description. */
    g_sai1_b_handle.FrameInit.FrameLength = 64;
    g_sai1_b_handle.FrameInit.ActiveFrameLength = 32;
    g_sai1_b_handle.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
    g_sai1_b_handle.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
    g_sai1_b_handle.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;

    /* Slot description. */
    g_sai1_b_handle.SlotInit.FirstBitOffset = 0;
    g_sai1_b_handle.SlotInit.SlotSize = SAI_SLOTSIZE_32B;
    g_sai1_b_handle.SlotInit.SlotNumber = 2;
    g_sai1_b_handle.SlotInit.SlotActive = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1;

    HAL_SAI_Init(&g_sai1_b_handle);
    sai1_saib_dma_enable();
    __HAL_SAI_ENABLE(&g_sai1_b_handle);
}

void HAL_SAI_MspInit(SAI_HandleTypeDef *hsai)
{
    GPIO_InitTypeDef gpio_init_struct;

    (void)hsai;

    SAI1_SAI_CLK_ENABLE();
    SAI1_CLK_GPIO_CLK_ENABLE();
    SAI1_SCK_GPIO_CLK_ENABLE();
    SAI1_FSA_GPIO_CLK_ENABLE();
    SAI1_SDA_GPIO_CLK_ENABLE();
    SAI1_SDB_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin = SAI1_CLK_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_HIGH;
    gpio_init_struct.Alternate = GPIO_AF6_SAI1;
    HAL_GPIO_Init(SAI1_CLK_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = SAI1_SCK_GPIO_PIN;
    HAL_GPIO_Init(SAI1_SCK_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = SAI1_FSA_GPIO_PIN;
    HAL_GPIO_Init(SAI1_FSA_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = SAI1_SDA_GPIO_PIN;
    HAL_GPIO_Init(SAI1_SDA_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = SAI1_SDB_GPIO_PIN;
    HAL_GPIO_Init(SAI1_SDB_GPIO_PORT, &gpio_init_struct);
}

/*
 * SAI audio clock dividers (@ HSE = 25 MHz, PLLM = 25 -> VCO input 1 MHz):
 * MCKDIV != 0: Fs = SAI_CK_x / [512 * MCKDIV]
 * MCKDIV == 0: Fs = SAI_CK_x / 256
 * SAI_CK_x = (HSE/pllm) * PLLI2SN / PLLI2SQ / (PLLI2SDIVQ + 1)
 */
const uint16_t SAI_PSC_TBL[][5] =
{
    { 800,   344, 7, 0,  12 },    /* 8 kHz     */
    { 1102,  429, 2, 18, 2  },    /* 11.025 kHz */
    { 1600,  344, 7, 0,  6  },    /* 16 kHz    */
    { 2205,  429, 2, 18, 1  },    /* 22.05 kHz */
    { 3200,  344, 7, 0,  3  },    /* 32 kHz    */
    { 4410,  429, 2, 18, 0  },    /* 44.1 kHz  */
    { 4800,  344, 7, 0,  2  },    /* 48 kHz    */
    { 8820,  271, 2, 2,  1  },    /* 88.2 kHz  */
    { 9600,  344, 7, 0,  1  },    /* 96 kHz    */
    { 17640, 271, 2, 2,  0  },    /* 176.4 kHz */
    { 19200, 344, 7, 0,  0  },    /* 192 kHz   */
};

void sai1_saia_dma_enable(void)
{
    uint32_t tempreg = 0;
    tempreg = SAI1_Block_A->CR1;
    tempreg |= 1 << 17;                     /* enable DMA */
    SAI1_Block_A->CR1 = tempreg;
}

void sai1_saib_dma_enable(void)
{
    uint32_t tempreg = 0;
    tempreg = SAI1_Block_B->CR1;
    tempreg |= 1 << 17;                     /* enable DMA */
    SAI1_Block_B->CR1 = tempreg;
}

uint8_t sai1_samplerate_set(uint32_t samplerate)
{
    uint8_t i = 0;
    RCC_PeriphCLKInitTypeDef rcc_sai1_sture;

    for (i = 0; i < (sizeof(SAI_PSC_TBL) / 10); i++)        /* is the rate supported? */
    {
        if ((samplerate / 10) == SAI_PSC_TBL[i][0])
        {
            break;
        }
    }

    if (i == (sizeof(SAI_PSC_TBL) / 10))
    {
        return 1;                                           /* not supported */
    }

    rcc_sai1_sture.PeriphClockSelection = RCC_PERIPHCLK_SAI_PLLI2S;
    rcc_sai1_sture.PLLI2S.PLLI2SN = (uint32_t)SAI_PSC_TBL[i][1];
    rcc_sai1_sture.PLLI2S.PLLI2SQ = (uint32_t)SAI_PSC_TBL[i][2];

    /* HAL adds 1 to PLLI2SDivQ when programming DCKCFGR, so pre-add it here. */
    rcc_sai1_sture.PLLI2SDivQ = SAI_PSC_TBL[i][3] + 1;
    HAL_RCCEx_PeriphCLKConfig(&rcc_sai1_sture);

    __HAL_RCC_SAI_BLOCKACLKSOURCE_CONFIG(RCC_SAIACLKSOURCE_PLLI2S);

    __HAL_SAI_DISABLE(&g_sai1_a_handle);
    g_sai1_a_handle.Init.AudioFrequency = samplerate;
    HAL_SAI_Init(&g_sai1_a_handle);
    sai1_saia_dma_enable();
    __HAL_SAI_ENABLE(&g_sai1_a_handle);

    return 0;
}

void sai1_tx_dma_init(uint8_t *buf0, uint8_t *buf1, uint16_t num, uint8_t width)
{
    uint32_t memwidth = 0, perwidth = 0;      /* memory / peripheral width */

    switch (width)
    {
        case 0:         /* 8-bit */
            memwidth = DMA_MDATAALIGN_BYTE;
            perwidth = DMA_PDATAALIGN_BYTE;
            break;

        case 1:         /* 16-bit */
            memwidth = DMA_MDATAALIGN_HALFWORD;
            perwidth = DMA_PDATAALIGN_HALFWORD;
            break;

        case 2:         /* 32-bit */
            memwidth = DMA_MDATAALIGN_WORD;
            perwidth = DMA_PDATAALIGN_WORD;
            break;

        default:
            break;
    }

    SAI1_TX_DMA_CLK_ENABLE();
    __HAL_LINKDMA(&g_sai1_a_handle, hdmatx, g_sai1_tx_dma_handle);

    g_sai1_tx_dma_handle.Instance = SAI1_TX_DMASx;
    g_sai1_tx_dma_handle.Init.Channel = SAI1_TX_DMASx_Channel;
    g_sai1_tx_dma_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
    g_sai1_tx_dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    g_sai1_tx_dma_handle.Init.MemInc = DMA_MINC_ENABLE;
    g_sai1_tx_dma_handle.Init.PeriphDataAlignment = perwidth;
    g_sai1_tx_dma_handle.Init.MemDataAlignment = memwidth;
    g_sai1_tx_dma_handle.Init.Mode = DMA_CIRCULAR;
    g_sai1_tx_dma_handle.Init.Priority = DMA_PRIORITY_HIGH;
    g_sai1_tx_dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    g_sai1_tx_dma_handle.Init.MemBurst = DMA_MBURST_SINGLE;
    g_sai1_tx_dma_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_DeInit(&g_sai1_tx_dma_handle);
    HAL_DMA_Init(&g_sai1_tx_dma_handle);

    HAL_DMAEx_MultiBufferStart(&g_sai1_tx_dma_handle, (uint32_t)buf0, (uint32_t)&SAI1_Block_A->DR, (uint32_t)buf1, num);
    __HAL_DMA_DISABLE(&g_sai1_tx_dma_handle);
    delay_us(10);

    __HAL_DMA_ENABLE_IT(&g_sai1_tx_dma_handle, DMA_IT_TC);
    __HAL_DMA_CLEAR_FLAG(&g_sai1_tx_dma_handle, SAI1_TX_DMASx_FLAG);
    HAL_NVIC_SetPriority(SAI1_TX_DMASx_IRQ, 0, 0);
    HAL_NVIC_EnableIRQ(SAI1_TX_DMASx_IRQ);
}

void sai1_rx_dma_init(uint8_t *buf0, uint8_t *buf1, uint16_t num, uint8_t width)
{
    uint32_t memwidth = 0, perwidth = 0;

    switch (width)
    {
        case 0:         /* 8-bit */
            memwidth = DMA_MDATAALIGN_BYTE;
            perwidth = DMA_PDATAALIGN_BYTE;
            break;

        case 1:         /* 16-bit */
            memwidth = DMA_MDATAALIGN_HALFWORD;
            perwidth = DMA_PDATAALIGN_HALFWORD;
            break;

        case 2:         /* 32-bit */
            memwidth = DMA_MDATAALIGN_WORD;
            perwidth = DMA_PDATAALIGN_WORD;
            break;

        default:
            break;
    }

    SAI1_RX_DMA_CLK_ENABLE();
    __HAL_LINKDMA(&g_sai1_b_handle, hdmarx, g_sai1_rx_dma_handle);

    g_sai1_rx_dma_handle.Instance = SAI1_RX_DMASx;
    g_sai1_rx_dma_handle.Init.Channel = SAI1_RX_DMASx_Channel;
    g_sai1_rx_dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    g_sai1_rx_dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    g_sai1_rx_dma_handle.Init.MemInc = DMA_MINC_ENABLE;
    g_sai1_rx_dma_handle.Init.PeriphDataAlignment = perwidth;
    g_sai1_rx_dma_handle.Init.MemDataAlignment = memwidth;
    g_sai1_rx_dma_handle.Init.Mode = DMA_CIRCULAR;
    g_sai1_rx_dma_handle.Init.Priority = DMA_PRIORITY_MEDIUM;
    g_sai1_rx_dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    g_sai1_rx_dma_handle.Init.MemBurst = DMA_MBURST_SINGLE;
    g_sai1_rx_dma_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_DeInit(&g_sai1_rx_dma_handle);
    HAL_DMA_Init(&g_sai1_rx_dma_handle);

    HAL_DMAEx_MultiBufferStart(&g_sai1_rx_dma_handle, (uint32_t)&SAI1_Block_B->DR, (uint32_t)buf0, (uint32_t)buf1, num);
    __HAL_DMA_DISABLE(&g_sai1_rx_dma_handle);
    delay_us(10);

    __HAL_DMA_CLEAR_FLAG(&g_sai1_rx_dma_handle, SAI1_RX_DMASx_FLAG);
    __HAL_DMA_ENABLE_IT(&g_sai1_rx_dma_handle, DMA_IT_TC);

    HAL_NVIC_SetPriority(SAI1_RX_DMASx_IRQ, 0, 1);
    HAL_NVIC_EnableIRQ(SAI1_RX_DMASx_IRQ);
}

/* DMA transfer-complete callbacks. */
void (*sai_tx_callback)(void);
void (*sai_rx_callback)(void);

void SAI1_TX_DMASx_IRQHandler(void)
{
    if (__HAL_DMA_GET_FLAG(&g_sai1_tx_dma_handle, SAI1_TX_DMASx_FLAG) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(&g_sai1_tx_dma_handle, SAI1_TX_DMASx_FLAG);
        if (sai_tx_callback != NULL)
        {
            sai_tx_callback();
        }
    }
}

void SAI1_RX_DMASx_IRQHandler(void)
{
    if (__HAL_DMA_GET_FLAG(&g_sai1_rx_dma_handle, SAI1_RX_DMASx_FLAG) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(&g_sai1_rx_dma_handle, SAI1_RX_DMASx_FLAG);
        if (sai_rx_callback != NULL)
        {
            sai_rx_callback();
        }
    }
}

void sai1_play_start(void)
{
    __HAL_DMA_ENABLE(&g_sai1_tx_dma_handle);
}

void sai1_play_stop(void)
{
    __HAL_DMA_DISABLE(&g_sai1_tx_dma_handle);
}

void sai1_rec_start(void)
{
    __HAL_DMA_ENABLE(&g_sai1_rx_dma_handle);
}

void sai1_rec_stop(void)
{
    __HAL_DMA_DISABLE(&g_sai1_rx_dma_handle);
}
