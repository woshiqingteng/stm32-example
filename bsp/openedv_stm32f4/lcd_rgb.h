/**
 * @file    lcd_rgb.h
 * @brief   RGB panel driver: panel id detection and per-panel timing/PLL.
 */

#ifndef BSP_LCD_RGB_H
#define BSP_LCD_RGB_H

#include <stdint.h>

/** @brief  Supported panel: 4.3 inch, native 800x480 raster (id 0x4384). */
#define LCD_PANEL_ID_4384     0x4384U
#define LCD_PANEL_WIDTH_PX    800U
#define LCD_PANEL_HEIGHT_PX   480U
#define LCD_PANEL_IDX_4384    4U

/* Panel raster timing. */
#define LCD_PANEL_HSW         48U
#define LCD_PANEL_HBP         88U
#define LCD_PANEL_HFP         40U
#define LCD_PANEL_VSW         3U
#define LCD_PANEL_VBP         32U
#define LCD_PANEL_VFP         13U

/* LTDC pixel clock PLL. */
#define LCD_PANEL_PLLSAIN_RAW     396U
#define LCD_PANEL_PLLSAIR_RAW     3U
#define LCD_PANEL_PLLSAIDIVR_RAW  RCC_PLLSAIDIVR_4

/* Panel id strap bit positions. */
#define LCD_PANEL_IDX_SHIFT_0 0U
#define LCD_PANEL_IDX_SHIFT_1 1U
#define LCD_PANEL_IDX_SHIFT_2 2U

/** @brief  RGB panel configuration. */
typedef struct
{
    uint16_t id;
    uint32_t pwidth;
    uint32_t pheight;
    uint16_t hsw;
    uint16_t vsw;
    uint16_t hbp;
    uint16_t vbp;
    uint16_t hfp;
    uint16_t vfp;
    uint32_t pllsain;
    uint32_t pllsair;
    uint32_t pllsaidivr;
    uint32_t pcpolarity;
} lcd_rgb_cfg_t;

/** @brief  Read the panel id and return the matching config, or 0 if none. */
const lcd_rgb_cfg_t *lcd_rgb_probe(void);

#endif /* BSP_LCD_RGB_H */
