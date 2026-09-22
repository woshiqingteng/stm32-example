/**
 * @file    ltdc.h
 * @brief   RGB screen driver (LTDC + SDRAM frame buffer).
 *
 * Only the 4.3 inch panel (id 0x4384) is implemented; all other panel ids are
 * collapsed into an empty "other" path. Clock/GPIO are initialised inline in
 * ltdc_init() instead of a HAL_LTDC_MspInit().
 */

#ifndef BSP_LTDC_H
#define BSP_LTDC_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

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
#define LTDC_PIXSIZE 4U
#else
#define LTDC_PIXSIZE 2U
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

#define LTDC_BL(x) do { \
        (x) ? HAL_GPIO_WritePin(LTDC_BL_PORT, LTDC_BL_PIN, GPIO_PIN_SET) : \
              HAL_GPIO_WritePin(LTDC_BL_PORT, LTDC_BL_PIN, GPIO_PIN_RESET); \
    } while (0)

/** @brief  Supported panel: 4.3 inch, native 800x480 raster (id 0x4384). */
#define LTDC_PANEL_ID_4384  0x4384U
#define LTDC_PANEL_WIDTH    800U
#define LTDC_PANEL_HEIGHT   480U
#define LTDC_IDX_4384       4U

/* Panel raster timing. */
#define LTDC_PANEL_HSW      48U
#define LTDC_PANEL_HBP      88U
#define LTDC_PANEL_HFP      40U
#define LTDC_PANEL_VSW      3U
#define LTDC_PANEL_VBP      32U
#define LTDC_PANEL_VFP      13U

/* LTDC pixel clock PLL. */
#define LTDC_PLLSAIN        396U
#define LTDC_PLLSAIR        3U
#define LTDC_PLLSAIDIVR     RCC_PLLSAIDIVR_4

/* Panel id strap bit positions. */
#define LTDC_IDX_SHIFT_0    0U
#define LTDC_IDX_SHIFT_1    1U
#define LTDC_IDX_SHIFT_2    2U

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
#define LTDC_DMA2D_TIMEOUT  0x1FFFFFU

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
    LTDC_ON
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
} _ltdc_dev;

extern _ltdc_dev lcdltdc;
extern LTDC_HandleTypeDef g_ltdc_handle;
extern DMA2D_HandleTypeDef g_dma2d_handle;
extern uint32_t *g_ltdc_framebuf[2];

void ltdc_switch(ltdc_power_t sw);
void ltdc_layer_switch(ltdc_layer_t layerx, ltdc_power_t sw);
void ltdc_select_layer(ltdc_layer_t layerx);
void ltdc_display_dir(ltdc_dir_t dir);
void ltdc_draw_point(uint16_t x, uint16_t y, uint32_t color);
uint32_t ltdc_read_point(uint16_t x, uint16_t y);
void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
void ltdc_clear(uint32_t color);
uint8_t ltdc_clk_set(uint32_t pllsain, uint32_t pllsair, uint32_t pllsaidivr);
void ltdc_layer_window_config(ltdc_layer_t layerx, uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);
void ltdc_layer_parameter_config(ltdc_layer_t layerx, uint32_t bufaddr, ltdc_pixformat_t pixformat, uint8_t alpha,
                                 uint8_t alpha0, uint32_t bfac1, uint32_t bfac2, uint32_t bkcolor);
uint16_t ltdc_panelid_read(void);
void ltdc_init(void);

#endif /* BSP_LTDC_H */
