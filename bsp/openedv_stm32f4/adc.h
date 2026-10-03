/**
 * @file    adc.h
 * @brief   ADC1 pure driver: polled single read and DMA acquisition.
 *          This interface is HAL-free (own enums); the HAL is used only inside
 *          adc.c for peripheral/stream initialisation. No averaging, voltage or
 *          temperature conversion lives here.
 */

#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdint.h>
#include <stdbool.h>

/** @brief ADC1 regular-channel id (driver value, not the HAL ADC_CHANNEL_x macro). */
typedef enum
{
    ADC_CH0 = 0, /*!< PA0 */
    ADC_CH1,     /*!< PA1 */
    ADC_CH2,     /*!< PA2 */
    ADC_CH3,     /*!< PA3 */
    ADC_CH4,     /*!< PA4 */
    ADC_CH5,     /*!< PA5 */
    ADC_TEMP_CH, /*!< internal temperature sensor */
    ADC_CH_NUM
} adc_channel_t;

#define ADC_SCAN_CH_NUM 6U /*!< external channels used by the scan DMA mode */

/** @brief Acquisition transport. */
typedef enum
{
    ADC_MODE_POLL = 0, /*!< polled single read (adc_read) */
    ADC_MODE_DMA       /*!< DMA acquisition (adc_dma_start / dma_cb) */
} adc_mode_t;

/** @brief DMA buffer handling. */
typedef enum
{
    ADC_DMA_ONESHOT = 0, /*!< stop after one block; re-arm with adc_dma_start() */
    ADC_DMA_CIRCULAR     /*!< free-running; never needs re-arming */
} adc_dma_mode_t;

/** @brief Resolution. */
typedef enum
{
    ADC_RES_12B = 0, /*!< 12-bit */
    ADC_RES_10B,     /*!< 10-bit */
    ADC_RES_8B,      /*!< 8-bit */
    ADC_RES_6B       /*!< 6-bit */
} adc_resolution_t;

/** @brief Regular-group sampling time. */
typedef enum
{
    ADC_SAMPLE_3C = 0, /*!< 3 cycles */
    ADC_SAMPLE_15C,    /*!< 15 cycles */
    ADC_SAMPLE_28C,    /*!< 28 cycles */
    ADC_SAMPLE_56C,    /*!< 56 cycles */
    ADC_SAMPLE_84C,    /*!< 84 cycles */
    ADC_SAMPLE_112C,   /*!< 112 cycles */
    ADC_SAMPLE_144C,   /*!< 144 cycles */
    ADC_SAMPLE_480C    /*!< 480 cycles */
} adc_sample_time_t;

/** @brief ADC clock prescaler (PCLK2/x). */
typedef enum
{
    ADC_CLK_DIV2 = 0, /*!< PCLK2 / 2 */
    ADC_CLK_DIV4,     /*!< PCLK2 / 4 */
    ADC_CLK_DIV6,     /*!< PCLK2 / 6 */
    ADC_CLK_DIV8      /*!< PCLK2 / 8 */
} adc_clock_t;

/** @brief DMA block completion (interrupt context).
 *  @param offset first sample of the finished block: 0 (half) or dma_len/2 (full). */
typedef void (*adc_dma_cb_t)(uint16_t offset);

/** @brief One-shot ADC configuration. */
typedef struct
{
    adc_mode_t           mode;          /*!< poll or DMA */
    adc_dma_mode_t       dma_mode;      /*!< valid when mode == ADC_MODE_DMA */
    adc_resolution_t     resolution;
    adc_sample_time_t    sample_time;
    adc_clock_t          clock;

    const adc_channel_t *chans;         /*!< DMA sequence (mode == DMA) */
    uint8_t              nchans;        /*!< 1 = single, 6 = scan */
    uint16_t            *dma_buf;
    uint16_t             dma_len;       /*!< samples in dma_buf = depth * nchans */
    bool                 dma_half_cb;   /*!< also report the half-transfer block */
    adc_dma_cb_t         dma_cb;
} adc_cfg_t;

/** @brief 12-bit, 480 cycles, PCLK/4, poll mode, no DMA. */
#define ADC_CFG_DEFAULT \
    .mode = ADC_MODE_POLL, .dma_mode = ADC_DMA_ONESHOT, \
    .resolution = ADC_RES_12B, .sample_time = ADC_SAMPLE_480C, .clock = ADC_CLK_DIV4, \
    .chans = 0, .nchans = 0U, .dma_buf = 0, .dma_len = 0U, \
    .dma_half_cb = false, .dma_cb = 0

/**
 * @brief  Initialise ADC1 according to @p cfg (NULL selects ADC_CFG_DEFAULT).
 *         In DMA mode the acquisition is configured and started once.
 */
void adc_init(const adc_cfg_t *cfg);

/** @brief Poll one conversion and return its raw result. */
uint32_t adc_read(adc_channel_t ch);

/** @brief Arm one DMA block (ONESHOT re-arm); CIRCULAR needs this only once. */
void adc_dma_start(void);

#endif /* BSP_ADC_H */
