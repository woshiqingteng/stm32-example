/**
 * @file    lcd_rgb.h
 * @brief   RGB screen driver: panel detection/configuration plus the LTDC
 *          controller (with SDRAM frame buffer) that drives it.
 *
 * Only the 4.3 inch panel (id 0x4384) is implemented; all other panel ids are
 * collapsed into an empty "other" path. Clock/GPIO are initialised inline in
 * ltdc_init() instead of a HAL_LTDC_MspInit().
 */

#ifndef BSP_LCD_RGB_H
#define BSP_LCD_RGB_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

/* ==================== panel ==================== */

/** @brief  Supported panel: 4.3 inch, native 800x480 raster (id 0x4384). */
#define LCD_PANEL_ID_4384     0x4384U
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

/* ==================== LTDC controller ==================== */

/* Pixel formats. The _ID macros keep the values visible to the preprocessor so
 * the pixel-size and drawing code below can be selected at compile time; the
 * enum exposes the same values through the typed API. */
#define LTDC_PIXFORMAT_ARGB8888_ID 0x00U
#define LTDC_PIXFORMAT_RGB888_ID   0x01U
#define LTDC_PIXFORMAT_RGB565_ID   0x02U
#define LTDC_PIXFORMAT_ARGB1555_ID 0x03U
#define LTDC_PIXFORMAT_ARGB4444_ID 0x04U
#define LTDC_PIXFORMAT_L8_ID       0x05U
#define LTDC_PIXFORMAT_AL44_ID     0x06U
#define LTDC_PIXFORMAT_AL88_ID     0x07U

/** @brief  LTDC layer pixel format. */
typedef enum
{
    LTDC_PIXFORMAT_ARGB8888 = LTDC_PIXFORMAT_ARGB8888_ID,
    LTDC_PIXFORMAT_RGB888   = LTDC_PIXFORMAT_RGB888_ID,
    LTDC_PIXFORMAT_RGB565   = LTDC_PIXFORMAT_RGB565_ID,
    LTDC_PIXFORMAT_ARGB1555 = LTDC_PIXFORMAT_ARGB1555_ID,
    LTDC_PIXFORMAT_ARGB4444 = LTDC_PIXFORMAT_ARGB4444_ID,
    LTDC_PIXFORMAT_L8       = LTDC_PIXFORMAT_L8_ID,
    LTDC_PIXFORMAT_AL44     = LTDC_PIXFORMAT_AL44_ID,
    LTDC_PIXFORMAT_AL88     = LTDC_PIXFORMAT_AL88_ID
} ltdc_pixformat_t;

#define LTDC_PIXFORMAT_ID   LTDC_PIXFORMAT_RGB565_ID
#define LTDC_PIXFORMAT      ((ltdc_pixformat_t)LTDC_PIXFORMAT_ID)
#define LTDC_BACKLAYERCOLOR 0x00000000U
#define LTDC_COLOR_WHITE    0xFFFFFFFFU

/* Bytes per pixel implied by the selected LTDC pixel format. */
#if (LTDC_PIXFORMAT_ID == LTDC_PIXFORMAT_ARGB8888_ID) || (LTDC_PIXFORMAT_ID == LTDC_PIXFORMAT_RGB888_ID)
#define LTDC_PIXSIZE_BYTE 4U
#else
#define LTDC_PIXSIZE_BYTE 2U
#endif

/** @brief  Frame buffer base address inside the on-board SDRAM. */
#define LTDC_FRAME_BUF_ADDR 0xC0000000U

/* LTDC_BL/DE/VSYNC/HSYNC/CLK pins (BL is a GPIO, the rest are AF14). */
#define LTDC_BL_PORT    GPIOB
#define LTDC_BL_PIN     GPIO_PIN_5
#define LTDC_DE_PORT    GPIOF
#define LTDC_DE_PIN     GPIO_PIN_10
#define LTDC_VSYNC_PORT GPIOI
#define LTDC_VSYNC_PIN  GPIO_PIN_9
#define LTDC_HSYNC_PORT GPIOI
#define LTDC_HSYNC_PIN  GPIO_PIN_10
#define LTDC_CLK_PORT   GPIOG
#define LTDC_CLK_PIN    GPIO_PIN_7

/* LTDC RGB565 data pins, grouped per GPIO port for MSP init. */
#define LTDC_R_PORT     GPIOG
#define LTDC_R_PINS     (GPIO_PIN_6 | GPIO_PIN_11)
#define LTDC_G_PORT     GPIOH
#define LTDC_G_PINS     (GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | \
                         GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15)
#define LTDC_B_PORT     GPIOI
#define LTDC_B_PINS     (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | \
                         GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7)

#define LTDC_BL(x) do { \
        (x) ? HAL_GPIO_WritePin(LTDC_BL_PORT, LTDC_BL_PIN, GPIO_PIN_SET) : \
              HAL_GPIO_WritePin(LTDC_BL_PORT, LTDC_BL_PIN, GPIO_PIN_RESET); \
    } while (0)

/* Layer defaults. Blending factors are passed as HAL LTDC_BLENDING_FACTORx_* enums. */
#define LTDC_LAYER_ALPHA            255U
#define LTDC_LAYER_ALPHA0           0U

/* Byte lanes of a packed RGB colour. */
#define LTDC_COLOR_RED_MASK    0x00FF0000U
#define LTDC_COLOR_GREEN_MASK  0x0000FF00U
#define LTDC_COLOR_BLUE_MASK   0x000000FFU
#define LTDC_COLOR_RED_SHIFT   16U
#define LTDC_COLOR_GREEN_SHIFT 8U

/* DMA2D transfer poll timeout (ms). */
#define LTDC_DMA2D_TIMEOUT_COUNT  0x1FFFFFU

/** @brief  Panel orientation: 0 swaps the native raster, 1 keeps it. */
typedef enum
{
    LTDC_DIR_PORTRAIT  = 0,
    LTDC_DIR_LANDSCAPE = 1
} ltdc_dir_t;

/** @brief  LTDC layer index. */
typedef enum
{
    LTDC_ACTIVE_LAYER_0 = 0,
    LTDC_ACTIVE_LAYER_1 = 1
} ltdc_layer_t;

/** @brief  LTDC / layer power state. */
typedef enum
{
    LTDC_OFF = 0,
    LTDC_ON = 1
} ltdc_power_t;

/** @brief  LTDC screen configuration. */
typedef struct
{
    uint32_t     pwidth;      /* panel width  (fixed) */
    uint32_t     pheight;     /* panel height (fixed) */
    uint16_t     hsw;         /* horizontal sync width */
    uint16_t     vsw;         /* vertical sync width */
    uint16_t     hbp;         /* horizontal back porch */
    uint16_t     vbp;         /* vertical back porch */
    uint16_t     hfp;         /* horizontal front porch */
    uint16_t     vfp;         /* vertical front porch */
    ltdc_layer_t activelayer; /* active layer: 0/1 */
    ltdc_dir_t   dir;         /* 0 portrait, 1 landscape */
    uint16_t     width;       /* logical width */
    uint16_t     height;      /* logical height */
    uint32_t     pixsize;     /* bytes per pixel */
} ltdc_dev_t;

/** @brief  Read-only access to the LTDC device state. */
const ltdc_dev_t *ltdc_info(void);

/** @brief  Frame buffer base address of the active layer. */
uint32_t ltdc_framebuf(void);

void ltdc_switch(ltdc_power_t sw);
void ltdc_layer_switch(ltdc_layer_t layerx, ltdc_power_t sw);
void ltdc_select_layer(ltdc_layer_t layerx);
void ltdc_display_dir(ltdc_dir_t dir);
void ltdc_draw_point(uint16_t x, uint16_t y, uint32_t color);
uint32_t ltdc_read_point(uint16_t x, uint16_t y);
void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void ltdc_blit(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, const uint16_t *src);
void ltdc_clear(uint32_t color);
uint8_t ltdc_clk_set(uint32_t pllsain, uint32_t pllsair, uint32_t pllsaidivr);
void ltdc_layer_window_config(ltdc_layer_t layerx, uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);
void ltdc_layer_parameter_config(ltdc_layer_t layerx, uint32_t bufaddr, ltdc_pixformat_t pixformat, uint8_t alpha,
                                 uint8_t alpha0, uint32_t bfac1, uint32_t bfac2, uint32_t bkcolor);
void ltdc_init(const lcd_rgb_cfg_t *panel);

#endif /* BSP_LCD_RGB_H */
