/**
 * @file    lcd.h
 * @brief   MCU TFT-LCD (SSD1963, native 800x480) driver on the FMC 8080 bus.
 */

#ifndef BSP_LCD_H
#define BSP_LCD_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief  Common RGB565 colours. */
typedef enum
{
    WHITE      = 0xFFFF,
    BLACK      = 0x0000,
    RED        = 0xF800,
    GREEN      = 0x07E0,
    BLUE       = 0x001F,
    MAGENTA    = 0xF81F,
    YELLOW     = 0xFFE0,
    CYAN       = 0x07FF,
    BROWN      = 0xBC40,
    BRRED      = 0xFC07,
    GRAY       = 0x8430,
    DARKBLUE   = 0x01CF,
    LIGHTBLUE  = 0x7D7C,
    GRAYBLUE   = 0x5458,
    LIGHTGREEN = 0x841F,
    LGRAY      = 0xC618,
    LGRAYBLUE  = 0xA651,
    LBBLUE     = 0x2B12
} lcd_color_t;

/* FMC chip select and RS address line. */
#define LCD_FMC_NEX  1U
#define LCD_FMC_AX   18U

/* SSD1963 panel native raster (fixed by the panel). */
#define LCD_SSD_PANEL_HOR 800U
#define LCD_SSD_PANEL_VER 480U

/* Horizontal/vertical timing (openedv SSD1963 defaults). */
#define LCD_SSD_HOR_PULSE_WIDTH 1U
#define LCD_SSD_HOR_BACK_PORCH  46U
#define LCD_SSD_HOR_FRONT_PORCH 210U
#define LCD_SSD_VER_PULSE_WIDTH 1U
#define LCD_SSD_VER_BACK_PORCH  23U
#define LCD_SSD_VER_FRONT_PORCH 22U

#define LCD_SSD_HT  (LCD_SSD_PANEL_HOR + LCD_SSD_HOR_BACK_PORCH + LCD_SSD_HOR_FRONT_PORCH)
#define LCD_SSD_HPS (LCD_SSD_HOR_BACK_PORCH)
#define LCD_SSD_VT  (LCD_SSD_PANEL_VER + LCD_SSD_VER_BACK_PORCH + LCD_SSD_VER_FRONT_PORCH)
#define LCD_SSD_VPS (LCD_SSD_VER_BACK_PORCH)

/* 8080 bus window into FMC bank1 (NE1) with RS on A18. */
#define LCD_BASE ((uint32_t)(0x60000000U | (((1U << LCD_FMC_AX) * 2U) - 2U)))

typedef struct
{
    volatile uint16_t LCD_REG;
    volatile uint16_t LCD_RAM;
} lcd_bus_t;

#define LCD ((lcd_bus_t *)LCD_BASE)

/** @brief  Initialise the FMC bus, SSD1963 controller and backlight. */
void lcd_init(void);

/** @brief  Return the detected controller id (0x1963 for SSD1963). */
uint16_t lcd_get_id(void);

/** @brief  Select the display orientation (0 portrait, 1 landscape). */
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
