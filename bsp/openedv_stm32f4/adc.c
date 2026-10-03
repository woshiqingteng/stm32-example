/**
 * @file    adc.c
 * @brief   ADC1 pure driver: polled single read and DMA acquisition.
 *
 * Only acquisition lives here. MSP content (clock/GPIO/NVIC/DMA) is inlined and
 * the DMA completion is handled explicitly (no HAL weak callbacks). Averaging,
 * voltage and temperature conversion belong to the application.
 */

#include "stm32f4xx_hal.h"
#include "adc.h"
#include "delay.h"

#define ADC_INSTANCE          ADC1
#define ADC_DMA_STREAM        DMA2_Stream4
#define ADC_DMA_CHANNEL_ID    DMA_CHANNEL_0
#define ADC_DMA_IRQN          DMA2_Stream4_IRQn
#define ADC_POLL_TIMEOUT_MS   10U

/* ===== channel / option mapping (own enum -> HAL) ===== */

static const uint32_t g_adc_ch_hal[ADC_CH_NUM] =
{
    ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3,
    ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_18, /* ADC_TEMP_CH */
};

static uint32_t adc_res_to_hal(adc_resolution_t v)
{
    switch (v)
    {
        case ADC_RES_10B: return ADC_RESOLUTION_10B;
        case ADC_RES_8B:  return ADC_RESOLUTION_8B;
        case ADC_RES_6B:  return ADC_RESOLUTION_6B;
        default:          return ADC_RESOLUTION_12B;
    }
}

static uint32_t adc_sample_to_hal(adc_sample_time_t v)
{
    static const uint32_t cycles[8] =
    {
        ADC_SAMPLETIME_3CYCLES, ADC_SAMPLETIME_15CYCLES, ADC_SAMPLETIME_28CYCLES,
        ADC_SAMPLETIME_56CYCLES, ADC_SAMPLETIME_84CYCLES, ADC_SAMPLETIME_112CYCLES,
        ADC_SAMPLETIME_144CYCLES, ADC_SAMPLETIME_480CYCLES,
    };

    return cycles[(v < 8) ? (uint32_t)v : 7U];
}

static uint32_t adc_clock_to_hal(adc_clock_t v)
{
    static const uint32_t presc[4] =
    {
        ADC_CLOCK_SYNC_PCLK_DIV2, ADC_CLOCK_SYNC_PCLK_DIV4,
        ADC_CLOCK_SYNC_PCLK_DIV6, ADC_CLOCK_SYNC_PCLK_DIV8,
    };

    return presc[(v < 4) ? (uint32_t)v : 1U];
}

/* ===== state ===== */

static ADC_HandleTypeDef g_adc;          /* polled */
static ADC_HandleTypeDef g_adc_dma;      /* DMA */
static DMA_HandleTypeDef g_adc_dma_stream;
static adc_cfg_t         g_cfg;
static uint16_t         *g_dma_buf;
static adc_dma_cb_t      g_dma_cb;

/* ===== helpers ===== */

static void adc_instance_config(ADC_HandleTypeDef *hadc, FunctionalState scan,
                                uint32_t conversions, FunctionalState continuous,
                                FunctionalState dma_continuous)
{
    hadc->Instance = ADC_INSTANCE;
    hadc->Init.ClockPrescaler        = adc_clock_to_hal(g_cfg.clock);
    hadc->Init.Resolution            = adc_res_to_hal(g_cfg.resolution);
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

static void adc_channel_config(ADC_HandleTypeDef *hadc, adc_channel_t ch, uint32_t rank)
{
    ADC_ChannelConfTypeDef channel_config = {0};

    channel_config.Channel      = g_adc_ch_hal[(ch < ADC_CH_NUM) ? (uint32_t)ch : 0U];
    channel_config.Rank         = rank;
    channel_config.SamplingTime = adc_sample_to_hal(g_cfg.sample_time);
    channel_config.Offset       = 0U;
    (void)HAL_ADC_ConfigChannel(hadc, &channel_config);
}

static uint32_t adc_channel_pin(adc_channel_t ch)
{
    return (ch <= ADC_CH5) ? (uint32_t)(1UL << (uint32_t)ch) : 0U; /* CH0..5 -> PA0..PA5 */
}

static void adc_gpio_analog(uint32_t pins)
{
    GPIO_InitTypeDef gpio = {0};

    if (pins == 0U)
    {
        return;
    }
    gpio.Pin  = pins;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
}

/* ===== public API ===== */

void adc_init(const adc_cfg_t *cfg)
{
    static const adc_cfg_t cfg_default = { ADC_CFG_DEFAULT };
    uint32_t i;
    uint32_t pins = 0U;

    g_cfg = (cfg != 0) ? *cfg : cfg_default;

    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Polled handle uses PA5 (single channel). */
    adc_gpio_analog(GPIO_PIN_5);
    adc_instance_config(&g_adc, DISABLE, 1U, DISABLE, DISABLE);

    if (g_cfg.mode != ADC_MODE_DMA)
    {
        return;
    }

    g_dma_buf = g_cfg.dma_buf;
    g_dma_cb  = g_cfg.dma_cb;

    for (i = 0U; i < (uint32_t)g_cfg.nchans; i++)
    {
        pins |= adc_channel_pin(g_cfg.chans[i]);
    }
    adc_gpio_analog(pins);

    /* ---- MSP begin: DMA clock + NVIC ---- */
    __HAL_RCC_DMA2_CLK_ENABLE();
    HAL_NVIC_SetPriority(ADC_DMA_IRQN, 3U, 3U);
    HAL_NVIC_EnableIRQ(ADC_DMA_IRQN);
    /* ---- MSP end ---- */

    g_adc_dma_stream.Instance                 = ADC_DMA_STREAM;
    g_adc_dma_stream.Init.Channel             = ADC_DMA_CHANNEL_ID;
    g_adc_dma_stream.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    g_adc_dma_stream.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_adc_dma_stream.Init.MemInc              = DMA_MINC_ENABLE;
    g_adc_dma_stream.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    g_adc_dma_stream.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    g_adc_dma_stream.Init.Mode                = (g_cfg.dma_mode == ADC_DMA_CIRCULAR) ? DMA_CIRCULAR : DMA_NORMAL;
    g_adc_dma_stream.Init.Priority            = DMA_PRIORITY_MEDIUM;
    g_adc_dma_stream.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    (void)HAL_DMA_Init(&g_adc_dma_stream);

    __HAL_LINKDMA(&g_adc_dma, DMA_Handle, g_adc_dma_stream);

    adc_instance_config(&g_adc_dma,
                        (g_cfg.nchans > 1U) ? ENABLE : DISABLE,
                        (uint32_t)g_cfg.nchans, ENABLE, ENABLE);
    for (i = 0U; i < (uint32_t)g_cfg.nchans; i++)
    {
        adc_channel_config(&g_adc_dma, g_cfg.chans[i], (uint32_t)(i + 1U));
    }

    adc_dma_start();
}

uint32_t adc_read(adc_channel_t ch)
{
    if (ch == ADC_TEMP_CH)
    {
        SET_BIT(ADC->CCR, ADC_CCR_TSVREFE);
    }

    adc_channel_config(&g_adc, ch, 1U);
    (void)HAL_ADC_Start(&g_adc);
    (void)HAL_ADC_PollForConversion(&g_adc, ADC_POLL_TIMEOUT_MS);

    return (uint32_t)HAL_ADC_GetValue(&g_adc);
}

void adc_dma_start(void)
{
    if ((g_cfg.mode != ADC_MODE_DMA) || (g_dma_buf == 0) || (g_cfg.dma_len == 0U))
    {
        return;
    }

    /* Restart the sequence from rank 1 and let the ADC stabilise after ADON. */
    __HAL_ADC_DISABLE(&g_adc_dma);
    __HAL_ADC_CLEAR_FLAG(&g_adc_dma, ADC_FLAG_EOC | ADC_FLAG_OVR);

    SET_BIT(ADC1->CR2, ADC_CR2_DMA);
    if (HAL_DMA_Start(&g_adc_dma_stream, (uint32_t)&ADC1->DR,
                      (uint32_t)g_dma_buf, g_cfg.dma_len) != HAL_OK)
    {
        CLEAR_BIT(ADC1->CR2, ADC_CR2_DMA);
        return;
    }

    __HAL_DMA_ENABLE_IT(&g_adc_dma_stream, DMA_IT_TC | DMA_IT_TE | DMA_IT_FE | DMA_IT_DME
                                           | (g_cfg.dma_half_cb ? DMA_IT_HT : 0U));

    __HAL_ADC_ENABLE(&g_adc_dma);
    delay_us(ADC_STAB_DELAY_US);
    SET_BIT(ADC1->CR2, ADC_CR2_SWSTART);
}

/* ===== interrupts ===== */

void DMA2_Stream4_IRQHandler(void)
{
    DMA_HandleTypeDef *hdma = &g_adc_dma_stream;

    if (g_cfg.dma_half_cb &&
        (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_HT_FLAG_INDEX(hdma)) != RESET))
    {
        __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_HT_FLAG_INDEX(hdma));
        if (g_dma_cb != 0)
        {
            g_dma_cb(0U);
        }
    }

    if (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma)) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma));
        if (g_cfg.dma_mode == ADC_DMA_ONESHOT)
        {
            (void)HAL_DMA_Abort(hdma);
            CLEAR_BIT(ADC1->CR2, ADC_CR2_DMA);
        }
        if (g_dma_cb != 0)
        {
            g_dma_cb((uint16_t)(g_cfg.dma_len / 2U));
        }
    }

    if ((__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_TE_FLAG_INDEX(hdma)) != RESET) ||
        (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_FE_FLAG_INDEX(hdma)) != RESET) ||
        (__HAL_DMA_GET_FLAG(hdma, __HAL_DMA_GET_DME_FLAG_INDEX(hdma)) != RESET))
    {
        (void)HAL_DMA_Abort(hdma);
        CLEAR_BIT(ADC1->CR2, ADC_CR2_DMA);
    }
}
