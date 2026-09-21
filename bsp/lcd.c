/**
 * @file    lcd.c
 * @brief   Unified screen entry point, backed by the RGB (LTDC) driver.
 *
 * NOTE: the MCU screen (SSD1963 / FMC 8080) implementation was removed from the
 * project. All screen experiments, including the former MCU-screen experiment
 * (12_tftlcd), use the RGB screen through this thin forwarding layer.
 */

#include "lcd.h"

void lcd_init(void)
{
    /* ltdc_init() also defaults to portrait and clears the frame buffer. */
    ltdc_init();
}

uint16_t lcd_get_id(void)
{
    return ltdc_panelid_read();
}

void lcd_display_dir(uint8_t dir)
{
    ltdc_display_dir(dir);
}

void lcd_clear(uint16_t color)
{
    ltdc_clear(color);
}

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    ltdc_draw_point(x, y, color);
}

void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    ltdc_fill(sx, sy, ex, ey, color);
}

void lcd_show_string(uint16_t x, uint16_t y, const char *str, uint8_t size, uint16_t color)
{
    ltdc_show_string(x, y, str, size, color);
}

void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    ltdc_show_num(x, y, num, len, size, color);
}
