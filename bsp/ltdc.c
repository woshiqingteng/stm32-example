/**
 * @file    ltdc.c
 * @brief   RGB screen driver (LTDC + SDRAM frame buffer), ported from the
 *          vendor example (ltdc.c). Clock/GPIO are initialised inline in
 *          ltdc_init(); there is no HAL_LTDC_MspInit().
 *
 * Only the 4.3 inch panel (id 0x4384) is implemented. The frame buffer lives in
 * the on-board SDRAM; the declared region is accessed through a pointer because
 * GCC has no __attribute__((at(...))).
 */

#include "lcd.h"
#include "ltdc.h"

_ltdc_dev lcdltdc;
LTDC_HandleTypeDef g_ltdc_handle;
DMA2D_HandleTypeDef g_dma2d_handle;
uint32_t *g_ltdc_framebuf[2];

void ltdc_switch(uint8_t sw)
{
    if (sw != 0U)
    {
        __HAL_LTDC_ENABLE(&g_ltdc_handle);
    }
    else
    {
        __HAL_LTDC_DISABLE(&g_ltdc_handle);
    }
}

void ltdc_layer_switch(ltdc_layer_t layerx, uint8_t sw)
{
    if (sw != 0U)
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
    lcdltdc.activelayer = layerx;
}

void ltdc_display_dir(ltdc_dir_t dir)
{
    lcdltdc.dir = dir;

    if (dir == LTDC_DIR_PORTRAIT)
    {
        lcdltdc.width  = lcdltdc.pheight;
        lcdltdc.height = lcdltdc.pwidth;
    }
    else
    {
        lcdltdc.width  = lcdltdc.pwidth;
        lcdltdc.height = lcdltdc.pheight;
    }
}

void ltdc_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888 || LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888
    if (lcdltdc.dir == LTDC_DIR_LANDSCAPE)
    {
        *(uint32_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] +
                      lcdltdc.pixsize * (lcdltdc.pwidth * y + x)) = color;
    }
    else
    {
        *(uint32_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] +
                      lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - x - 1) + y)) = color;
    }
#else
    if (lcdltdc.dir == LTDC_DIR_LANDSCAPE)
    {
        *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] +
                      lcdltdc.pixsize * (lcdltdc.pwidth * y + x)) = (uint16_t)color;
    }
    else
    {
        *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] +
                      lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - x - 1) + y)) = (uint16_t)color;
    }
#endif
}

uint32_t ltdc_read_point(uint16_t x, uint16_t y)
{
#if LTDC_PIXFORMAT == LTDC_PIXFORMAT_ARGB8888 || LTDC_PIXFORMAT == LTDC_PIXFORMAT_RGB888
    if (lcdltdc.dir == LTDC_DIR_LANDSCAPE)
    {
        return *(uint32_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] +
                             lcdltdc.pixsize * (lcdltdc.pwidth * y + x));
    }
    else
    {
        return *(uint32_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] +
                             lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - x - 1) + y));
    }
#else
    if (lcdltdc.dir == LTDC_DIR_LANDSCAPE)
    {
        return *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] +
                             lcdltdc.pixsize * (lcdltdc.pwidth * y + x));
    }
    else
    {
        return *(uint16_t *)((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] +
                             lcdltdc.pixsize * (lcdltdc.pwidth * (lcdltdc.pheight - x - 1) + y));
    }
#endif
}

void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    uint32_t psx;
    uint32_t psy;
    uint32_t pex;
    uint32_t pey;
    uint32_t timeout = 0;
    uint16_t offline;
    uint32_t addr;

    if (lcdltdc.dir == LTDC_DIR_LANDSCAPE)
    {
        psx = sx;
        psy = sy;
        pex = ex;
        pey = ey;
    }
    else
    {
        if (ex >= lcdltdc.pheight)
        {
            ex = lcdltdc.pheight - 1U;
        }

        if (sx >= lcdltdc.pheight)
        {
            sx = lcdltdc.pheight - 1U;
        }

        psx = sy;
        psy = lcdltdc.pheight - ex - 1U;
        pex = ey;
        pey = lcdltdc.pheight - sx - 1U;
    }

    offline = (uint16_t)(lcdltdc.pwidth - (pex - psx + 1U));
    addr = ((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] +
            lcdltdc.pixsize * (lcdltdc.pwidth * psy + psx));

    __HAL_RCC_DMA2D_CLK_ENABLE();
    DMA2D->IFCR = DMA2D_FLAG_TC | DMA2D_FLAG_TE | DMA2D_FLAG_TW |
                  DMA2D_FLAG_CAE | DMA2D_FLAG_CTC | DMA2D_FLAG_CE;
    DMA2D->CR &= ~DMA2D_CR_START;
    DMA2D->CR = DMA2D_R2M;
    DMA2D->OPFCCR = LTDC_PIXFORMAT;
    DMA2D->OOR = offline;
    DMA2D->OMAR = addr;
    DMA2D->NLR = (pey - psy + 1U) | ((pex - psx + 1U) << DMA2D_NLR_PL_SHIFT);
    DMA2D->OCOLR = color;
    DMA2D->CR |= DMA2D_CR_START;

    while ((DMA2D->ISR & DMA2D_FLAG_TC) == 0U)
    {
        timeout++;
        if (timeout > DMA2D_TIMEOUT_MAX)
        {
            break;
        }
    }

    DMA2D->IFCR |= DMA2D_FLAG_TC;
}

void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    uint32_t psx;
    uint32_t psy;
    uint32_t pex;
    uint32_t pey;
    uint32_t timeout = 0;
    uint16_t offline;
    uint32_t addr;

    if (lcdltdc.dir == LTDC_DIR_LANDSCAPE)
    {
        psx = sx;
        psy = sy;
        pex = ex;
        pey = ey;
    }
    else
    {
        psx = sy;
        psy = lcdltdc.pheight - ex - 1U;
        pex = ey;
        pey = lcdltdc.pheight - sx - 1U;
    }

    offline = (uint16_t)(lcdltdc.pwidth - (pex - psx + 1U));
    addr = ((uint32_t)g_ltdc_framebuf[lcdltdc.activelayer] +
            lcdltdc.pixsize * (lcdltdc.pwidth * psy + psx));

    __HAL_RCC_DMA2D_CLK_ENABLE();
    DMA2D->CR = DMA2D_M2M;
    DMA2D->FGPFCCR = LTDC_PIXFORMAT;
    DMA2D->FGOR = 0U;
    DMA2D->OOR = offline;
    DMA2D->CR &= ~DMA2D_CR_START;
    DMA2D->FGMAR = (uint32_t)color;
    DMA2D->OMAR = addr;
    DMA2D->NLR = (pey - psy + 1U) | ((pex - psx + 1U) << DMA2D_NLR_PL_SHIFT);
    DMA2D->CR |= DMA2D_CR_START;

    while ((DMA2D->ISR & DMA2D_FLAG_TC) == 0U)
    {
        timeout++;
        if (timeout > DMA2D_TIMEOUT_MAX)
        {
            break;
        }
    }

    DMA2D->IFCR |= DMA2D_FLAG_TC;
}

void ltdc_clear(uint32_t color)
{
    ltdc_fill(0U, 0U, (uint16_t)(lcdltdc.width - 1U), (uint16_t)(lcdltdc.height - 1U), color);
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

void ltdc_layer_parameter_config(ltdc_layer_t layerx, uint32_t bufaddr, uint8_t pixformat, uint8_t alpha,
                                 uint8_t alpha0, uint8_t bfac1, uint8_t bfac2, uint32_t bkcolor)
{
    LTDC_LayerCfgTypeDef playercfg = {0};

    playercfg.WindowX0 = 0U;
    playercfg.WindowY0 = 0U;
    playercfg.WindowX1 = lcdltdc.pwidth;
    playercfg.WindowY1 = lcdltdc.pheight;
    playercfg.PixelFormat = pixformat;
    playercfg.Alpha = alpha;
    playercfg.Alpha0 = alpha0;
    playercfg.BlendingFactor1 = (uint32_t)bfac1 << LTDC_BLENDING_FACTOR1_SHIFT;
    playercfg.BlendingFactor2 = (uint32_t)bfac2;
    playercfg.FBStartAdress = bufaddr;
    playercfg.ImageWidth = lcdltdc.pwidth;
    playercfg.ImageHeight = lcdltdc.pheight;
    playercfg.Backcolor.Red = (uint8_t)(bkcolor & LTDC_COLOR_RED_MASK) >> LTDC_COLOR_RED_SHIFT;
    playercfg.Backcolor.Green = (uint8_t)(bkcolor & LTDC_COLOR_GREEN_MASK) >> LTDC_COLOR_GREEN_SHIFT;
    playercfg.Backcolor.Blue = (uint8_t)bkcolor & LTDC_COLOR_BLUE_MASK;

    (void)HAL_LTDC_ConfigLayer(&g_ltdc_handle, &playercfg, layerx);
}

uint16_t ltdc_panelid_read(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    uint8_t idx = 0;

    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio_init_struct.Pin = GPIO_PIN_6;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOG, &gpio_init_struct);

    gpio_init_struct.Pin = GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOI, &gpio_init_struct);

    idx = (uint8_t)(HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_6) << LTDC_IDX_SHIFT_0);
    idx |= (uint8_t)(HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_2) << LTDC_IDX_SHIFT_1);
    idx |= (uint8_t)(HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_7) << LTDC_IDX_SHIFT_2);

    if (idx == LTDC_IDX_4384)
    {
        return LTDC_PANEL_ID_4384;
    }

    return 0U;
}

void ltdc_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    uint16_t ltdcid;

    ltdcid = ltdc_panelid_read();

    if (ltdcid == LTDC_PANEL_ID_4384)
    {
        lcdltdc.pwidth = LTDC_PANEL_WIDTH;
        lcdltdc.pheight = LTDC_PANEL_HEIGHT;
        lcdltdc.hbp = LTDC_PANEL_HBP;
        lcdltdc.hfp = LTDC_PANEL_HFP;
        lcdltdc.hsw = LTDC_PANEL_HSW;
        lcdltdc.vbp = LTDC_PANEL_VBP;
        lcdltdc.vfp = LTDC_PANEL_VFP;
        lcdltdc.vsw = LTDC_PANEL_VSW;
        (void)ltdc_clk_set(LTDC_PLLSAIN, LTDC_PLLSAIR, LTDC_PLLSAIDIVR);
    }
    else
    {
        /* other panels intentionally not implemented. */
    }

    lcddev.width = (uint16_t)lcdltdc.pwidth;
    lcddev.height = (uint16_t)lcdltdc.pheight;
    lcdltdc.width = (uint16_t)lcdltdc.pwidth;
    lcdltdc.height = (uint16_t)lcdltdc.pheight;

    g_ltdc_framebuf[0] = (uint32_t *)LTDC_FRAME_BUF_ADDR;
    lcdltdc.pixsize = LTDC_PIXSIZE;

    /* MSP begin */
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

    gpio_init_struct.Pin = GPIO_PIN_6 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOG, &gpio_init_struct);

    gpio_init_struct.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                           GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOH, &gpio_init_struct);

    gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
                           GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOI, &gpio_init_struct);
    /* MSP end */

    g_ltdc_handle.Instance = LTDC;
    g_ltdc_handle.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    g_ltdc_handle.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    g_ltdc_handle.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    g_ltdc_handle.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    g_ltdc_handle.Init.HorizontalSync = lcdltdc.hsw - 1U;
    g_ltdc_handle.Init.VerticalSync = lcdltdc.vsw - 1U;
    g_ltdc_handle.Init.AccumulatedHBP = lcdltdc.hsw + lcdltdc.hbp - 1U;
    g_ltdc_handle.Init.AccumulatedVBP = lcdltdc.vsw + lcdltdc.vbp - 1U;
    g_ltdc_handle.Init.AccumulatedActiveW = lcdltdc.hsw + lcdltdc.hbp + lcdltdc.pwidth - 1U;
    g_ltdc_handle.Init.AccumulatedActiveH = lcdltdc.vsw + lcdltdc.vbp + lcdltdc.pheight - 1U;
    g_ltdc_handle.Init.TotalWidth = lcdltdc.hsw + lcdltdc.hbp + lcdltdc.pwidth + lcdltdc.hfp - 1U;
    g_ltdc_handle.Init.TotalHeigh = lcdltdc.vsw + lcdltdc.vbp + lcdltdc.pheight + lcdltdc.vfp - 1U;
    g_ltdc_handle.Init.Backcolor.Red = 0U;
    g_ltdc_handle.Init.Backcolor.Green = 0U;
    g_ltdc_handle.Init.Backcolor.Blue = 0U;
    g_ltdc_handle.State = HAL_LTDC_STATE_RESET;

    (void)HAL_LTDC_Init(&g_ltdc_handle);

    ltdc_layer_parameter_config(LTDC_ACTIVE_LAYER_0, (uint32_t)g_ltdc_framebuf[0], LTDC_PIXFORMAT,
                                LTDC_LAYER_ALPHA, LTDC_LAYER_ALPHA0,
                                LTDC_BLENDING_FACTOR1, LTDC_BLENDING_FACTOR2, LTDC_BACKLAYERCOLOR);
    ltdc_layer_window_config(LTDC_ACTIVE_LAYER_0, 0U, 0U, (uint16_t)lcdltdc.pwidth, (uint16_t)lcdltdc.pheight);

    ltdc_select_layer(LTDC_ACTIVE_LAYER_0);
    LTDC_BL(1);
    ltdc_clear(LTDC_COLOR_WHITE);
}
