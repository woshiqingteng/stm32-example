/**
 * @file    ltdc.h
 * @brief   RGB LCD driver using LTDC with an SDRAM frame buffer.
 */

#ifndef BSP_LTDC_H
#define BSP_LTDC_H

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
} ltdc_color_t;

/** @brief  Frame buffer base address inside the on-board SDRAM. */
#define LTDC_FRAME_BUF_ADDR 0xC0000000UL

/* Supported panel: 4.3 inch, native 800x480 raster (panel id 0x4384). */
#define LTDC_PANEL_ID_4384  0x4384U
#define LTDC_PANEL_WIDTH    800U
#define LTDC_PANEL_HEIGHT   480U

/** @brief  Display orientation. */
typedef enum
{
    LTDC_DIR_PORTRAIT  = 0,
    LTDC_DIR_LANDSCAPE = 1
} ltdc_dir_t;

/** @brief  Read the panel id from the LCD RGB lines (PG6/PI2/PI7). */
uint16_t ltdc_panelid_read(void);

/** @brief  Configure the LTDC pixel clock via PLLSAI. @return 0 on success. */
uint8_t ltdc_clk_set(uint32_t pllsain, uint32_t pllsair, uint32_t pllsaidivr);

/** @brief  Initialise the LTDC controller and start scanning the frame buffer. */
void ltdc_init(void);

/** @brief  Select the display orientation (portrait rotates the drawing). */
void ltdc_display_dir(uint8_t dir);

/** @brief  Fill the whole screen with one colour. */
void ltdc_clear(uint16_t color);

/** @brief  Fill a rectangle given by opposite corners. */
void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);

/** @brief  Draw a single pixel in the frame buffer. */
void ltdc_draw_point(uint16_t x, uint16_t y, uint16_t color);

/** @brief  Draw a string. @param size Only 16 (8x16 font) is supported. */
void ltdc_show_string(uint16_t x, uint16_t y, const char *str, uint8_t size, uint16_t color);

/** @brief  Draw a number of len digits. @param size Only 16 is supported. */
void ltdc_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);

#endif /* BSP_LTDC_H */
