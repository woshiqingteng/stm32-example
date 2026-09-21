/**
 * @file    adc.c
 * @brief   ADC1 driver: polled channel read and single/scan DMA acquisition.
 *
 * MSP content (clock/GPIO/NVIC/DMA) is inlined into the init functions instead
 * of HAL_ADC_MspInit().
 */

#include "stm32f4xx_hal.h"
#include "adc.h"
#include "delay.h"

#define ADC_INSTANCE            ADC1
#define ADC_DMA_STREAM          DMA2_Stream4
#define ADC_DMA_CHANNEL_ID      DMA_CHANNEL_0
#define ADC_DMA_IRQN            DMA2_Stream4_IRQn

#define ADC_GPIO_PORT           GPIOA
#define ADC_SINGLE_GPIO_PIN     GPIO_PIN_5
#define ADC_SCAN_GPIO_PINS      (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | \
                                 GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5)

#define ADC_SINGLE_CONV_NUM     1U
#define ADC_RANK_FIRST          1U
#define ADC_POLL_TIMEOUT_MS     10U
#define ADC_AVG_DELAY_MS        5U

#define ADC_DMA_IRQ_PRIORITY    3U
#define ADC_DMA_IRQ_SUBPRIORITY 3U

typedef enum
{
    ADC_DMA_MODE_NONE = 0,
    ADC_DMA_MODE_SINGLE,
    ADC_DMA_MODE_SCAN,
} adc_dma_mode_t;

static const uint32_t g_adc_scan_channels[ADC_SCAN_CH_NUM] =
{
    ADC_CH0, ADC_CH1, ADC_CH2, ADC_CH3, ADC_CH4, ADC_CH5,
};

static ADC_HandleTypeDef g_adc_handle;
static ADC_HandleTypeDef g_adc_dma_handle;
static ADC_HandleTypeDef g_adc_scan_handle;
static DMA_HandleTypeDef g_adc_dma_stream;
static adc_dma_mode_t    g_adc_dma_mode = ADC_DMA_MODE_NONE;
static adc_dma_cb_t      g_adc_dma_hook;

static void adc_gpio_config(uint32_t pins)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin  = pins;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ADC_GPIO_PORT, &gpio_init);
}

static void adc_instance_config(ADC_HandleTypeDef *hadc, FunctionalState scan,
                                uint32_t conversions, FunctionalState continuous,
                                FunctionalState dma_continuous)
{
    hadc->Instance = ADC_INSTANCE;
    hadc->Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc->Init.Resolution            = ADC_RESOLUTION_12B;
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
    HAL_ADC_Init(hadc);
}

static void adc_channel_config(ADC_HandleTypeDef *hadc, uint32_t channel, uint32_t rank)
{
    ADC_ChannelConfTypeDef channel_config = {0};

    channel_config.Channel      = channel;
    channel_config.Rank         = rank;
    channel_config.SamplingTime = ADC_SAMPLE_TIME;
    channel_config.Offset       = 0U;
    HAL_ADC_ConfigChannel(hadc, &channel_config);
}

void adc_init(void)
{
    /* ---- MSP begin: clock + GPIO ---- */
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    adc_gpio_config(ADC_SINGLE_GPIO_PIN);
    /* ---- MSP end ---- */

    adc_instance_config(&g_adc_handle, DISABLE, ADC_SINGLE_CONV_NUM, DISABLE, DISABLE);
}

uint32_t adc_get_result(uint32_t channel)
{
    adc_channel_config(&g_adc_handle, channel, ADC_RANK_FIRST);

    HAL_ADC_Start(&g_adc_handle);
    HAL_ADC_PollForConversion(&g_adc_handle, ADC_POLL_TIMEOUT_MS);

    return (uint32_t)HAL_ADC_GetValue(&g_adc_handle);
}

uint32_t adc_get_result_average(uint32_t channel, uint8_t times)
{
    uint32_t sum = 0U;
    uint8_t  i;

    if (times == 0U)
    {
        return 0U;
    }

    for (i = 0U; i < times; i++)
    {
        sum += adc_get_result(channel);
        delay_ms(ADC_AVG_DELAY_MS);
    }

    return sum / times;
}

static void adc_dma_arm(uint16_t len)
{
    ADC_HandleTypeDef *hadc;
    uint32_t flag;

    if (g_adc_dma_mode == ADC_DMA_MODE_NONE)
    {
        return;
    }

    hadc = (g_adc_dma_mode == ADC_DMA_MODE_SCAN) ? &g_adc_scan_handle : &g_adc_dma_handle;
    flag = __HAL_DMA_GET_TC_FLAG_INDEX(&g_adc_dma_stream);

    __HAL_ADC_DISABLE(hadc);
    __HAL_DMA_DISABLE(&g_adc_dma_stream);
    __HAL_DMA_CLEAR_FLAG(&g_adc_dma_stream, flag);

    g_adc_dma_stream.Instance->NDTR = len;
    __HAL_DMA_ENABLE(&g_adc_dma_stream);

    SET_BIT(hadc->Instance->CR2, ADC_CR2_DMA);
    __HAL_ADC_ENABLE(hadc);
    SET_BIT(hadc->Instance->CR2, ADC_CR2_SWSTART);
}

static void adc_dma_config(uint16_t *buf, uint16_t len, adc_dma_mode_t mode)
{
    ADC_HandleTypeDef *hadc = (mode == ADC_DMA_MODE_SCAN) ? &g_adc_scan_handle : &g_adc_dma_handle;
    uint32_t i;

    /* ---- MSP begin: clocks + GPIO + NVIC ---- */
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    adc_gpio_config((mode == ADC_DMA_MODE_SCAN) ? ADC_SCAN_GPIO_PINS : ADC_SINGLE_GPIO_PIN);

    HAL_NVIC_SetPriority(ADC_DMA_IRQN, ADC_DMA_IRQ_PRIORITY, ADC_DMA_IRQ_SUBPRIORITY);
    HAL_NVIC_EnableIRQ(ADC_DMA_IRQN);
    /* ---- MSP end ---- */

    g_adc_dma_stream.Instance                 = ADC_DMA_STREAM;
    g_adc_dma_stream.Init.Channel             = ADC_DMA_CHANNEL_ID;
    g_adc_dma_stream.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    g_adc_dma_stream.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_adc_dma_stream.Init.MemInc              = DMA_MINC_ENABLE;
    g_adc_dma_stream.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    g_adc_dma_stream.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    g_adc_dma_stream.Init.Mode                = DMA_NORMAL;
    g_adc_dma_stream.Init.Priority            = DMA_PRIORITY_MEDIUM;
    HAL_DMA_Init(&g_adc_dma_stream);

    hadc->DMA_Handle = &g_adc_dma_stream;

    if (mode == ADC_DMA_MODE_SCAN)
    {
        adc_instance_config(hadc, ENABLE, (uint32_t)ADC_SCAN_CH_NUM, ENABLE, ENABLE);

        for (i = 0U; i < (uint32_t)ADC_SCAN_CH_NUM; i++)
        {
            adc_channel_config(hadc, g_adc_scan_channels[i], i + 1U);
        }
    }
    else
    {
        adc_instance_config(hadc, DISABLE, ADC_SINGLE_CONV_NUM, ENABLE, ENABLE);
        adc_channel_config(hadc, ADC_CH5, ADC_RANK_FIRST);
    }

    HAL_DMA_Start(&g_adc_dma_stream, (uint32_t)&ADC_INSTANCE->DR, (uint32_t)buf, (uint32_t)len);
    __HAL_DMA_ENABLE_IT(&g_adc_dma_stream, DMA_IT_TC);

    g_adc_dma_mode = mode;
}

void adc_dma_init(uint16_t *buf, uint16_t len)
{
    adc_dma_config(buf, len, ADC_DMA_MODE_SINGLE);
}

void adc_dma_start(uint16_t len)
{
    adc_dma_arm(len);
}

void adc_scan_dma_init(uint16_t *buf, uint16_t len)
{
    adc_dma_config(buf, len, ADC_DMA_MODE_SCAN);
}

void adc_scan_dma_start(uint16_t len)
{
    adc_dma_arm(len);
}

void adc_register_dma_hook(adc_dma_cb_t cb)
{
    g_adc_dma_hook = cb;
}

void DMA2_Stream4_IRQHandler(void)
{
    uint32_t flag = __HAL_DMA_GET_TC_FLAG_INDEX(&g_adc_dma_stream);

    if (__HAL_DMA_GET_FLAG(&g_adc_dma_stream, flag) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(&g_adc_dma_stream, flag);

        if (g_adc_dma_hook != 0)
        {
            g_adc_dma_hook();
        }
    }
}
