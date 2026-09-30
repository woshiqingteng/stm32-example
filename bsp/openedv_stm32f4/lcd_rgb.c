/**
 * @file    lcd_rgb.c
 * @brief   RGB screen driver: panel id detection/configuration and the LTDC
 *          (with SDRAM frame buffer) controller. Ported from the vendor
 *          example; clock/GPIO are initialised inline in ltdc_init() so there
 *          is no HAL_LTDC_MspInit().
 *
 * Only the 4.3 inch panel (id 0x4384) is implemented. The frame buffer lives in
 * the on-board SDRAM; the declared region is accessed through a pointer because
 * GCC has no __attribute__((at(...))).
 */

#include "lcd.h"
#include "lcd_rgb.h"

/* ==================== panel ==================== */

/* Single supported panel: 4.3 inch, 800x480 (id 0x4384). */
static const lcd_rgb_cfg_t g_lcd_rgb_4384 =
{
    LCD_PANEL_ID_4384,
    LCD_WIDTH_PX, LCD_HEIGHT_PX,
    LCD_PANEL_HSW, LCD_PANEL_VSW,
    LCD_PANEL_HBP, LCD_PANEL_VBP,
    LCD_PANEL_HFP, LCD_PANEL_VFP,
    LCD_PANEL_PLLSAIN_RAW, LCD_PANEL_PLLSAIR_RAW, LCD_PANEL_PLLSAIDIVR_RAW,
    LTDC_PCPOLARITY_IPC
};

/* Panel id straps: PG6, PI2, PI7. */
static uint16_t lcd_rgb_id_decode(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    uint8_t idx;

    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio_init.Pin   = GPIO_PIN_6;
    gpio_init.Mode  = GPIO_MODE_INPUT;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOG, &gpio_init);

    gpio_init.Pin = GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOI, &gpio_init);

    idx = (uint8_t)(HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_6) << LCD_PANEL_IDX_SHIFT_0);
    idx |= (uint8_t)(HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_2) << LCD_PANEL_IDX_SHIFT_1);
    idx |= (uint8_t)(HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_7) << LCD_PANEL_IDX_SHIFT_2);

    if (idx == LCD_PANEL_IDX_4384)
    {
        return LCD_PANEL_ID_4384;
    }

    return 0U;
}

const lcd_rgb_cfg_t *lcd_rgb_probe(void)
{
    if (lcd_rgb_id_decode() == LCD_PANEL_ID_4384)
    {
        return &g_lcd_rgb_4384;
    }

    return 0;
}

/* ==================== LTDC controller ==================== */

static ltdc_dev_t g_ltdc_dev;
static LTDC_HandleTypeDef g_ltdc_handle;
static DMA2D_HandleTypeDef g_dma2d_handle;
static uint32_t *g_ltdc_framebuf[2];

const ltdc_dev_t *ltdc_info(void)
{
    return &g_ltdc_dev;
}

uint32_t ltdc_framebuf(void)
{
    return (uint32_t)g_ltdc_framebuf[g_ltdc_dev.activelayer];
}

void ltdc_switch(ltdc_power_t sw)
{
    if (sw != LTDC_OFF)
    {
        __HAL_LTDC_ENABLE(&g_ltdc_handle);
    }
    else
    {
        __HAL_LTDC_DISABLE(&g_ltdc_handle);
    }
}

void ltdc_layer_switch(ltdc_layer_t layerx, ltdc_power_t sw)
{
    if (sw != LTDC_OFF)
    {
        __HAL_LTDC_LAYER_ENABLE(&g_ltdc_handle, layerx);
    }
    else
    {
        __HAL_LTDC_LAYER_DISABLE(&g_ltdc_handle, layerx);
    }

    __HAL_LTDC_RELOAD_CONFIG(&g_ltdc_handle);
}

void ltdc_select_layer(ltdc_layer_t layerx)
{
    g_ltdc_dev.activelayer = layerx;
}

void ltdc_display_dir(ltdc_dir_t dir)
{
    g_ltdc_dev.dir = dir;

    if (dir == LTDC_DIR_PORTRAIT)
    {
        g_ltdc_dev.width  = g_ltdc_dev.pheight;
        g_ltdc_dev.height = g_ltdc_dev.pwidth;
    }
    else
    {
        g_ltdc_dev.width  = g_ltdc_dev.pwidth;
        g_ltdc_dev.height = g_ltdc_dev.pheight;
    }
}

void ltdc_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
#if (LTDC_PIXFORMAT_ID == LTDC_PIXFORMAT_ARGB8888_ID) || (LTDC_PIXFORMAT_ID == LTDC_PIXFORMAT_RGB888_ID)
    if (g_ltdc_dev.dir == LTDC_DIR_LANDSCAPE)
    {
        *(uint32_t *)((uint32_t)g_ltdc_framebuf[g_ltdc_dev.activelayer] +
                      g_ltdc_dev.pixsize * (g_ltdc_dev.pwidth * y + x)) = color;
    }
    else
    {
        *(uint32_t *)((uint32_t)g_ltdc_framebuf[g_ltdc_dev.activelayer] +
                      g_ltdc_dev.pixsize * (g_ltdc_dev.pwidth * (g_ltdc_dev.pheight - x - 1) + y)) = color;
    }
#else
    if (g_ltdc_dev.dir == LTDC_DIR_LANDSCAPE)
    {
        *(uint16_t *)((uint32_t)g_ltdc_framebuf[g_ltdc_dev.activelayer] +
                      g_ltdc_dev.pixsize * (g_ltdc_dev.pwidth * y + x)) = (uint16_t)color;
    }
    else
    {
        *(uint16_t *)((uint32_t)g_ltdc_framebuf[g_ltdc_dev.activelayer] +
                      g_ltdc_dev.pixsize * (g_ltdc_dev.pwidth * (g_ltdc_dev.pheight - x - 1) + y)) = (uint16_t)color;
    }
#endif
}

uint32_t ltdc_read_point(uint16_t x, uint16_t y)
{
#if (LTDC_PIXFORMAT_ID == LTDC_PIXFORMAT_ARGB8888_ID) || (LTDC_PIXFORMAT_ID == LTDC_PIXFORMAT_RGB888_ID)
    if (g_ltdc_dev.dir == LTDC_DIR_LANDSCAPE)
    {
        return *(uint32_t *)((uint32_t)g_ltdc_framebuf[g_ltdc_dev.activelayer] +
                             g_ltdc_dev.pixsize * (g_ltdc_dev.pwidth * y + x));
    }
    else
    {
        return *(uint32_t *)((uint32_t)g_ltdc_framebuf[g_ltdc_dev.activelayer] +
                             g_ltdc_dev.pixsize * (g_ltdc_dev.pwidth * (g_ltdc_dev.pheight - x - 1) + y));
    }
#else
    if (g_ltdc_dev.dir == LTDC_DIR_LANDSCAPE)
    {
        return *(uint16_t *)((uint32_t)g_ltdc_framebuf[g_ltdc_dev.activelayer] +
                             g_ltdc_dev.pixsize * (g_ltdc_dev.pwidth * y + x));
    }
    else
    {
        return *(uint16_t *)((uint32_t)g_ltdc_framebuf[g_ltdc_dev.activelayer] +
                             g_ltdc_dev.pixsize * (g_ltdc_dev.pwidth * (g_ltdc_dev.pheight - x - 1) + y));
    }
#endif
}

/* HAL_DMA2D R2M expects an ARGB8888 colour value and converts it to the output
 * format internally. Callers pass packed RGB565 colours, so expand first to
 * keep the same output bytes as the previous direct OCOLR write. */
static uint32_t ltdc_expand_color(uint32_t color)
{
#if LTDC_PIXFORMAT_ID == LTDC_PIXFORMAT_RGB565_ID
    uint32_t r = (color >> 11) & 0x1FU;
    uint32_t g = (color >> 5) & 0x3FU;
    uint32_t b = color & 0x1FU;

    return (r << 19) | (g << 10) | (b << 3);
#else
    return color;
#endif
}

/* Map a logical rect to the frame-buffer address, line offset and size. */
static void ltdc_rect_addr(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey,
                           uint32_t *addr, uint16_t *offline, uint32_t *width, uint32_t *height)
{
    uint32_t psx;
    uint32_t psy;
    uint32_t pex;
    uint32_t pey;

    if (g_ltdc_dev.dir == LTDC_DIR_LANDSCAPE)
    {
        psx = sx;
        psy = sy;
        pex = ex;
        pey = ey;
    }
    else
    {
        psx = sy;
        psy = g_ltdc_dev.pheight - ex - 1U;
        pex = ey;
        pey = g_ltdc_dev.pheight - sx - 1U;
    }

    *offline = (uint16_t)(g_ltdc_dev.pwidth - (pex - psx + 1U));
    *addr    = ((uint32_t)g_ltdc_framebuf[g_ltdc_dev.activelayer] +
                g_ltdc_dev.pixsize * (g_ltdc_dev.pwidth * psy + psx));
    *width   = pex - psx + 1U;
    *height  = pey - psy + 1U;
}

void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    uint32_t addr;
    uint32_t w;
    uint32_t h;
    uint16_t offline;

    if (g_ltdc_dev.dir == LTDC_DIR_PORTRAIT)
    {
        if (ex >= g_ltdc_dev.pheight)
        {
            ex = g_ltdc_dev.pheight - 1U;
        }

        if (sx >= g_ltdc_dev.pheight)
        {
            sx = g_ltdc_dev.pheight - 1U;
        }
    }

    ltdc_rect_addr(sx, sy, ex, ey, &addr, &offline, &w, &h);

    g_dma2d_handle.Instance          = DMA2D;
    g_dma2d_handle.Init.Mode         = DMA2D_R2M;
    g_dma2d_handle.Init.ColorMode    = LTDC_PIXFORMAT;
    g_dma2d_handle.Init.OutputOffset = offline;
    (void)HAL_DMA2D_Init(&g_dma2d_handle);

    (void)HAL_DMA2D_Start(&g_dma2d_handle, ltdc_expand_color(color), addr, w, h);
    (void)HAL_DMA2D_PollForTransfer(&g_dma2d_handle, LTDC_DMA2D_TIMEOUT_COUNT);
}

void ltdc_blit(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, const uint16_t *src)
{
    uint32_t addr;
    uint32_t w;
    uint32_t h;
    uint16_t offline;

    ltdc_rect_addr(sx, sy, ex, ey, &addr, &offline, &w, &h);

    g_dma2d_handle.Instance          = DMA2D;
    g_dma2d_handle.Init.Mode         = DMA2D_M2M;
    g_dma2d_handle.Init.ColorMode    = LTDC_PIXFORMAT;
    g_dma2d_handle.Init.OutputOffset = offline;
    (void)HAL_DMA2D_Init(&g_dma2d_handle);

    /* Foreground layer describes the source rectangle (no row stride offset). */
    g_dma2d_handle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputColorMode = LTDC_PIXFORMAT;
    g_dma2d_handle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputOffset    = 0U;
    g_dma2d_handle.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaMode      = DMA2D_NO_MODIF_ALPHA;
    g_dma2d_handle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputAlpha     = 0U;
    (void)HAL_DMA2D_ConfigLayer(&g_dma2d_handle, DMA2D_FOREGROUND_LAYER);

    (void)HAL_DMA2D_Start(&g_dma2d_handle, (uint32_t)src, addr, w, h);
    (void)HAL_DMA2D_PollForTransfer(&g_dma2d_handle, LTDC_DMA2D_TIMEOUT_COUNT);
}

void ltdc_clear(uint32_t color)
{
    ltdc_fill(0U, 0U, (uint16_t)(g_ltdc_dev.width - 1U), (uint16_t)(g_ltdc_dev.height - 1U), color);
}

uint8_t ltdc_clk_set(uint32_t pllsain, uint32_t pllsair, uint32_t pllsaidivr)
{
    RCC_PeriphCLKInitTypeDef periphclk_initure = {0};

    periphclk_initure.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    periphclk_initure.PLLSAI.PLLSAIN = pllsain;
    periphclk_initure.PLLSAI.PLLSAIR = pllsair;
    periphclk_initure.PLLSAIDivR = pllsaidivr;

    if (HAL_RCCEx_PeriphCLKConfig(&periphclk_initure) == HAL_OK)
    {
        return 0U;
    }

    return 1U;
}

void ltdc_layer_window_config(ltdc_layer_t layerx, uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    (void)HAL_LTDC_SetWindowPosition(&g_ltdc_handle, sx, sy, layerx);
    (void)HAL_LTDC_SetWindowSize(&g_ltdc_handle, width, height, layerx);
}

void ltdc_layer_parameter_config(ltdc_layer_t layerx, uint32_t bufaddr, ltdc_pixformat_t pixformat, uint8_t alpha,
                                 uint8_t alpha0, uint32_t bfac1, uint32_t bfac2, uint32_t bkcolor)
{
    LTDC_LayerCfgTypeDef playercfg = {0};

    playercfg.WindowX0 = 0U;
    playercfg.WindowY0 = 0U;
    playercfg.WindowX1 = g_ltdc_dev.pwidth;
    playercfg.WindowY1 = g_ltdc_dev.pheight;
    playercfg.PixelFormat = (uint32_t)pixformat;
    playercfg.Alpha = alpha;
    playercfg.Alpha0 = alpha0;
    playercfg.BlendingFactor1 = bfac1;
    playercfg.BlendingFactor2 = bfac2;
    playercfg.FBStartAdress = bufaddr;
    playercfg.ImageWidth = g_ltdc_dev.pwidth;
    playercfg.ImageHeight = g_ltdc_dev.pheight;
    playercfg.Backcolor.Red = (uint8_t)(bkcolor & LTDC_COLOR_RED_MASK) >> LTDC_COLOR_RED_SHIFT;
    playercfg.Backcolor.Green = (uint8_t)(bkcolor & LTDC_COLOR_GREEN_MASK) >> LTDC_COLOR_GREEN_SHIFT;
    playercfg.Backcolor.Blue = (uint8_t)bkcolor & LTDC_COLOR_BLUE_MASK;

    (void)HAL_LTDC_ConfigLayer(&g_ltdc_handle, &playercfg, layerx);
}

void ltdc_init(const lcd_rgb_cfg_t *panel)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    if (panel != 0)
    {
        g_ltdc_dev.pwidth  = panel->pwidth;
        g_ltdc_dev.pheight = panel->pheight;
        g_ltdc_dev.hbp = panel->hbp;
        g_ltdc_dev.hfp = panel->hfp;
        g_ltdc_dev.hsw = panel->hsw;
        g_ltdc_dev.vbp = panel->vbp;
        g_ltdc_dev.vfp = panel->vfp;
        g_ltdc_dev.vsw = panel->vsw;
        (void)ltdc_clk_set(panel->pllsain, panel->pllsair, panel->pllsaidivr);
    }

    g_ltdc_dev.width = (uint16_t)g_ltdc_dev.pwidth;
    g_ltdc_dev.height = (uint16_t)g_ltdc_dev.pheight;

    g_ltdc_framebuf[0] = (uint32_t *)LTDC_FRAME_BUF_ADDR;
    g_ltdc_dev.pixsize = LTDC_PIXSIZE_BYTE;

    /* ---- MSP begin ---- */
    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_DMA2D_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio_init_struct.Pin = LTDC_BL_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LTDC_BL_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = LTDC_DE_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(LTDC_DE_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = LTDC_VSYNC_PIN;
    HAL_GPIO_Init(LTDC_VSYNC_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = LTDC_HSYNC_PIN;
    HAL_GPIO_Init(LTDC_HSYNC_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = LTDC_CLK_PIN;
    HAL_GPIO_Init(LTDC_CLK_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = LTDC_R_PINS;
    HAL_GPIO_Init(LTDC_R_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = LTDC_G_PINS;
    HAL_GPIO_Init(LTDC_G_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = LTDC_B_PINS;
    HAL_GPIO_Init(LTDC_B_PORT, &gpio_init_struct);
    /* ---- MSP end ---- */

    g_ltdc_handle.Instance = LTDC;
    g_ltdc_handle.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    g_ltdc_handle.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    g_ltdc_handle.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    g_ltdc_handle.Init.PCPolarity = (panel != 0) ? panel->pcpolarity : LTDC_PCPOLARITY_IPC;
    g_ltdc_handle.Init.HorizontalSync = g_ltdc_dev.hsw - 1U;
    g_ltdc_handle.Init.VerticalSync = g_ltdc_dev.vsw - 1U;
    g_ltdc_handle.Init.AccumulatedHBP = g_ltdc_dev.hsw + g_ltdc_dev.hbp - 1U;
    g_ltdc_handle.Init.AccumulatedVBP = g_ltdc_dev.vsw + g_ltdc_dev.vbp - 1U;
    g_ltdc_handle.Init.AccumulatedActiveW = g_ltdc_dev.hsw + g_ltdc_dev.hbp + g_ltdc_dev.pwidth - 1U;
    g_ltdc_handle.Init.AccumulatedActiveH = g_ltdc_dev.vsw + g_ltdc_dev.vbp + g_ltdc_dev.pheight - 1U;
    g_ltdc_handle.Init.TotalWidth = g_ltdc_dev.hsw + g_ltdc_dev.hbp + g_ltdc_dev.pwidth + g_ltdc_dev.hfp - 1U;
    g_ltdc_handle.Init.TotalHeigh = g_ltdc_dev.vsw + g_ltdc_dev.vbp + g_ltdc_dev.pheight + g_ltdc_dev.vfp - 1U;
    g_ltdc_handle.Init.Backcolor.Red = 0U;
    g_ltdc_handle.Init.Backcolor.Green = 0U;
    g_ltdc_handle.Init.Backcolor.Blue = 0U;
    g_ltdc_handle.State = HAL_LTDC_STATE_RESET;

    (void)HAL_LTDC_Init(&g_ltdc_handle);

    ltdc_layer_parameter_config(LTDC_ACTIVE_LAYER_0, (uint32_t)g_ltdc_framebuf[0], LTDC_PIXFORMAT,
                                LTDC_LAYER_ALPHA, LTDC_LAYER_ALPHA0,
                                LTDC_BLENDING_FACTOR1_PAxCA, LTDC_BLENDING_FACTOR2_PAxCA,
                                LTDC_BACKLAYERCOLOR);
    ltdc_layer_window_config(LTDC_ACTIVE_LAYER_0, 0U, 0U, (uint16_t)g_ltdc_dev.pwidth, (uint16_t)g_ltdc_dev.pheight);

    ltdc_select_layer(LTDC_ACTIVE_LAYER_0);
    LTDC_BL(1);
    ltdc_clear(LTDC_COLOR_WHITE);
}
