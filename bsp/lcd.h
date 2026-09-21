/**
 * @file    lcd.h
 * @brief   RGB screen driver, ported from the vendor example (lcd.c) with the
 *          MCU (SSD1963/FMC) parts removed. The MCU-screen experiment reuses
 *          this RGB driver.
 */

#ifndef BSP_LCD_H
#define BSP_LCD_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "ltdc.h"

/* Common colours. */
#define WHITE           0xFFFF
#define BLACK           0x0000
#define RED             0xF800
#define GREEN           0x07E0
#define BLUE            0x001F
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define CYAN            0x07FF

#define BROWN           0xBC40
#define BRRED           0xFC07
#define GRAY            0x8430
#define DARKBLUE        0x01CF
#define LIGHTBLUE       0x7D7C
#define GRAYBLUE        0x5458
#define LIGHTGREEN      0x841F
#define LGRAY           0xC618
#define LGRAYBLUE       0xA651
#define LBBLUE          0x2B12

/** @brief  Glyph background / leading-zero handling for lcd_show_char() and
 *          lcd_show_xnum(). */
typedef enum
{
    LCD_TEXT_BG_OVERWRITE        = 0, /* fill glyph cells, pad with ' '  */
    LCD_TEXT_BG_OVERWRITE_PAD_ZERO,   /* fill glyph cells, pad with '0'  */
    LCD_TEXT_TRANSPARENT,             /* keep background, pad with ' '   */
    LCD_TEXT_TRANSPARENT_PAD_ZERO     /* keep background, pad with '0'   */
} lcd_text_mode_t;

/** @brief  LCD main parameters. */
typedef struct
{
    uint16_t   width;   /* LCD width  */
    uint16_t   height;  /* LCD height */
    uint16_t   id;      /* LCD id */
    ltdc_dir_t dir;     /* 0 portrait, 1 landscape */
} _lcd_dev;

extern _lcd_dev lcddev;
extern uint32_t g_point_color; /* default point colour */
extern uint32_t g_back_color;  /* default background colour */

/* Scan direction (RGB: only L2R_U2D is meaningful). */
#define L2R_U2D   0
#define DFT_SCAN_DIR L2R_U2D

void lcd_init(void);
void lcd_display_dir(ltdc_dir_t dir);
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);
void lcd_clear(uint16_t color);
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, lcd_text_mode_t mode, uint16_t color);
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, lcd_text_mode_t mode, uint16_t color);
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, const char *p, uint16_t color);

#endif /* BSP_LCD_H */
