/**
 * @file    ltdc.c
 * @brief   RGB LCD driver using LTDC with an SDRAM frame buffer.
 *
 * Only the 4.3 inch panel (id 0x4384, native 800x480 raster) is implemented;
 * all other panel ids are collapsed into an empty "other" path. Portrait is
 * obtained like the vendor driver: keep the native raster and rotate the
 * drawing coordinates (see ltdc_display_dir()).
 */

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "ltdc.h"
#include "lcdfont.h"

/* LTDC control and backlight pins. */
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

#define LTDC_BL_ON()  HAL_GPIO_WritePin(LTDC_BL_PORT, LTDC_BL_PIN, GPIO_PIN_SET)
#define LTDC_BL_OFF() HAL_GPIO_WritePin(LTDC_BL_PORT, LTDC_BL_PIN, GPIO_PIN_RESET)

/* Panel id strap pins. */
#define LTDC_ID_R7_PORT GPIOG
#define LTDC_ID_R7_PIN  GPIO_PIN_6
#define LTDC_ID_G7_PORT GPIOI
#define LTDC_ID_G7_PIN  GPIO_PIN_2
#define LTDC_ID_B7_PORT GPIOI
#define LTDC_ID_B7_PIN  GPIO_PIN_7

/* Panel id strap value for the 4.3 inch panel. */
#define LTDC_IDX_4384 4U

/* 4.3 inch panel native raster timing (800 x 480). */
#define LTDC_4384_HSW 48U
#define LTDC_4384_HBP 88U
#define LTDC_4384_HFP 40U
#define LTDC_4384_VSW 3U
#define LTDC_4384_VBP 32U
#define LTDC_4384_VFP 13U

#define LTDC_PLLSAIN_33MHZ 396U
#define LTDC_PLLSAIR_33MHZ 3U
#define LTDC_PLLSAIDIVR_33MHZ RCC_PLLSAIDIVR_4

/* 8x16 font metrics. */
#define LTDC_FONT_8X16   16U
#define LTDC_CHAR_WIDTH  8U
#define LTDC_ASCII_FIRST 0x20U
#define LTDC_ASCII_LAST  0x7EU

/* Pixel format and layer. */
#define LTDC_LAYER_INDEX 0U
#define LTDC_LAYER_ALPHA 255U
#define LTDC_LAYER_ALPHA0 0U

static LTDC_HandleTypeDef g_ltdc_handle;
static uint16_t *const g_ltdc_framebuf = (uint16_t *)LTDC_FRAME_BUF_ADDR;

/* Native raster size (fixed by the panel) and logical size (depends on dir). */
static uint16_t g_ltdc_pwidth  = LTDC_PANEL_WIDTH;
static uint16_t g_ltdc_pheight = LTDC_PANEL_HEIGHT;
static uint16_t g_ltdc_width   = LTDC_PANEL_HEIGHT; /* portrait 480 */
static uint16_t g_ltdc_height  = LTDC_PANEL_WIDTH;  /* portrait 800 */
static uint8_t  g_ltdc_dir     = LTDC_DIR_PORTRAIT;

static uint16_t g_ltdc_hsw = LTDC_4384_HSW;
static uint16_t g_ltdc_hbp = LTDC_4384_HBP;
static uint16_t g_ltdc_hfp = LTDC_4384_HFP;
static uint16_t g_ltdc_vsw = LTDC_4384_VSW;
static uint16_t g_ltdc_vbp = LTDC_4384_VBP;
static uint16_t g_ltdc_vfp = LTDC_4384_VFP;

uint16_t ltdc_panelid_read(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t idx;

    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_INPUT;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = LTDC_ID_R7_PIN;
    HAL_GPIO_Init(LTDC_ID_R7_PORT, &gpio);

    gpio.Pin = LTDC_ID_G7_PIN | LTDC_ID_B7_PIN;
    HAL_GPIO_Init(GPIOI, &gpio);

    idx = (uint8_t)HAL_GPIO_ReadPin(LTDC_ID_R7_PORT, LTDC_ID_R7_PIN);
    idx |= (uint8_t)(HAL_GPIO_ReadPin(LTDC_ID_G7_PORT, LTDC_ID_G7_PIN) << 1);
    idx |= (uint8_t)(HAL_GPIO_ReadPin(LTDC_ID_B7_PORT, LTDC_ID_B7_PIN) << 2);

    if (idx == LTDC_IDX_4384)
    {
        return LTDC_PANEL_ID_4384;
    }

    return 0U;
}

uint8_t ltdc_clk_set(uint32_t pllsain, uint32_t pllsair, uint32_t pllsaidivr)
{
    RCC_PeriphCLKInitTypeDef periph_clk = {0};

    periph_clk.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    periph_clk.PLLSAI.PLLSAIN      = pllsain;
    periph_clk.PLLSAI.PLLSAIR      = pllsair;
    periph_clk.PLLSAIDivR          = pllsaidivr;

    if (HAL_RCCEx_PeriphCLKConfig(&periph_clk) == HAL_OK)
    {
        return 0U;
    }

    return 1U;
}

void ltdc_display_dir(uint8_t dir)
{
    g_ltdc_dir = dir;

    if (dir == LTDC_DIR_PORTRAIT)
    {
        g_ltdc_width  = g_ltdc_pheight;
        g_ltdc_height = g_ltdc_pwidth;
    }
    else
    {
        g_ltdc_width  = g_ltdc_pwidth;
        g_ltdc_height = g_ltdc_pheight;
    }
}

void ltdc_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    uint32_t index;

    if ((x >= g_ltdc_width) || (y >= g_ltdc_height))
    {
        return;
    }

    if (g_ltdc_dir == LTDC_DIR_PORTRAIT)
    {
        index = ((uint32_t)g_ltdc_pwidth * (uint32_t)(g_ltdc_pheight - x - 1U)) + y;
    }
    else
    {
        index = ((uint32_t)g_ltdc_pwidth * y) + x;
    }

    g_ltdc_framebuf[index] = color;
}

void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    uint16_t x;
    uint16_t y;

    if ((sx > ex) || (sy > ey))
    {
        return;
    }

    for (y = sy; y <= ey; y++)
    {
        for (x = sx; x <= ex; x++)
        {
            ltdc_draw_point(x, y, color);
        }
    }
}

void ltdc_clear(uint16_t color)
{
    ltdc_fill(0U, 0U, (uint16_t)(g_ltdc_width - 1U), (uint16_t)(g_ltdc_height - 1U), color);
}

static void ltdc_show_char(uint16_t x, uint16_t y, char chr, uint16_t color)
{
    const uint8_t *pfont;
    uint16_t y0 = y;
    uint8_t t;
    uint8_t t1;
    uint8_t temp;

    if ((chr < (char)LTDC_ASCII_FIRST) || (chr > (char)LTDC_ASCII_LAST))
    {
        return;
    }

    pfont = (const uint8_t *)asc2_1608[(uint8_t)chr - LTDC_ASCII_FIRST];

    for (t = 0; t < LTDC_FONT_8X16; t++)
    {
        temp = pfont[t];

        for (t1 = 0; t1 < 8U; t1++)
        {
            if ((temp & 0x80U) != 0U)
            {
                ltdc_draw_point(x, y, color);
            }

            temp <<= 1;
            y++;

            if ((uint16_t)(y - y0) == LTDC_FONT_8X16)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}

void ltdc_show_string(uint16_t x, uint16_t y, const char *str, uint8_t size, uint16_t color)
{
    if (size != LTDC_FONT_8X16)
    {
        return;
    }

    while ((*str >= (char)LTDC_ASCII_FIRST) && (*str <= (char)LTDC_ASCII_LAST))
    {
        if (x > (g_ltdc_width - LTDC_CHAR_WIDTH))
        {
            x = 0U;
            y += LTDC_FONT_8X16;
        }

        ltdc_show_char(x, y, *str, color);
        x += LTDC_CHAR_WIDTH;
        str++;
    }
}

static uint32_t ltdc_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1U;

    while (n-- != 0U)
    {
        result *= m;
    }

    return result;
}

void ltdc_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t;
    uint8_t digit;
    uint8_t enshow = 0U;

    if (size != LTDC_FONT_8X16)
    {
        return;
    }

    for (t = 0; t < len; t++)
    {
        digit = (uint8_t)((num / ltdc_pow(10U, (uint8_t)(len - t - 1U))) % 10U);

        if ((enshow == 0U) && (t < (uint8_t)(len - 1U)))
        {
            if (digit == 0U)
            {
                ltdc_show_char((uint16_t)(x + (LTDC_CHAR_WIDTH * t)), y, ' ', color);
                continue;
            }

            enshow = 1U;
        }

        ltdc_show_char((uint16_t)(x + (LTDC_CHAR_WIDTH * t)), y, (char)('0' + digit), color);
    }
}

void ltdc_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    LTDC_LayerCfgTypeDef layer = {0};
    uint16_t panel_id;

    panel_id = ltdc_panelid_read();
    printf("LTDC panel id:0x%04X\r\n", (unsigned int)panel_id);

    /* Only the 4.3 inch panel (native 800x480) is implemented. */
    if (panel_id == LTDC_PANEL_ID_4384)
    {
        g_ltdc_pwidth  = LTDC_PANEL_WIDTH;
        g_ltdc_pheight = LTDC_PANEL_HEIGHT;
        g_ltdc_hsw     = LTDC_4384_HSW;
        g_ltdc_hbp     = LTDC_4384_HBP;
        g_ltdc_hfp     = LTDC_4384_HFP;
        g_ltdc_vsw     = LTDC_4384_VSW;
        g_ltdc_vbp     = LTDC_4384_VBP;
        g_ltdc_vfp     = LTDC_4384_VFP;
        (void)ltdc_clk_set(LTDC_PLLSAIN_33MHZ, LTDC_PLLSAIR_33MHZ, LTDC_PLLSAIDIVR_33MHZ);
    }
    else
    {
        /* other panels intentionally not implemented. */
    }

    /* Default to portrait (480 x 800 logical). */
    ltdc_display_dir(LTDC_DIR_PORTRAIT);

    /* MSP begin */
    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = LTDC_BL_PIN;
    HAL_GPIO_Init(LTDC_BL_PORT, &gpio);
    LTDC_BL_OFF();

    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Alternate = GPIO_AF14_LTDC;

    gpio.Pin = LTDC_DE_PIN;
    HAL_GPIO_Init(LTDC_DE_PORT, &gpio);

    gpio.Pin = LTDC_VSYNC_PIN;
    HAL_GPIO_Init(LTDC_VSYNC_PORT, &gpio);

    gpio.Pin = LTDC_HSYNC_PIN;
    HAL_GPIO_Init(LTDC_HSYNC_PORT, &gpio);

    gpio.Pin = LTDC_CLK_PIN;
    HAL_GPIO_Init(LTDC_CLK_PORT, &gpio);

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOG, &gpio);

    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOH, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
               GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOI, &gpio);
    /* MSP end */

    g_ltdc_handle.Instance         = LTDC;
    g_ltdc_handle.Init.HSPolarity  = LTDC_HSPOLARITY_AL;
    g_ltdc_handle.Init.VSPolarity  = LTDC_VSPOLARITY_AL;
    g_ltdc_handle.Init.DEPolarity  = LTDC_DEPOLARITY_AL;
    g_ltdc_handle.Init.PCPolarity  = LTDC_PCPOLARITY_IPC;
    g_ltdc_handle.Init.HorizontalSync     = (uint32_t)(g_ltdc_hsw - 1U);
    g_ltdc_handle.Init.VerticalSync       = (uint32_t)(g_ltdc_vsw - 1U);
    g_ltdc_handle.Init.AccumulatedHBP     = (uint32_t)(g_ltdc_hsw + g_ltdc_hbp - 1U);
    g_ltdc_handle.Init.AccumulatedVBP     = (uint32_t)(g_ltdc_vsw + g_ltdc_vbp - 1U);
    g_ltdc_handle.Init.AccumulatedActiveW = (uint32_t)(g_ltdc_hsw + g_ltdc_hbp + g_ltdc_pwidth - 1U);
    g_ltdc_handle.Init.AccumulatedActiveH = (uint32_t)(g_ltdc_vsw + g_ltdc_vbp + g_ltdc_pheight - 1U);
    g_ltdc_handle.Init.TotalWidth         = (uint32_t)(g_ltdc_hsw + g_ltdc_hbp + g_ltdc_pwidth + g_ltdc_hfp - 1U);
    g_ltdc_handle.Init.TotalHeigh         = (uint32_t)(g_ltdc_vsw + g_ltdc_vbp + g_ltdc_pheight + g_ltdc_vfp - 1U);
    g_ltdc_handle.Init.Backcolor.Red      = 0U;
    g_ltdc_handle.Init.Backcolor.Green    = 0U;
    g_ltdc_handle.Init.Backcolor.Blue     = 0U;
    g_ltdc_handle.State = HAL_LTDC_STATE_RESET;

    (void)HAL_LTDC_Init(&g_ltdc_handle);

    layer.WindowX0       = 0U;
    layer.WindowY0       = 0U;
    layer.WindowX1       = g_ltdc_pwidth;
    layer.WindowY1       = g_ltdc_pheight;
    layer.PixelFormat    = LTDC_PIXEL_FORMAT_RGB565;
    layer.Alpha          = LTDC_LAYER_ALPHA;
    layer.Alpha0         = LTDC_LAYER_ALPHA0;
    layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    layer.FBStartAdress  = (uint32_t)g_ltdc_framebuf;
    layer.ImageWidth     = g_ltdc_pwidth;
    layer.ImageHeight    = g_ltdc_pheight;
    layer.Backcolor.Red   = 0U;
    layer.Backcolor.Green = 0U;
    layer.Backcolor.Blue  = 0U;

    (void)HAL_LTDC_ConfigLayer(&g_ltdc_handle, &layer, LTDC_LAYER_INDEX);

    ltdc_clear(WHITE);
    LTDC_BL_ON();
}
