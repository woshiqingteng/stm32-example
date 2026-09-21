/**
 * @file    lcd.h
 * @brief   Unified screen entry point.
 *
 * NOTE: the MCU screen (SSD1963 / FMC 8080) driver has been removed. Only the
 * RGB screen (LTDC + SDRAM frame buffer) is supported, so the former MCU-screen
 * experiment (12_tftlcd) now drives the RGB screen as well. This header keeps a
 * single lcd_* API that forwards to the lower-level ltdc driver.
 */

#ifndef BSP_LCD_H
#define BSP_LCD_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "ltdc.h"

/** @brief  Display orientation. */
typedef enum
{
    LCD_DIR_PORTRAIT  = 0,
    LCD_DIR_LANDSCAPE = 1
} lcd_dir_t;

/** @brief  Initialise the RGB screen (LTDC). */
void lcd_init(void);

/** @brief  Detected panel id (0x4384), or 0 when no RGB panel is present. */
uint16_t lcd_get_id(void);

/** @brief  Select the display orientation (portrait rotates the drawing). */
void lcd_display_dir(uint8_t dir);

/** @brief  Fill the whole screen with one colour. */
void lcd_clear(uint16_t color);

/** @brief  Draw a single pixel. */
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);

/** @brief  Fill a rectangle given by opposite corners. */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);

/** @brief  Draw a string. @param size Only 16 (8x16 font) is supported. */
void lcd_show_string(uint16_t x, uint16_t y, const char *str, uint8_t size, uint16_t color);

/** @brief  Draw a number of len digits. @param size Only 16 is supported. */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);

#endif /* BSP_LCD_H */
