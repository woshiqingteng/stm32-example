/**
 * @file    es8388.h
 * @brief   ES8388 audio codec control over the bit-bang IIC bus.
 *
 * The codec is wired to the ALIENTEK F429 board IIC pins (SCL = PH4, SDA =
 * PH5). Volume, DAC/ADC enable, output and input routing and the SAI data
 * format are all configured through the register interface below.
 */

#ifndef BSP_ES8388_H
#define BSP_ES8388_H

#include <stdint.h>

#define ES8388_ADDR     0x10    /* ES8388 device address, fixed at 0x10 */

/** @brief  Reset and configure the codec. @return 0 on success. */
uint8_t es8388_init(void);

/** @brief  Write one register. @return 0 on success, non-zero on IIC error. */
uint8_t es8388_write_reg(uint8_t reg, uint8_t val);

/** @brief  Read one register. @return the register value. */
uint8_t es8388_read_reg(uint8_t reg);

/**
 * @brief  Set the codec serial audio format.
 * @param  fmt 0 = standard I2S, 1 = MSB, 2 = LSB, 3 = PCM/DSP.
 * @param  len 0 = 24-bit, 1 = 20-bit, 2 = 18-bit, 3 = 16-bit, 4 = 32-bit.
 */
void es8388_sai_cfg(uint8_t fmt, uint8_t len);

/** @brief  Set the headphone volume (0 ~ 33). */
void es8388_hpvol_set(uint8_t volume);

/** @brief  Set the speaker volume (0 ~ 33). */
void es8388_spkvol_set(uint8_t volume);

/** @brief  Set the 3D enhancement depth (0 ~ 7, 0 off). */
void es8388_3d_set(uint8_t depth);

/** @brief  Enable/disable the DAC and ADC. @param dacen 1 = on. @param adcen 1 = on. */
void es8388_adda_cfg(uint8_t dacen, uint8_t adcen);

/** @brief  Enable/disable the two DAC output channels. */
void es8388_output_cfg(uint8_t o1en, uint8_t o2en);

/** @brief  Set the MIC PGA gain (0 ~ 8, 3 dB per step). */
void es8388_mic_gain(uint8_t gain);

/** @brief  Configure the automatic level control (ALC). */
void es8388_alc_ctrl(uint8_t sel, uint8_t maxgain, uint8_t mingain);

/** @brief  Select the ADC input channel (0 = channel 1, 1 = channel 2). */
void es8388_input_cfg(uint8_t in);

#endif /* BSP_ES8388_H */
