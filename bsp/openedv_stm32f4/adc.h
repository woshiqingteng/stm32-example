/**
 * @file    adc.h
 * @brief   ADC1 interface: polled channel read and single/scan DMA acquisition.
 */

#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief ADC1 regular channels routed to PA0..PA5. */
typedef enum
{
    ADC_CH0 = ADC_CHANNEL_0, /*!< PA0 */
    ADC_CH1 = ADC_CHANNEL_1, /*!< PA1 */
    ADC_CH2 = ADC_CHANNEL_2, /*!< PA2 */
    ADC_CH3 = ADC_CHANNEL_3, /*!< PA3 */
    ADC_CH4 = ADC_CHANNEL_4, /*!< PA4 */
    ADC_CH5 = ADC_CHANNEL_5, /*!< PA5 (single-channel input) */
    ADC_SCAN_CH_NUM = 6,     /*!< channels sampled by the scan DMA mode */
    ADC_TEMP_CH = ADC_CHANNEL_18, /*!< internal temperature sensor (ADC1 only) */
} adc_channel_t;

/** @brief Regular-group sampling time used by this driver. */
typedef enum
{
    ADC_SAMPLE_TIME = ADC_SAMPLETIME_480CYCLES,
} adc_sample_time_t;

/** @brief Completion callback invoked from the ADC DMA transfer-complete ISR. */
typedef void (*adc_dma_cb_t)(void);

/** @brief  Initialise ADC1 for polled single-channel conversion. */
void adc_init(void);

/** @brief  Poll one regular channel and return its 12-bit result. */
uint32_t adc_get_result(adc_channel_t channel);

/** @brief  Average `times` polled samples of one channel. */
uint32_t adc_get_result_average(adc_channel_t channel, uint8_t times);

/** @brief  Configure ADC1 + DMA2_Stream4 for single-channel acquisition. */
void adc_dma_init(uint16_t *buf, uint16_t len);

/** @brief  (Re-)arm a single-channel DMA acquisition of `len` samples. */
void adc_dma_start(uint16_t len);

/** @brief  Configure ADC1 + DMA2_Stream4 for 6-channel scan acquisition. */
void adc_scan_dma_init(uint16_t *buf, uint16_t len);

/** @brief  (Re-)arm a 6-channel scan DMA acquisition of `len` samples. */
void adc_scan_dma_start(uint16_t len);

/** @brief  Register the DMA transfer-complete hook (0 clears it). */
void adc_register_dma_hook(adc_dma_cb_t cb);

/** @brief  Enable the internal temperature sensor path on ADC1. */
void adc_temp_init(void);

/** @brief  Read the internal temperature sensor and return temperature * 100. */
int16_t adc_get_temperature(void);

#endif /* BSP_ADC_H */
