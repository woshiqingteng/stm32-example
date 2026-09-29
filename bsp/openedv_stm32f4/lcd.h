/**
 * @file    lcd.h
 * @brief   RGB panel public API: geometry, pixels and text.
 */

#ifndef BSP_LCD_H
#define BSP_LCD_H

#include <stdint.h>

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

/** @brief  Glyph background / leading-zero handling for lcd_show_char(). */
typedef enum
{
    LCD_TEXT_BG_OVERWRITE          = 0, /* fill glyph cells, pad with ' '  */
    LCD_TEXT_BG_OVERWRITE_PAD_ZERO = 1, /* fill glyph cells, pad with '0'  */
    LCD_TEXT_TRANSPARENT           = 2, /* keep background, pad with ' '   */
    LCD_TEXT_TRANSPARENT_PAD_ZERO  = 3  /* keep background, pad with '0'   */
} lcd_text_mode_t;

/** @brief  Selectable ASCII font heights (pixels). */
typedef enum
{
    LCD_FONT_SIZE_12 = 12,
    LCD_FONT_SIZE_16 = 16,
    LCD_FONT_SIZE_24 = 24,
    LCD_FONT_SIZE_32 = 32
} lcd_font_size_t;

/** @brief  Panel orientation. */
typedef enum
{
    LCD_DIR_PORTRAIT  = 0,
    LCD_DIR_LANDSCAPE = 1
} lcd_dir_t;

/** @brief  Aggregated panel info (filled by lcd_init()/lcd_display_dir()). */
typedef struct
{
    uint16_t  pwidth;    /* native panel width  */
    uint16_t  pheight;   /* native panel height */
    uint16_t  width;     /* logical width  */
    uint16_t  height;    /* logical height */
    uint16_t  id;        /* panel id */
    lcd_dir_t dir;       /* orientation */
    uint8_t   pixsize;   /* bytes per pixel */
    uint32_t  framebuf;  /* active layer frame buffer address */
} lcd_info_t;

void              lcd_init(void);
void              lcd_display_dir(lcd_dir_t dir);
const lcd_info_t *lcd_info(void);

void              lcd_set_back_color(uint32_t color);
uint32_t          lcd_get_back_color(void);

void              lcd_clear(uint32_t color);
void              lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);
uint32_t          lcd_read_point(uint16_t x, uint16_t y);
void              lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void              lcd_blit(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, const uint16_t *src);

void              lcd_show_char(uint16_t x, uint16_t y, char chr, lcd_font_size_t size, lcd_text_mode_t mode, uint32_t color);
void              lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, lcd_font_size_t size, uint32_t color);
void              lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, lcd_font_size_t size, const char *p, uint32_t color);

#endif /* BSP_LCD_H */
