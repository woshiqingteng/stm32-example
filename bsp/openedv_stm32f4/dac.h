/**
 * @file    dac.h
 * @brief   DAC1 pure driver: software-triggered output and timer-triggered DMA
 *          waveform playback. This interface is HAL-free (own enums); the HAL is
 *          used only inside dac.c for peripheral/stream initialisation. The
 *          waveform buffer is supplied by the caller.
 */

#ifndef BSP_DAC_H
#define BSP_DAC_H

#include <stdint.h>
#include <stdbool.h>

/** @brief DAC1 output channel (driver value, not the HAL DAC_CHANNEL_x macro). */
typedef enum
{
    DAC_CH1 = 0, /*!< PA4 */
    DAC_CH2,     /*!< PA5 */
    DAC_CH_NUM
} dac_channel_t;

/** @brief Output mode. */
typedef enum
{
    DAC_MODE_SW = 0, /*!< software-triggered single value (dac_write) */
    DAC_MODE_WAVE    /*!< timer-triggered DMA waveform (dac_start/dac_stop) */
} dac_mode_t;

/** @brief Timer used to pace the wave DMA (mode == DAC_MODE_WAVE). */
typedef enum
{
    DAC_TIMER_6 = 0, /*!< TIM6 TRGO */
    DAC_TIMER_7,     /*!< TIM7 TRGO */
    DAC_TIMER_NUM
} dac_timer_t;

/** @brief 12-bit full-scale DAC code. */
#define DAC_FULL_SCALE_COUNT 4095U

/** @brief One-shot DAC configuration. */
typedef struct
{
    dac_mode_t      mode;           /*!< SW output or timer+DMA wave */
    dac_channel_t   channel;        /*!< output channel */
    bool            buffer_enable;  /*!< output buffer on/off */

    dac_timer_t     timer;          /*!< valid when mode == DAC_MODE_WAVE */
    const uint16_t *buf;            /*!< caller waveform, one period */
    uint16_t        len;            /*!< samples in buf */
    uint16_t        arr;            /*!< timer auto-reload (Period) */
    uint16_t        psc;            /*!< timer prescaler */
} dac_cfg_t;

/** @brief DAC1, channel 1, software mode, output buffer off. */
#define DAC_CFG_DEFAULT \
    .mode = DAC_MODE_SW, .channel = DAC_CH1, .buffer_enable = false, \
    .timer = DAC_TIMER_6, .buf = 0, .len = 0U, .arr = 0U, .psc = 0U

/**
 * @brief  Initialise DAC1 according to @p cfg (NULL selects DAC_CFG_DEFAULT).
 *         Configuration only: a waveform is started by dac_start().
 */
void dac_init(const dac_cfg_t *cfg);

/** @brief Start (or re-arm) the wave configured by dac_init(). No-op in SW mode. */
void dac_start(void);

/** @brief Stop the wave. No-op in SW mode. */
void dac_stop(void);

/** @brief Write a 12-bit right-aligned code to a channel. */
void dac_write(dac_channel_t channel, uint16_t value);

#endif /* BSP_DAC_H */
