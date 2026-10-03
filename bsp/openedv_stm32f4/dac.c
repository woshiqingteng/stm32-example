/**
 * @file    dac.c
 * @brief   DAC1 driver: software-triggered channel output and timer-triggered
 *          DMA waveform playback.
 *
 * All hardware facts live in the static dac_hw_t/dac_tim_hw_t tables; per-instance
 * state is held in dac_handle_t. GPIO/DMA come from the generic gpio_hw/dma_hw
 * helpers; the DAC DMA enable is written explicitly (no HAL_DAC_Start_DMA) and no
 * DMA interrupt is used.
 */

#include "stm32f4xx_hal.h"
#include "dac.h"
#include "gpio_hw.h"
#include "dma_hw.h"

/* ===== hardware descriptors ===== */

typedef struct
{
    DAC_TypeDef *instance;           /* DAC */
    uint32_t     rcc_en;             /* RCC_APB1ENR_DACEN */
    gpio_hw_t    gpio[DAC_CH_NUM];   /* PA4 / PA5 analog output */
    uint32_t     ch_hal[DAC_CH_NUM]; /* DAC_CHANNEL_1 / DAC_CHANNEL_2 */
    dma_hw_t     dma[DAC_CH_NUM];    /* streaming DMA per channel */
} dac_hw_t;

static const dac_hw_t g_dac_hw =
{
    .instance = DAC,
    .rcc_en   = RCC_APB1ENR_DACEN,
    .gpio     = {
        { GPIOA, RCC_AHB1ENR_GPIOAEN, GPIO_PIN_4, GPIO_MODE_ANALOG, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0U },
        { GPIOA, RCC_AHB1ENR_GPIOAEN, GPIO_PIN_5, GPIO_MODE_ANALOG, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0U },
    },
    .ch_hal   = { DAC_CHANNEL_1, DAC_CHANNEL_2 },
    .dma      = {
        {
            .rcc_en         = RCC_AHB1ENR_DMA1EN,
            .stream         = DMA1_Stream5,
            .irqn           = DMA1_Stream5_IRQn,
            .channel        = DMA_CHANNEL_7,
            .direction      = DMA_MEMORY_TO_PERIPH,
            .periph_inc     = DMA_PINC_DISABLE,
            .mem_inc        = DMA_MINC_ENABLE,
            .periph_align   = DMA_PDATAALIGN_HALFWORD,
            .mem_align      = DMA_MDATAALIGN_HALFWORD,
            .mode           = DMA_CIRCULAR,
            .priority       = DMA_PRIORITY_MEDIUM,
            .fifo_mode      = DMA_FIFOMODE_DISABLE,
            .fifo_threshold = DMA_FIFO_THRESHOLD_1QUARTERFULL,
            .mem_burst      = DMA_MBURST_SINGLE,
            .periph_burst   = DMA_PBURST_SINGLE,
        },
        {
            .rcc_en         = RCC_AHB1ENR_DMA1EN,
            .stream         = DMA1_Stream6,
            .irqn           = DMA1_Stream6_IRQn,
            .channel        = DMA_CHANNEL_7,
            .direction      = DMA_MEMORY_TO_PERIPH,
            .periph_inc     = DMA_PINC_DISABLE,
            .mem_inc        = DMA_MINC_ENABLE,
            .periph_align   = DMA_PDATAALIGN_HALFWORD,
            .mem_align      = DMA_MDATAALIGN_HALFWORD,
            .mode           = DMA_CIRCULAR,
            .priority       = DMA_PRIORITY_MEDIUM,
            .fifo_mode      = DMA_FIFOMODE_DISABLE,
            .fifo_threshold = DMA_FIFO_THRESHOLD_1QUARTERFULL,
            .mem_burst      = DMA_MBURST_SINGLE,
            .periph_burst   = DMA_PBURST_SINGLE,
        },
    },
};

typedef struct
{
    TIM_TypeDef *instance; /* TIM6 / TIM7 */
    uint32_t     rcc_en;   /* RCC_APB1ENR_TIM6EN / TIM7EN */
    uint32_t     trig_hal; /* DAC_TRIGGER_T6_TRGO / T7_TRGO */
} dac_tim_hw_t;

static const dac_tim_hw_t g_dac_tim[DAC_TIMER_NUM] =
{
    { TIM6, RCC_APB1ENR_TIM6EN, DAC_TRIGGER_T6_TRGO },
    { TIM7, RCC_APB1ENR_TIM7EN, DAC_TRIGGER_T7_TRGO },
};

/* ===== per-instance state ===== */

typedef struct
{
    const dac_hw_t   *hw;
    dac_cfg_t         cfg;
    DAC_HandleTypeDef dac;
    DMA_HandleTypeDef dma;
    dma_hw_t          dma_cfg; /* runtime working copy */
    TIM_HandleTypeDef tim;
} dac_handle_t;

static dac_handle_t g_dac;

/* ===== helpers ===== */

static void dac_channel_config(dac_handle_t *h)
{
    DAC_ChannelConfTypeDef channel_config = {0};
    uint32_t               trigger;

    h->dac.Instance = h->hw->instance;
    (void)HAL_DAC_Init(&h->dac);

    trigger = (h->cfg.mode == DAC_MODE_WAVE) ? g_dac_tim[h->cfg.timer].trig_hal
                                             : DAC_TRIGGER_NONE;

    channel_config.DAC_Trigger      = trigger;
    channel_config.DAC_OutputBuffer = h->cfg.buffer_enable ? DAC_OUTPUTBUFFER_ENABLE
                                                           : DAC_OUTPUTBUFFER_DISABLE;
    (void)HAL_DAC_ConfigChannel(&h->dac, &channel_config, h->hw->ch_hal[h->cfg.channel]);
    (void)HAL_DAC_Start(&h->dac, h->hw->ch_hal[h->cfg.channel]);
}

static void dac_timer_config(dac_handle_t *h)
{
    TIM_MasterConfigTypeDef master = {0};
    const dac_tim_hw_t     *tim    = &g_dac_tim[h->cfg.timer];

    SET_BIT(RCC->APB1ENR, tim->rcc_en);

    h->tim.Instance               = tim->instance;
    h->tim.Init.Prescaler         = h->cfg.psc;
    h->tim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    h->tim.Init.Period            = h->cfg.arr;
    h->tim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    h->tim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    (void)HAL_TIM_Base_Init(&h->tim);

    master.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    (void)HAL_TIMEx_MasterConfigSynchronization(&h->tim, &master);
}

/* ===== public API ===== */

void dac_init(const dac_cfg_t *cfg)
{
    static const dac_cfg_t cfg_default = { DAC_CFG_DEFAULT };
    const dac_cfg_t *c = (cfg != 0) ? cfg : &cfg_default;
    dac_handle_t    *h = &g_dac;

    if ((c->channel >= DAC_CH_NUM) || (c->timer >= DAC_TIMER_NUM))
    {
        return;
    }
    h->hw  = &g_dac_hw;
    h->cfg = *c;

    SET_BIT(RCC->APB1ENR, h->hw->rcc_en);
    gpio_hw_setup(&h->hw->gpio[c->channel]);
    dac_channel_config(h);

    if (c->mode == DAC_MODE_WAVE)
    {
        dac_timer_config(h);
    }
}

void dac_start(void)
{
    dac_handle_t *h = &g_dac;
    uint32_t      dmaen;
    uint32_t      dhr;

    if ((h->hw == 0) || (h->cfg.mode != DAC_MODE_WAVE) ||
        (h->cfg.buf == 0) || (h->cfg.len == 0U))
    {
        return;
    }

    dmaen = (h->cfg.channel == DAC_CH1) ? DAC_CR_DMAEN1 : DAC_CR_DMAEN2;
    dhr   = (h->cfg.channel == DAC_CH1) ? (uint32_t)&h->hw->instance->DHR12R1
                                        : (uint32_t)&h->hw->instance->DHR12R2;

    /* Generic stream setup (clock + Init + HAL_DMA_Init); mode is circular. */
    h->dma_cfg = h->hw->dma[h->cfg.channel];
    h->dma_cfg.mode = DMA_CIRCULAR;
    dma_hw_setup(&h->dma, &h->dma_cfg);

    /* Hand-written DAC DMA enable, then start the stream and the trigger timer.
     * MEMORY_TO_PERIPH: source is the caller buffer, destination the DAC register. */
    SET_BIT(h->hw->instance->CR, dmaen);
    if (HAL_DMA_Start(&h->dma, (uint32_t)h->cfg.buf, dhr, h->cfg.len) != HAL_OK)
    {
        CLEAR_BIT(h->hw->instance->CR, dmaen);
        return;
    }
    (void)HAL_TIM_Base_Start(&h->tim);
}

void dac_stop(void)
{
    dac_handle_t *h = &g_dac;
    uint32_t      dmaen;

    if ((h->hw == 0) || (h->cfg.mode != DAC_MODE_WAVE))
    {
        return;
    }

    dmaen = (h->cfg.channel == DAC_CH1) ? DAC_CR_DMAEN1 : DAC_CR_DMAEN2;

    (void)HAL_TIM_Base_Stop(&h->tim);
    (void)HAL_DMA_Abort(&h->dma);
    CLEAR_BIT(h->hw->instance->CR, dmaen);
    (void)HAL_DAC_Stop(&h->dac, h->hw->ch_hal[h->cfg.channel]);
}

void dac_set(dac_channel_t channel, uint16_t value)
{
    if ((g_dac.hw == 0) || (channel >= DAC_CH_NUM))
    {
        return;
    }
    if (value > DAC_FULL_SCALE_COUNT)
    {
        value = DAC_FULL_SCALE_COUNT;
    }
    (void)HAL_DAC_SetValue(&g_dac.dac, g_dac.hw->ch_hal[channel], DAC_ALIGN_12B_R, value);
}

void dac_set_voltage(dac_channel_t channel, uint16_t millivolt)
{
    uint32_t code;

    if (millivolt > DAC_VREF_MV)
    {
        millivolt = DAC_VREF_MV;
    }
    code = ((uint32_t)millivolt * (DAC_FULL_SCALE_COUNT + 1U)) / DAC_VREF_MV;
    dac_set(channel, (uint16_t)code);
}
