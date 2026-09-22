/**
 * @file    ov5640.h
 * @brief   OV5640 5 MP camera sensor driver (SCCB control + DVP output).
 *
 * The control channel is a 2-wire SCCB bus that is bit-banged on PB4 (SCL) and
 * PB3 (SDA). This is the camera header bus and is separate from the board IIC
 * bus (PH4/PH5) used by the EEPROM / PCF8574. The sensor reset line is PA15.
 */

#ifndef BSP_OV5640_H
#define BSP_OV5640_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/* Reset pin. */
#define OV5640_RESET_GPIO_PORT   GPIOA
#define OV5640_RESET_GPIO_PIN    GPIO_PIN_15

/* SCCB pins. */
#define OV5640_SCL_GPIO_PORT     GPIOB
#define OV5640_SCL_GPIO_PIN      GPIO_PIN_4
#define OV5640_SDA_GPIO_PORT     GPIOB
#define OV5640_SDA_GPIO_PIN      GPIO_PIN_3

#define OV5640_CHIPID            0x5640U
#define OV5640_SCCB_ADDR         0x78U

/* Register addresses. */
#define OV5640_CHIPIDH           0x300AU
#define OV5640_CHIPIDL           0x300BU

/** @brief  Read a 16-bit sensor register over SCCB. */
uint8_t ov5640_read_reg(uint16_t reg);

/** @brief  Write a 16-bit sensor register over SCCB (0: ok, 1: no ack). */
uint8_t ov5640_write_reg(uint16_t reg, uint8_t data);

/** @brief  Drive the camera power-down line through the PCF8574. */
void ov5640_pwdn_set(uint8_t sta);

/** @brief  Read the 16-bit chip id (0x5640 for OV5640). */
uint16_t ov5640_read_id(void);

/** @brief  Reset, probe and load the default register table (0: ok). */
uint8_t ov5640_init(void);

void ov5640_jpeg_mode(void);
void ov5640_rgb565_mode(void);

void ov5640_exposure(uint8_t exposure);
void ov5640_light_mode(uint8_t mode);
void ov5640_color_saturation(uint8_t sat);
void ov5640_brightness(uint8_t bright);
void ov5640_contrast(uint8_t contrast);
void ov5640_sharpness(uint8_t sharp);
void ov5640_special_effects(uint8_t eft);
void ov5640_test_pattern(uint8_t mode);
void ov5640_flash_ctrl(uint8_t sw);

/** @brief  Set the scaled output (DVPHO/DVPVO) size and its crop offset. */
uint8_t ov5640_outsize_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height);

/** @brief  Set the ISP crop window (full resolution source rectangle). */
uint8_t ov5640_image_window_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height);

/** @brief  Load the auto-focus firmware and start continuous focusing. */
uint8_t ov5640_focus_init(void);
uint8_t ov5640_focus_single(void);
uint8_t ov5640_focus_constant(void);

#endif /* BSP_OV5640_H */
