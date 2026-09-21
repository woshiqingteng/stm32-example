/**
 * @file    lcd.h
 * @brief   Unified screen driver: MCU screen (SSD1963 over FMC) and RGB screen
 *          (LTDC, forwarded to the lower-level ltdc driver). The attached type
 *          is detected at runtime by lcd_init().
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

/* ---- MCU screen: SSD1963 over FMC (8080 bus, 16-bit) ---- */
#define LCD_FMC_NEX  1U
#define LCD_FMC_AX   18U

#define LCD_SSD_PANEL_HOR 800U
#define LCD_SSD_PANEL_VER 480U

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

#define LCD_BASE ((uint32_t)(0x60000000U | (((1U << LCD_FMC_AX) * 2U) - 2U)))

typedef struct
{
    volatile uint16_t LCD_REG;
    volatile uint16_t LCD_RAM;
} lcd_bus_t;

#define LCD ((lcd_bus_t *)LCD_BASE)

/** @brief  Detect and initialise the attached LCD (MCU or RGB). */
void lcd_init(void);

/** @brief  Detected id: 0x1963 (SSD1963) or 0x4384 (RGB panel), 0 if none. */
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
