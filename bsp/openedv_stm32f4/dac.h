/**
 * @file    dac.h
 * @brief   DAC1 driver: software-triggered channel output and timer-triggered
 *          DMA triangle/sine wave generation.
 */

#ifndef BSP_DAC_H
#define BSP_DAC_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief DAC1 output channels: PA4 = channel 1, PA5 = channel 2. */
typedef enum
{
    DAC_CH1 = 1,
    DAC_CH2 = 2,
} dac_channel_t;

/** @brief 12-bit full-scale DAC code. */
#define DAC_FULL_SCALE 4095U

/** @brief  Initialise DAC1 channels 1 and 2 for software-triggered output. */
void dac_init(void);

/** @brief  Write a 12-bit right-aligned value to a channel (DAC_CH1/DAC_CH2). */
void dac_set(uint32_t channel, uint16_t value);

/** @brief  Write an output voltage (0..3300 mV) to a channel. */
void dac_set_voltage(uint32_t channel, uint16_t millivolt);

/** @brief  Configure DAC1 channel 1 triangle output driven by TIM6 TRGO + DMA. */
void dac_triangle_init(uint16_t arr, uint16_t psc);

/** @brief  Start the triangle wave configured by dac_triangle_init(). */
void dac_triangle_start(void);

/** @brief  Stop the triangle wave. */
void dac_triangle_stop(void);

/** @brief  Configure DAC1 channel 1 sine output driven by TIM7 TRGO + DMA. */
void dac_sine_init(uint16_t arr, uint16_t psc);

/** @brief  Start the sine wave configured by dac_sine_init(). */
void dac_sine_start(void);

/** @brief  Stop the sine wave. */
void dac_sine_stop(void);

#endif /* BSP_DAC_H */
