/**
 * @file    dac.c
 * @brief   DAC1 driver: software-triggered channel output and timer-triggered
 *          DMA triangle/sine wave generation. MSP content is inlined into the
 *          init functions.
 */

#include "stm32f4xx_hal.h"
#include "dac.h"

#define DAC_INSTANCE            DAC
#define DAC_GPIO_PORT           GPIOA
#define DAC_CH1_GPIO_PIN        GPIO_PIN_4
#define DAC_CH2_GPIO_PIN        GPIO_PIN_5

#define DAC_DMA_STREAM          DMA1_Stream5
#define DAC_DMA_CHANNEL_ID      DMA_CHANNEL_7
#define DAC_DMA_IRQN            DMA1_Stream5_IRQn
#define DAC_DMA_IRQ_PRIORITY    3U
#define DAC_DMA_IRQ_SUBPRIORITY 3U

#define DAC_WAVE_SAMPLES        100U

static DAC_HandleTypeDef g_dac_handle;
static DMA_HandleTypeDef g_dac_dma_handle;
static TIM_HandleTypeDef g_dac_tim_handle;

/* One full sine period, offset into the 0..4095 range (2048 * (1 + sin)). */
static const uint16_t g_dac_sine_buf[DAC_WAVE_SAMPLES] =
{
    2048, 2177, 2305, 2432, 2557, 2681, 2802, 2920, 3035, 3145,
    3252, 3353, 3450, 3541, 3626, 3705, 3777, 3843, 3901, 3952,
    3996, 4032, 4060, 4080, 4092, 4095, 4092, 4080, 4060, 4032,
    3996, 3952, 3901, 3843, 3777, 3705, 3626, 3541, 3450, 3353,
    3252, 3145, 3035, 2920, 2802, 2681, 2557, 2432, 2305, 2177,
    2048, 1919, 1791, 1664, 1539, 1415, 1294, 1176, 1061,  951,
     844,  743,  646,  555,  470,  391,  319,  253,  195,  144,
     100,   64,   36,   16,    4,    0,    4,   16,   36,   64,
     100,  144,  195,  253,  319,  391,  470,  555,  646,  743,
     844,  951, 1061, 1176, 1294, 1415, 1539, 1664, 1791, 1919,
};

/* One full triangle period: 50 rising samples followed by 50 falling. */
static const uint16_t g_dac_triangle_buf[DAC_WAVE_SAMPLES] =
{
       0,   84,  167,  251,  334,  418,  501,  585,  669,  752,
     836,  919, 1003, 1086, 1170, 1254, 1337, 1421, 1504, 1588,
    1671, 1755, 1839, 1922, 2006, 2089, 2173, 2256, 2340, 2424,
    2507, 2591, 2674, 2758, 2841, 2925, 3009, 3092, 3176, 3259,
    3343, 3426, 3510, 3594, 3677, 3761, 3844, 3928, 4011, 4095,
    4095, 4011, 3928, 3844, 3761, 3677, 3594, 3510, 3426, 3343,
    3259, 3176, 3092, 3009, 2925, 2841, 2758, 2674, 2591, 2507,
    2424, 2340, 2256, 2173, 2089, 2006, 1922, 1839, 1755, 1671,
    1588, 1504, 1421, 1337, 1254, 1170, 1086, 1003,  919,  836,
     752,  669,  585,  501,  418,  334,  251,  167,   84,    0,
};

static void dac_gpio_config(uint32_t pins)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin  = pins;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DAC_GPIO_PORT, &gpio_init);
}

static uint32_t dac_channel_to_hal(uint32_t channel)
{
    return (channel == (uint32_t)DAC_CH2) ? DAC_CHANNEL_2 : DAC_CHANNEL_1;
}

void dac_init(void)
{
    DAC_ChannelConfTypeDef channel_config = {0};

    /* ---- MSP begin: clocks + GPIO ---- */
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    dac_gpio_config(DAC_CH1_GPIO_PIN | DAC_CH2_GPIO_PIN);
    /* ---- MSP end ---- */

    g_dac_handle.Instance = DAC_INSTANCE;
    HAL_DAC_Init(&g_dac_handle);

    channel_config.DAC_Trigger      = DAC_TRIGGER_NONE;
    channel_config.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;

    HAL_DAC_ConfigChannel(&g_dac_handle, &channel_config, DAC_CHANNEL_1);
    HAL_DAC_ConfigChannel(&g_dac_handle, &channel_config, DAC_CHANNEL_2);
    HAL_DAC_Start(&g_dac_handle, DAC_CHANNEL_1);
    HAL_DAC_Start(&g_dac_handle, DAC_CHANNEL_2);
}

void dac_set(uint32_t channel, uint16_t value)
{
    if (value > DAC_FULL_SCALE)
    {
        value = DAC_FULL_SCALE;
    }

    (void)HAL_DAC_SetValue(&g_dac_handle, dac_channel_to_hal(channel), DAC_ALIGN_12B_R, value);
}

void dac_set_voltage(uint32_t channel, uint16_t millivolt)
{
    uint32_t code;

    if (millivolt > 3300U)
    {
        millivolt = 3300U;
    }

    code = ((uint32_t)millivolt * (DAC_FULL_SCALE + 1U)) / 3300U;
    dac_set(channel, (uint16_t)code);
}

static void dac_wave_dma_init(void)
{
    /* ---- MSP begin: DMA clock + NVIC ---- */
    __HAL_RCC_DMA1_CLK_ENABLE();

    HAL_NVIC_SetPriority(DAC_DMA_IRQN, DAC_DMA_IRQ_PRIORITY, DAC_DMA_IRQ_SUBPRIORITY);
    HAL_NVIC_EnableIRQ(DAC_DMA_IRQN);
    /* ---- MSP end ---- */

    g_dac_dma_handle.Instance                 = DAC_DMA_STREAM;
    g_dac_dma_handle.Init.Channel             = DAC_DMA_CHANNEL_ID;
    g_dac_dma_handle.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    g_dac_dma_handle.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_dac_dma_handle.Init.MemInc              = DMA_MINC_ENABLE;
    g_dac_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    g_dac_dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    g_dac_dma_handle.Init.Mode                = DMA_CIRCULAR;
    g_dac_dma_handle.Init.Priority            = DMA_PRIORITY_MEDIUM;
    g_dac_dma_handle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&g_dac_dma_handle);

    __HAL_LINKDMA(&g_dac_handle, DMA_Handle1, g_dac_dma_handle);
}

static void dac_wave_timer_init(TIM_TypeDef *instance, uint16_t arr, uint16_t psc)
{
    TIM_MasterConfigTypeDef master_config = {0};

    g_dac_tim_handle.Instance               = instance;
    g_dac_tim_handle.Init.Prescaler         = psc;
    g_dac_tim_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_dac_tim_handle.Init.Period            = arr;
    g_dac_tim_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_dac_tim_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&g_dac_tim_handle);

    master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master_config.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&g_dac_tim_handle, &master_config);
}

static void dac_wave_channel_config(uint32_t trigger)
{
    DAC_ChannelConfTypeDef channel_config = {0};

    channel_config.DAC_Trigger      = trigger;
    channel_config.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&g_dac_handle, &channel_config, DAC_CHANNEL_1);
}

static void dac_wave_stop(void);

static void dac_wave_start(const uint16_t *buf)
{
    dac_wave_stop();

    (void)HAL_DAC_Start_DMA(&g_dac_handle, DAC_CHANNEL_1, (const uint32_t *)buf,
                            DAC_WAVE_SAMPLES, DAC_ALIGN_12B_R);
    (void)HAL_TIM_Base_Start(&g_dac_tim_handle);
}

static void dac_wave_stop(void)
{
    (void)HAL_TIM_Base_Stop(&g_dac_tim_handle);
    (void)HAL_DAC_Stop_DMA(&g_dac_handle, DAC_CHANNEL_1);
}

void dac_triangle_init(uint16_t arr, uint16_t psc)
{
    /* ---- MSP begin: clocks + GPIO ---- */
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM6_CLK_ENABLE();

    dac_gpio_config(DAC_CH1_GPIO_PIN);
    /* ---- MSP end ---- */

    g_dac_handle.Instance = DAC_INSTANCE;
    HAL_DAC_Init(&g_dac_handle);

    dac_wave_dma_init();
    dac_wave_channel_config(DAC_TRIGGER_T6_TRGO);
    dac_wave_timer_init(TIM6, arr, psc);
}

void dac_triangle_start(void)
{
    dac_wave_start(g_dac_triangle_buf);
}

void dac_triangle_stop(void)
{
    dac_wave_stop();
}

void dac_sine_init(uint16_t arr, uint16_t psc)
{
    /* ---- MSP begin: clocks + GPIO ---- */
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM7_CLK_ENABLE();

    dac_gpio_config(DAC_CH1_GPIO_PIN);
    /* ---- MSP end ---- */

    g_dac_handle.Instance = DAC_INSTANCE;
    HAL_DAC_Init(&g_dac_handle);

    dac_wave_dma_init();
    dac_wave_channel_config(DAC_TRIGGER_T7_TRGO);
    dac_wave_timer_init(TIM7, arr, psc);
}

void dac_sine_start(void)
{
    dac_wave_start(g_dac_sine_buf);
}

void dac_sine_stop(void)
{
    dac_wave_stop();
}

void DMA1_Stream5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_dac_dma_handle);
}
