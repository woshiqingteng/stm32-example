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

/* Pixel formats. */
#define LTDC_PIXFORMAT_ARGB8888 0x00U
#define LTDC_PIXFORMAT_RGB888   0x01U
#define LTDC_PIXFORMAT_RGB565   0x02U
#define LTDC_PIXFORMAT_ARGB1555 0x03U
#define LTDC_PIXFORMAT_ARGB4444 0x04U
#define LTDC_PIXFORMAT_L8       0x05U
#define LTDC_PIXFORMAT_AL44     0x06U
#define LTDC_PIXFORMAT_AL88     0x07U

#define LTDC_PIXFORMAT LTDC_PIXFORMAT_RGB565
#define LTDC_BACKLAYERCOLOR 0x00000000U

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

/** @brief  LTDC screen configuration. */
typedef struct
{
    uint32_t pwidth;      /* panel width  (fixed) */
    uint32_t pheight;     /* panel height (fixed) */
    uint16_t hsw;         /* horizontal sync width */
    uint16_t vsw;         /* vertical sync width */
    uint16_t hbp;         /* horizontal back porch */
    uint16_t vbp;         /* vertical back porch */
    uint16_t hfp;         /* horizontal front porch */
    uint16_t vfp;         /* vertical front porch */
    uint8_t  activelayer; /* active layer: 0/1 */
    uint8_t  dir;         /* 0 portrait, 1 landscape */
    uint16_t width;       /* logical width */
    uint16_t height;      /* logical height */
    uint32_t pixsize;     /* bytes per pixel */
} _ltdc_dev;

extern _ltdc_dev lcdltdc;
extern LTDC_HandleTypeDef g_ltdc_handle;
extern DMA2D_HandleTypeDef g_dma2d_handle;
extern uint32_t *g_ltdc_framebuf[2];

void ltdc_switch(uint8_t sw);
void ltdc_layer_switch(uint8_t layerx, uint8_t sw);
void ltdc_select_layer(uint8_t layerx);
void ltdc_display_dir(uint8_t dir);
void ltdc_draw_point(uint16_t x, uint16_t y, uint32_t color);
uint32_t ltdc_read_point(uint16_t x, uint16_t y);
void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
void ltdc_clear(uint32_t color);
uint8_t ltdc_clk_set(uint32_t pllsain, uint32_t pllsair, uint32_t pllsaidivr);
void ltdc_layer_window_config(uint8_t layerx, uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);
void ltdc_layer_parameter_config(uint8_t layerx, uint32_t bufaddr, uint8_t pixformat, uint8_t alpha,
                                 uint8_t alpha0, uint8_t bfac1, uint8_t bfac2, uint32_t bkcolor);
uint16_t ltdc_panelid_read(void);
void ltdc_init(void);

#endif /* BSP_LTDC_H */
