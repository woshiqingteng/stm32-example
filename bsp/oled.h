/**
 * @file    oled.h
 * @brief   SSD1306 128x64 OLED driver over the 8080 8-bit parallel bus.
 */

#ifndef BSP_OLED_H
#define BSP_OLED_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief  Host interface selection (only the 8080 parallel path is implemented). */
typedef enum
{
    OLED_IF_8080 = 0,
    OLED_IF_SPI  = 1
} oled_if_t;

/** @brief  Selectable ASCII font heights. */
typedef enum
{
    OLED_FONT_6X8  = 8,
    OLED_FONT_8X16 = 16
} oled_font_t;

/** @brief  Initialise the OLED (SSD1306) and clear the screen. */
void oled_init(void);

/** @brief  Clear the whole screen. */
void oled_clear(void);

/** @brief  Turn the display on. */
void oled_display_on(void);

/** @brief  Turn the display off. */
void oled_display_off(void);

/** @brief  Flush the internal frame buffer to the panel. */
void oled_refresh(void);

/** @brief  Draw a string at (x,y). @param size OLED_FONT_6X8 or OLED_FONT_8X16. */
void oled_show_string(uint8_t x, uint8_t y, const char *str, uint8_t size);

/** @brief  Draw a right-aligned number of len digits at (x,y). */
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);

#endif /* BSP_OLED_H */
