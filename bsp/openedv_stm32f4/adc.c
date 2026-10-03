/**
 * @file    adc.c
 * @brief   ADC driver: polled single read and DMA acquisition.
 *
 * Only acquisition lives here. All hardware facts live in the per-instance
 * adc_hw_t table; per-instance state is held in adc_handle_t. The DMA completion
 * is handled explicitly (no HAL weak callbacks) and no ADC OVR interrupt is used.
 * Averaging, voltage and temperature conversion belong to the application.
 */

#include "stm32f4xx_hal.h"
#include "adc.h"
#include "delay.h"
#include "gpio_hw.h"

#define ADC_POLL_TIMEOUT_MS 10U

/* ===== option mapping (own enum -> HAL) ===== */

static const uint32_t g_res_hal[4] =
{
    ADC_RESOLUTION_12B, ADC_RESOLUTION_10B, ADC_RESOLUTION_8B, ADC_RESOLUTION_6B,
};

static const uint32_t g_smp_hal[8] =
{
    ADC_SAMPLETIME_3CYCLES, ADC_SAMPLETIME_15CYCLES, ADC_SAMPLETIME_28CYCLES,
    ADC_SAMPLETIME_56CYCLES, ADC_SAMPLETIME_84CYCLES, ADC_SAMPLETIME_112CYCLES,
    ADC_SAMPLETIME_144CYCLES, ADC_SAMPLETIME_480CYCLES,
};

static const uint32_t g_clk_hal[4] =
{
    ADC_CLOCK_SYNC_PCLK_DIV2, ADC_CLOCK_SYNC_PCLK_DIV4,
    ADC_CLOCK_SYNC_PCLK_DIV6, ADC_CLOCK_SYNC_PCLK_DIV8,
};

/* ===== hardware descriptors ===== */

typedef struct
{
    ADC_TypeDef        *instance;
    ADC_Common_TypeDef *common;      /* ADC123_COMMON (CCR / TSVREFE) */
    uint32_t            adc_rcc_en;  /* RCC_APB2ENR_ADC1EN */
    uint32_t            dma_rcc_en;  /* RCC_AHB1ENR_DMA2EN */
    DMA_Stream_TypeDef *dma_stream;
    uint32_t            dma_channel;
    IRQn_Type           dma_irqn;
    gpio_hw_t           gpio;        /* generic GPIO attributes */
    uint16_t            poll_pins;   /* analog pins configured for polling */
    uint16_t            ch_pin[ADC_CH_NUM]; /* GPIO_PIN_x per channel id (0 = none) */
    uint32_t            ch_hal[ADC_CH_NUM]; /* ADC_CHANNEL_x per channel id (not GPIO) */
} adc_hw_t;

static const adc_hw_t g_adc_hw[ADC_ID_NUM] =
{
    {
        .instance    = ADC1,
        .common      = ADC,
        .adc_rcc_en  = RCC_APB2ENR_ADC1EN,
        .dma_rcc_en  = RCC_AHB1ENR_DMA2EN,
        .dma_stream  = DMA2_Stream4,
        .dma_channel = DMA_CHANNEL_0,
        .dma_irqn    = DMA2_Stream4_IRQn,
        .gpio        = { GPIOA, RCC_AHB1ENR_GPIOAEN, GPIO_MODE_ANALOG,
                         GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0U },
        .poll_pins   = GPIO_PIN_5,
        .ch_pin      = { GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3,
                         GPIO_PIN_4, GPIO_PIN_5, 0U },
        .ch_hal      = { ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3,
                         ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_18 },
    },
    { .instance = 0 }, /* ADC_ID_2: reserved (not wired) */
    { .instance = 0 }, /* ADC_ID_3: reserved (not wired) */
};

/* ===== per-instance state ===== */

typedef struct
{
    const adc_hw_t   *hw;
    ADC_HandleTypeDef poll;
    ADC_HandleTypeDef dma;
    DMA_HandleTypeDef dma_stream;
    adc_cfg_t         cfg;
    uint16_t         *dma_buf;
    adc_dma_cb_t      dma_cb;
} adc_handle_t;

static adc_handle_t g_adc[ADC_ID_NUM];

/* ===== helpers ===== */

static void adc_instance_config(ADC_HandleTypeDef *hadc, const adc_hw_t *hw,
                                const adc_cfg_t *cfg, FunctionalState scan,
                                uint32_t conversions, FunctionalState continuous,
                                FunctionalState dma_continuous)
{
    hadc->Instance = hw->instance;
    hadc->Init.ClockPrescaler        = g_clk_hal[(cfg->clock < 4) ? (uint32_t)cfg->clock : 0U];
    hadc->Init.Resolution            = g_res_hal[(cfg->resolution < 4) ? (uint32_t)cfg->resolution : 0U];
    hadc->Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc->Init.ScanConvMode          = scan;
    hadc->Init.EOCSelection          = (scan == ENABLE) ? ADC_EOC_SEQ_CONV : ADC_EOC_SINGLE_CONV;
    hadc->Init.ContinuousConvMode    = continuous;
    hadc->Init.NbrOfConversion       = conversions;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.NbrOfDiscConversion   = 0U;
    hadc->Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc->Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc->Init.DMAContinuousRequests = dma_continuous;
    (void)HAL_ADC_Init(hadc);
}

static void adc_channel_config(ADC_HandleTypeDef *hadc, const adc_hw_t *hw,
                               const adc_cfg_t *cfg, adc_channel_t ch, uint32_t rank)
{
    ADC_ChannelConfTypeDef channel_config = {0};

    channel_config.Channel      = hw->ch_hal[(ch < ADC_CH_NUM) ? (uint32_t)ch : 0U];
    channel_config.Rank         = rank;
    channel_config.SamplingTime = g_smp_hal[(cfg->sample_time < 8) ? (uint32_t)cfg->sample_time : 7U];
    channel_config.Offset       = 0U;
    (void)HAL_ADC_ConfigChannel(hadc, &channel_config);
}

/* Explicit DMA completion (no HAL weak callbacks). */
static void adc_dma_irq(adc_handle_t *h)
{
    DMA_HandleTypeDef *hdma;

    if ((h->hw == 0) || (h->dma_stream.Instance == 0))
    {
        return;
    }
    hdma = &h->dma_stream;

    if (h->cfg.dma_half_cb &&
        (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_HT_FLAG_INDEX(hdma)) != RESET))
    {
        __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_HT_FLAG_INDEX(hdma));
        if (h->dma_cb != 0)
        {
            h->dma_cb(0U);
        }
    }

    if (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma)) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma));
        if (h->cfg.dma_mode == ADC_DMA_ONESHOT)
        {
            (void)HAL_DMA_Abort(hdma);
            CLEAR_BIT(h->hw->instance->CR2, ADC_CR2_DMA);
        }
        if (h->dma_cb != 0)
        {
            h->dma_cb((uint16_t)(h->cfg.dma_len / 2U));
        }
    }

    if ((__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_TE_FLAG_INDEX(hdma)) != RESET) ||
        (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_FE_FLAG_INDEX(hdma)) != RESET) ||
        (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_DME_FLAG_INDEX(hdma)) != RESET))
    {
        (void)HAL_DMA_Abort(hdma);
        CLEAR_BIT(h->hw->instance->CR2, ADC_CR2_DMA);
    }
}

/* ===== public API ===== */

void adc_init(const adc_cfg_t *cfg)
{
    static const adc_cfg_t cfg_default = { ADC_CFG_DEFAULT };
    const adc_cfg_t *c = (cfg != 0) ? cfg : &cfg_default;
    const adc_hw_t  *hw;
    adc_handle_t    *h;
    uint32_t         i;
    uint32_t         pins = 0U;

    if ((c->id >= ADC_ID_NUM) || (g_adc_hw[c->id].instance == 0))
    {
        return; /* reserved/unwired instance */
    }
    hw = &g_adc_hw[c->id];
    h  = &g_adc[c->id];
    h->hw  = hw;
    h->cfg = *c;

    /* ---- MSP begin: clocks (GPIO clock is enabled inside gpio_hw_setup) ---- */
    SET_BIT(RCC->APB2ENR, hw->adc_rcc_en);
    SET_BIT(RCC->AHB1ENR, hw->dma_rcc_en);
    /* ---- MSP end ---- */

    /* Polled handle uses the poll analog pins. */
    gpio_hw_setup(&hw->gpio, hw->poll_pins);
    adc_instance_config(&h->poll, hw, c, DISABLE, 1U, DISABLE, DISABLE);

    if (c->mode != ADC_MODE_DMA)
    {
        return;
    }

    h->dma_buf = c->dma_buf;
    h->dma_cb  = c->dma_cb;

    for (i = 0U; i < (uint32_t)c->nchans; i++)
    {
        pins |= hw->ch_pin[(c->chans[i] < ADC_CH_NUM) ? (uint32_t)c->chans[i] : 0U];
    }
    gpio_hw_setup(&hw->gpio, pins);

    /* ---- MSP begin: DMA NVIC ---- */
    HAL_NVIC_SetPriority(hw->dma_irqn, 3U, 3U);
    HAL_NVIC_EnableIRQ(hw->dma_irqn);
    /* ---- MSP end ---- */

    h->dma_stream.Instance                 = hw->dma_stream;
    h->dma_stream.Init.Channel             = hw->dma_channel;
    h->dma_stream.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    h->dma_stream.Init.PeriphInc           = DMA_PINC_DISABLE;
    h->dma_stream.Init.MemInc              = DMA_MINC_ENABLE;
    h->dma_stream.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    h->dma_stream.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    h->dma_stream.Init.Mode                = (c->dma_mode == ADC_DMA_CIRCULAR) ? DMA_CIRCULAR : DMA_NORMAL;
    h->dma_stream.Init.Priority            = DMA_PRIORITY_MEDIUM;
    h->dma_stream.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    (void)HAL_DMA_Init(&h->dma_stream);

    __HAL_LINKDMA(&h->dma, DMA_Handle, h->dma_stream);

    adc_instance_config(&h->dma, hw, c, (c->nchans > 1U) ? ENABLE : DISABLE,
                        (uint32_t)c->nchans, ENABLE, ENABLE);
    for (i = 0U; i < (uint32_t)c->nchans; i++)
    {
        adc_channel_config(&h->dma, hw, c, c->chans[i], (uint32_t)(i + 1U));
    }

    adc_dma_start(c->id);
}

uint32_t adc_read(adc_id_t id, adc_channel_t ch)
{
    adc_handle_t *h;

    if (id >= ADC_ID_NUM)
    {
        return 0U;
    }
    h = &g_adc[id];
    if (h->hw == 0)
    {
        return 0U;
    }

    if (ch == ADC_TEMP_CH)
    {
        SET_BIT(h->hw->common->CCR, ADC_CCR_TSVREFE);
    }

    adc_channel_config(&h->poll, h->hw, &h->cfg, ch, 1U);
    (void)HAL_ADC_Start(&h->poll);
    (void)HAL_ADC_PollForConversion(&h->poll, ADC_POLL_TIMEOUT_MS);

    return (uint32_t)HAL_ADC_GetValue(&h->poll);
}

void adc_dma_start(adc_id_t id)
{
    adc_handle_t *h;

    if (id >= ADC_ID_NUM)
    {
        return;
    }
    h = &g_adc[id];
    if ((h->hw == 0) || (h->cfg.mode != ADC_MODE_DMA) ||
        (h->dma_buf == 0) || (h->cfg.dma_len == 0U))
    {
        return;
    }

    /* Restart the sequence from rank 1 and let the ADC stabilise after ADON. */
    __HAL_ADC_DISABLE(&h->dma);
    __HAL_ADC_CLEAR_FLAG(&h->dma, ADC_FLAG_EOC | ADC_FLAG_OVR);

    SET_BIT(h->hw->instance->CR2, ADC_CR2_DMA);
    if (HAL_DMA_Start(&h->dma_stream, (uint32_t)&h->hw->instance->DR,
                      (uint32_t)h->dma_buf, h->cfg.dma_len) != HAL_OK)
    {
        CLEAR_BIT(h->hw->instance->CR2, ADC_CR2_DMA);
        return;
    }

    __HAL_DMA_ENABLE_IT(&h->dma_stream, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE | DMA_IT_DME
                                         | (h->cfg.dma_half_cb ? DMA_IT_HT : 0U));

    __HAL_ADC_ENABLE(&h->dma);
    delay_us(ADC_STAB_DELAY_US);
    SET_BIT(h->hw->instance->CR2, ADC_CR2_SWSTART);
}

/* ===== interrupts ===== */

void DMA2_Stream4_IRQHandler(void)
{
    adc_dma_irq(&g_adc[ADC_ID_1]);
}
