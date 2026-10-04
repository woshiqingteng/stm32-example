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
#include "tim.h"

/* ===== hardware descriptors ===== */

typedef struct
{
    DAC_TypeDef *instance;           /* DAC */
    uint32_t     rcc_en;             /* RCC_APB1ENR_DACEN */
    gpio_hw_t    gpio[DAC_CH_NUM];   /* PA4 / PA5 analog output */
    uint32_t     ch_hal[DAC_CH_NUM]; /* DAC_CHANNEL_1 / DAC_CHANNEL_2 */
    dma_hw_t     dma[DAC_CH_NUM];    /* streaming DMA per channel (dma.irqn unused: no DMA interrupt) */
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

/* Wave trigger timer (TIM6/TIM7) mapped onto the unified tim driver. */
static const tim_id_t g_dac_tim_id[DAC_TIMER_NUM] = { TIM_ID_6, TIM_ID_7 };
static const uint32_t g_dac_trig[DAC_TIMER_NUM]   = { DAC_TRIGGER_T6_TRGO,
                                                      DAC_TRIGGER_T7_TRGO };

/* ===== per-instance state ===== */

typedef struct
{
    const dac_hw_t   *hw;
    dac_cfg_t         cfg;
    DAC_HandleTypeDef dac;
    DMA_HandleTypeDef dma;
    dma_hw_t          dma_cfg; /* runtime working copy */
} dac_handle_t;

static dac_handle_t g_dac;

/* ===== helpers ===== */

static void dac_channel_config(dac_handle_t *h)
{
    DAC_ChannelConfTypeDef channel_config = {0};
    uint32_t               trigger;

    h->dac.Instance = h->hw->instance;
    (void)HAL_DAC_Init(&h->dac);

    if (h->cfg.mode == DAC_MODE_WAVE)
    {
        trigger = g_dac_trig[h->cfg.timer];
    }
    else
    {
        trigger = DAC_TRIGGER_NONE;
    }

    if (h->cfg.buffer_enable)
    {
        channel_config.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    }
    else
    {
        channel_config.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
    }
    channel_config.DAC_Trigger = trigger;

    (void)HAL_DAC_ConfigChannel(&h->dac, &channel_config, h->hw->ch_hal[h->cfg.channel]);
    (void)HAL_DAC_Start(&h->dac, h->hw->ch_hal[h->cfg.channel]);
}

static void dac_timer_config(dac_handle_t *h)
{
    tim_cfg_t tcfg = { TIM_CFG_DEFAULT };

    tcfg.id   = g_dac_tim_id[h->cfg.timer];
    tcfg.mode = TIM_MODE_BASE;
    tcfg.arr  = h->cfg.arr;
    tcfg.psc  = h->cfg.psc;
    tim_init(&tcfg);   /* BASE sets TRGO=UPDATE and starts the timer */
}

/* ===== public API ===== */

void dac_init(const dac_cfg_t *cfg)
{
    static const dac_cfg_t cfg_default = { DAC_CFG_DEFAULT };
    const dac_cfg_t *c = (cfg != 0) ? cfg : &cfg_default;
    dac_handle_t    *h = &g_dac;

    if ((c->channel >= DAC_CH_NUM) ||
        ((c->mode != DAC_MODE_SW) && (c->mode != DAC_MODE_WAVE)))
    {
        return; /* invalid channel or mode */
    }
    if ((c->mode == DAC_MODE_WAVE) && (c->timer >= DAC_TIMER_NUM))
    {
        return; /* timer is selected in wave mode only */
    }

    /* Reconfiguring while a wave is running is not supported: call dac_stop()
     * first (HAL_DAC_ConfigChannel would rewrite TSEL/TEN with EN set). */
    h->hw  = &g_dac_hw;
    h->cfg = *c;

    SET_BIT(RCC->APB1ENR, h->hw->rcc_en);
    gpio_hw_setup(&h->hw->gpio[c->channel]);
    dac_channel_config(h);

    if (c->mode == DAC_MODE_WAVE)
    {
        dac_timer_config(h);
    }
    /* DAC_MODE_SW: software output only, no timer/DMA. */
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

    if (h->cfg.channel == DAC_CH1)
    {
        dmaen = DAC_CR_DMAEN1;
        dhr   = (uint32_t)&h->hw->instance->DHR12R1;
    }
    else
    {
        dmaen = DAC_CR_DMAEN2;
        dhr   = (uint32_t)&h->hw->instance->DHR12R2;
    }

    /* Generic stream setup (clock + Init + HAL_DMA_Init); the table is circular. */
    h->dma_cfg = h->hw->dma[h->cfg.channel];
    dma_hw_setup(&h->dma, &h->dma_cfg);

    /* Hand-written DAC DMA enable, then start the stream and the trigger timer.
     * MEMORY_TO_PERIPH: source is the caller buffer, destination the DAC register. */
    SET_BIT(h->hw->instance->CR, dmaen);
    if (HAL_DMA_Start(&h->dma, (uint32_t)h->cfg.buf, dhr, h->cfg.len) != HAL_OK)
    {
        CLEAR_BIT(h->hw->instance->CR, dmaen);
        return;
    }
    tim_enable(g_dac_tim_id[h->cfg.timer], true);
}

void dac_stop(void)
{
    dac_handle_t *h = &g_dac;
    uint32_t      dmaen;

    if ((h->hw == 0) || (h->cfg.mode != DAC_MODE_WAVE))
    {
        return;
    }

    if (h->cfg.channel == DAC_CH1)
    {
        dmaen = DAC_CR_DMAEN1;
    }
    else
    {
        dmaen = DAC_CR_DMAEN2;
    }

    tim_enable(g_dac_tim_id[h->cfg.timer], false);
    (void)HAL_DMA_Abort(&h->dma);
    CLEAR_BIT(h->hw->instance->CR, dmaen);
    (void)HAL_DAC_Stop(&h->dac, h->hw->ch_hal[h->cfg.channel]);
}

void dac_write(dac_channel_t channel, uint16_t value)
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
