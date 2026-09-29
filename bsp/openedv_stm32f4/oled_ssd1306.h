/**
 * @file    oled_ssd1306.h
 * @brief   SSD1306 128x64 controller protocol over a selectable bus.
 */

#ifndef BSP_OLED_SSD1306_H
#define BSP_OLED_SSD1306_H

#include <stdint.h>

/* Transport: edit OLED_BUS below (or override with -DOLED_BUS=...). Default 8080. */
#define OLED_BUS_8080 0
#define OLED_BUS_SPI  1
#define OLED_BUS_I2C  2
#ifndef OLED_BUS
#define OLED_BUS OLED_BUS_8080
#endif

/** @brief  Initialise the bus and send the SSD1306 init sequence. */
void oled_ssd1306_init(void);

void oled_ssd1306_display_on(void);
void oled_ssd1306_display_off(void);

/**
 * @brief  Flush a column-major frame buffer: gram[col * pages + page].
 * @param  gram   Frame buffer (8 vertical pixels per byte).
 * @param  width  Panel width in columns.
 * @param  pages  Number of 8-pixel pages.
 */
void oled_ssd1306_flush(const uint8_t *gram, uint8_t width, uint8_t pages);

#endif /* BSP_OLED_SSD1306_H */
