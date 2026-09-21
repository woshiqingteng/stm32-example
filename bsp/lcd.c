/**
 * @file    lcd.c
 * @brief   Unified screen driver.
 *
 * MCU screen: SSD1963 controller on the FMC 8080 bus (implemented here).
 * RGB screen: forwarded to the lower-level LTDC driver (ltdc.c).
 * lcd_init() detects which panel is attached.
 */

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "lcd.h"
#include "lcdfont.h"
#include "delay.h"

/* FMC control pins and backlight (MCU screen). */
#define LCD_CS_PORT GPIOD
#define LCD_CS_PIN  GPIO_PIN_7

#define LCD_WR_PORT GPIOD
#define LCD_WR_PIN  GPIO_PIN_5

#define LCD_RD_PORT GPIOD
#define LCD_RD_PIN  GPIO_PIN_4

#define LCD_RS_PORT GPIOD
#define LCD_RS_PIN  GPIO_PIN_13

#define LCD_BL_PORT GPIOB
#define LCD_BL_PIN  GPIO_PIN_5

#define LCD_BL_ON()  HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_SET)
#define LCD_BL_OFF() HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_RESET)

/* SSD1963 command set. */
typedef enum
{
    SSD1963_SOFT_RESET   = 0x01,
    SSD1963_DISPLAY_ON   = 0x29,
    SSD1963_SET_COLUMN   = 0x2A,
    SSD1963_SET_PAGE     = 0x2B,
    SSD1963_WRITE_MEM    = 0x2C,
    SSD1963_ADDR_MODE    = 0x36,
    SSD1963_READ_ID      = 0xA1,
    SSD1963_LCD_MODE     = 0xB0,
    SSD1963_HOR_PERIOD   = 0xB4,
    SSD1963_VER_PERIOD   = 0xB6,
    SSD1963_GPIO_CFG     = 0xB8,
    SSD1963_GPIO_CTRL    = 0xBA,
    SSD1963_PWM_CFG      = 0xBE,
    SSD1963_DBC_CTRL     = 0xD0,
    SSD1963_PLL_ENABLE   = 0xE0,
    SSD1963_PLL_CONFIG   = 0xE2,
    SSD1963_PIXEL_FREQ   = 0xE6,
    SSD1963_CPU_IF       = 0xF0
} ssd1963_cmd_t;

/* SSD1963 command parameters (openedv defaults). */
#define SSD1963_ID_5761       0x5761U
#define SSD1963_ID_1963       0x1963U
#define SSD1963_PLL_MULT      0x1DU
#define SSD1963_PLL_DIV       0x02U
#define SSD1963_PLL_VALIDATE  0x04U
#define SSD1963_PLL_ENABLE_A  0x01U
#define SSD1963_PLL_ENABLE_B  0x03U
#define SSD1963_PIXEL_FREQ_0  0x2FU
#define SSD1963_PIXEL_FREQ_1  0xFFU
#define SSD1963_MODE_24BIT    0x20U
#define SSD1963_MODE_TFT      0x00U
#define SSD1963_MODE_RGB_SEQ  0x00U
#define SSD1963_CPU_IF_16BIT  0x03U
#define SSD1963_DBC_DISABLE   0x00U
#define SSD1963_PWM_FREQ      0x05U
#define SSD1963_PWM_DUTY      0xFEU
#define SSD1963_PWM_C         0x01U
#define SSD1963_PWM_D         0x00U
#define SSD1963_PWM_E         0x00U
#define SSD1963_PWM_F         0x00U
#define SSD1963_GPIO_NUM      0x03U
#define SSD1963_GPIO_NORMAL   0x01U
#define SSD1963_GPIO_LCD_DIR  0x01U
#define SSD1963_ADDR_L2R_U2D  0x00U

#define SSD1963_PLL_STABILIZE_US 100U
#define SSD1963_PLL_ENABLE_MS    10U
#define SSD1963_PLL_SWITCH_MS    12U
#define SSD1963_RESET_MS         10U
#define LCD_POWER_ON_DELAY_MS    50U

/* 8x16 font metrics. */
#define LCD_FONT_8X16   16U
#define LCD_CHAR_WIDTH  8U
#define LCD_ASCII_FIRST 0x20U
#define LCD_ASCII_LAST  0x7EU

/** @brief  Which screen is attached. */
typedef enum
{
    LCD_MODE_MCU = 0,
    LCD_MODE_RGB = 1
} lcd_mode_t;

static lcd_mode_t g_lcd_mode = LCD_MODE_MCU;
static uint16_t g_lcd_rgb_id = 0U;

/* MCU screen state. */
static SRAM_HandleTypeDef g_lcd_sram_handle;
static uint16_t g_mcu_id = SSD1963_ID_1963;
static uint8_t g_mcu_dir = LCD_DIR_PORTRAIT;
static uint16_t g_mcu_width = LCD_SSD_PANEL_VER;  /* portrait: 480 */
static uint16_t g_mcu_height = LCD_SSD_PANEL_HOR; /* portrait: 800 */

static void lcd_wr_data(uint16_t data)
{
    LCD->LCD_RAM = data;
}

static void lcd_wr_regno(uint16_t regno)
{
    LCD->LCD_REG = regno;
}

static uint16_t lcd_rd_data(void)
{
    volatile uint16_t ram;

    ram = LCD->LCD_RAM;
    return ram;
}

static void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey)
{
    uint16_t xcmd = SSD1963_SET_COLUMN;
    uint16_t ycmd = SSD1963_SET_PAGE;

    if (g_mcu_dir == LCD_DIR_PORTRAIT)
    {
        /* Portrait: the native column axis is drawn as the page axis. */
        xcmd = SSD1963_SET_PAGE;
        ycmd = SSD1963_SET_COLUMN;
    }

    lcd_wr_regno(xcmd);
    lcd_wr_data((uint16_t)(sx >> 8));
    lcd_wr_data((uint16_t)(sx & 0xFFU));
    lcd_wr_data((uint16_t)(ex >> 8));
    lcd_wr_data((uint16_t)(ex & 0xFFU));

    lcd_wr_regno(ycmd);
    lcd_wr_data((uint16_t)(sy >> 8));
    lcd_wr_data((uint16_t)(sy & 0xFFU));
    lcd_wr_data((uint16_t)(ey >> 8));
    lcd_wr_data((uint16_t)(ey & 0xFFU));

    lcd_wr_regno(SSD1963_WRITE_MEM);
}

static void lcd_bus_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    FMC_NORSRAM_TimingTypeDef read_timing = {0};
    FMC_NORSRAM_TimingTypeDef write_timing = {0};

    /* MSP begin */
    __HAL_RCC_FMC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF12_FMC;

    gpio.Pin = LCD_CS_PIN | LCD_WR_PIN | LCD_RD_PIN | LCD_RS_PIN |
               GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
               GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
               GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &gpio);

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Pin  = LCD_BL_PIN;
    HAL_GPIO_Init(LCD_BL_PORT, &gpio);
    LCD_BL_OFF();
    /* MSP end */

    g_lcd_sram_handle.Instance        = FMC_NORSRAM_DEVICE;
    g_lcd_sram_handle.Extended        = FMC_NORSRAM_EXTENDED_DEVICE;
    g_lcd_sram_handle.Init.NSBank             = FMC_NORSRAM_BANK1;
    g_lcd_sram_handle.Init.DataAddressMux     = FMC_DATA_ADDRESS_MUX_DISABLE;
    g_lcd_sram_handle.Init.MemoryType         = FMC_MEMORY_TYPE_SRAM;
    g_lcd_sram_handle.Init.MemoryDataWidth    = FMC_NORSRAM_MEM_BUS_WIDTH_16;
    g_lcd_sram_handle.Init.BurstAccessMode    = FMC_BURST_ACCESS_MODE_DISABLE;
    g_lcd_sram_handle.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_LOW;
    g_lcd_sram_handle.Init.WaitSignalActive   = FMC_WAIT_TIMING_BEFORE_WS;
    g_lcd_sram_handle.Init.WriteOperation     = FMC_WRITE_OPERATION_ENABLE;
    g_lcd_sram_handle.Init.WaitSignal         = FMC_WAIT_SIGNAL_DISABLE;
    g_lcd_sram_handle.Init.ExtendedMode       = FMC_EXTENDED_MODE_ENABLE;
    g_lcd_sram_handle.Init.AsynchronousWait   = FMC_ASYNCHRONOUS_WAIT_DISABLE;
    g_lcd_sram_handle.Init.WriteBurst         = FMC_WRITE_BURST_DISABLE;
    g_lcd_sram_handle.Init.ContinuousClock    = FMC_CONTINUOUS_CLOCK_SYNC_ASYNC;

    read_timing.AddressSetupTime = 0x0FU;
    read_timing.AddressHoldTime  = 0x00U;
    read_timing.DataSetupTime    = 0x46U;
    read_timing.AccessMode       = FMC_ACCESS_MODE_A;

    write_timing.AddressSetupTime = 0x0FU;
    write_timing.AddressHoldTime  = 0x00U;
    write_timing.DataSetupTime    = 0x0FU;
    write_timing.AccessMode       = FMC_ACCESS_MODE_A;

    (void)HAL_SRAM_Init(&g_lcd_sram_handle, &read_timing, &write_timing);
    delay_ms(LCD_POWER_ON_DELAY_MS);
}

static void lcd_bus_speedup(void)
{
    FMC_NORSRAM_TimingTypeDef write_timing = {0};

    write_timing.AddressSetupTime = 0x02U;
    write_timing.AddressHoldTime  = 0x00U;
    write_timing.DataSetupTime    = 0x02U;
    write_timing.AccessMode       = FMC_ACCESS_MODE_A;

    (void)FMC_NORSRAM_Extended_Timing_Init(g_lcd_sram_handle.Extended, &write_timing,
                                           g_lcd_sram_handle.Init.NSBank,
                                           g_lcd_sram_handle.Init.ExtendedMode);
}

static void lcd_read_id(void)
{
    lcd_wr_regno(SSD1963_READ_ID);
    (void)lcd_rd_data();
    g_mcu_id = lcd_rd_data();
    g_mcu_id <<= 8;
    g_mcu_id |= lcd_rd_data();

    if (g_mcu_id == SSD1963_ID_5761)
    {
        g_mcu_id = SSD1963_ID_1963;
    }
}

static void lcd_ssd1963_reginit(void)
{
    lcd_wr_regno(SSD1963_PLL_CONFIG);
    lcd_wr_data(SSD1963_PLL_MULT);
    lcd_wr_data(SSD1963_PLL_DIV);
    lcd_wr_data(SSD1963_PLL_VALIDATE);
    delay_us(SSD1963_PLL_STABILIZE_US);

    lcd_wr_regno(SSD1963_PLL_ENABLE);
    lcd_wr_data(SSD1963_PLL_ENABLE_A);
    delay_ms(SSD1963_PLL_ENABLE_MS);

    lcd_wr_regno(SSD1963_PLL_ENABLE);
    lcd_wr_data(SSD1963_PLL_ENABLE_B);
    delay_ms(SSD1963_PLL_SWITCH_MS);

    lcd_wr_regno(SSD1963_SOFT_RESET);
    delay_ms(SSD1963_RESET_MS);

    lcd_wr_regno(SSD1963_PIXEL_FREQ);
    lcd_wr_data(SSD1963_PIXEL_FREQ_0);
    lcd_wr_data(SSD1963_PIXEL_FREQ_1);
    lcd_wr_data(SSD1963_PIXEL_FREQ_1);

    lcd_wr_regno(SSD1963_LCD_MODE);
    lcd_wr_data(SSD1963_MODE_24BIT);
    lcd_wr_data(SSD1963_MODE_TFT);
    lcd_wr_data((uint16_t)((LCD_SSD_PANEL_HOR - 1U) >> 8));
    lcd_wr_data((uint16_t)(LCD_SSD_PANEL_HOR - 1U));
    lcd_wr_data((uint16_t)((LCD_SSD_PANEL_VER - 1U) >> 8));
    lcd_wr_data((uint16_t)(LCD_SSD_PANEL_VER - 1U));
    lcd_wr_data(SSD1963_MODE_RGB_SEQ);

    lcd_wr_regno(SSD1963_HOR_PERIOD);
    lcd_wr_data((uint16_t)((LCD_SSD_HT - 1U) >> 8));
    lcd_wr_data((uint16_t)(LCD_SSD_HT - 1U));
    lcd_wr_data((uint16_t)(LCD_SSD_HPS >> 8));
    lcd_wr_data((uint16_t)LCD_SSD_HPS);
    lcd_wr_data((uint16_t)(LCD_SSD_HOR_PULSE_WIDTH - 1U));
    lcd_wr_data(0x00U);
    lcd_wr_data(0x00U);
    lcd_wr_data(0x00U);

    lcd_wr_regno(SSD1963_VER_PERIOD);
    lcd_wr_data((uint16_t)((LCD_SSD_VT - 1U) >> 8));
    lcd_wr_data((uint16_t)(LCD_SSD_VT - 1U));
    lcd_wr_data((uint16_t)(LCD_SSD_VPS >> 8));
    lcd_wr_data((uint16_t)LCD_SSD_VPS);
    lcd_wr_data((uint16_t)(LCD_SSD_VER_FRONT_PORCH - 1U));
    lcd_wr_data(0x00U);
    lcd_wr_data(0x00U);

    lcd_wr_regno(SSD1963_CPU_IF);
    lcd_wr_data(SSD1963_CPU_IF_16BIT);

    lcd_wr_regno(SSD1963_DISPLAY_ON);

    lcd_wr_regno(SSD1963_DBC_CTRL);
    lcd_wr_data(SSD1963_DBC_DISABLE);

    lcd_wr_regno(SSD1963_PWM_CFG);
    lcd_wr_data(SSD1963_PWM_FREQ);
    lcd_wr_data(SSD1963_PWM_DUTY);
    lcd_wr_data(SSD1963_PWM_C);
    lcd_wr_data(SSD1963_PWM_D);
    lcd_wr_data(SSD1963_PWM_E);
    lcd_wr_data(SSD1963_PWM_F);

    lcd_wr_regno(SSD1963_GPIO_CFG);
    lcd_wr_data(SSD1963_GPIO_NUM);
    lcd_wr_data(SSD1963_GPIO_NORMAL);

    lcd_wr_regno(SSD1963_GPIO_CTRL);
    lcd_wr_data(SSD1963_GPIO_LCD_DIR);

    lcd_wr_regno(SSD1963_ADDR_MODE);
    lcd_wr_data(SSD1963_ADDR_L2R_U2D);
}

void lcd_display_dir(uint8_t dir)
{
    if (g_lcd_mode == LCD_MODE_RGB)
    {
        ltdc_display_dir(dir);
        return;
    }

    g_mcu_dir = dir;

    if (dir == LCD_DIR_PORTRAIT)
    {
        g_mcu_width  = LCD_SSD_PANEL_VER;
        g_mcu_height = LCD_SSD_PANEL_HOR;
    }
    else
    {
        g_mcu_width  = LCD_SSD_PANEL_HOR;
        g_mcu_height = LCD_SSD_PANEL_VER;
    }
}

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    if (g_lcd_mode == LCD_MODE_RGB)
    {
        ltdc_draw_point(x, y, color);
        return;
    }

    if ((x >= g_mcu_width) || (y >= g_mcu_height))
    {
        return;
    }

    lcd_set_window(x, y, x, y);
    lcd_wr_data(color);
}

void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    uint32_t count;
    uint32_t index;

    if (g_lcd_mode == LCD_MODE_RGB)
    {
        ltdc_fill(sx, sy, ex, ey, color);
        return;
    }

    if ((sx > ex) || (sy > ey))
    {
        return;
    }

    if (ex >= g_mcu_width)
    {
        ex = g_mcu_width - 1U;
    }

    if (ey >= g_mcu_height)
    {
        ey = g_mcu_height - 1U;
    }

    count = ((uint32_t)(ex - sx) + 1U) * ((uint32_t)(ey - sy) + 1U);

    lcd_set_window(sx, sy, ex, ey);
    for (index = 0; index < count; index++)
    {
        lcd_wr_data(color);
    }
}

void lcd_clear(uint16_t color)
{
    if (g_lcd_mode == LCD_MODE_RGB)
    {
        ltdc_clear(color);
        return;
    }

    lcd_fill(0U, 0U, (uint16_t)(g_mcu_width - 1U), (uint16_t)(g_mcu_height - 1U), color);
}

static void lcd_show_char(uint16_t x, uint16_t y, char chr, uint16_t color)
{
    const uint8_t *pfont;
    uint16_t y0 = y;
    uint8_t t;
    uint8_t t1;
    uint8_t temp;

    if ((chr < (char)LCD_ASCII_FIRST) || (chr > (char)LCD_ASCII_LAST))
    {
        return;
    }

    pfont = (const uint8_t *)asc2_1608[(uint8_t)chr - LCD_ASCII_FIRST];

    for (t = 0; t < LCD_FONT_8X16; t++)
    {
        temp = pfont[t];

        for (t1 = 0; t1 < 8U; t1++)
        {
            if ((temp & 0x80U) != 0U)
            {
                lcd_draw_point(x, y, color);
            }

            temp <<= 1;
            y++;

            if ((uint16_t)(y - y0) == LCD_FONT_8X16)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}

static uint32_t lcd_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1U;

    while (n-- != 0U)
    {
        result *= m;
    }

    return result;
}

void lcd_show_string(uint16_t x, uint16_t y, const char *str, uint8_t size, uint16_t color)
{
    if (g_lcd_mode == LCD_MODE_RGB)
    {
        ltdc_show_string(x, y, str, size, color);
        return;
    }

    if (size != LCD_FONT_8X16)
    {
        return;
    }

    while ((*str >= (char)LCD_ASCII_FIRST) && (*str <= (char)LCD_ASCII_LAST))
    {
        if (x > (g_mcu_width - LCD_CHAR_WIDTH))
        {
            x = 0U;
            y += LCD_FONT_8X16;
        }

        lcd_show_char(x, y, *str, color);
        x += LCD_CHAR_WIDTH;
        str++;
    }
}

void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t;
    uint8_t digit;
    uint8_t enshow = 0U;

    if (g_lcd_mode == LCD_MODE_RGB)
    {
        ltdc_show_num(x, y, num, len, size, color);
        return;
    }

    if (size != LCD_FONT_8X16)
    {
        return;
    }

    for (t = 0; t < len; t++)
    {
        digit = (uint8_t)((num / lcd_pow(10U, (uint8_t)(len - t - 1U))) % 10U);

        if ((enshow == 0U) && (t < (uint8_t)(len - 1U)))
        {
            if (digit == 0U)
            {
                lcd_show_char((uint16_t)(x + (LCD_CHAR_WIDTH * t)), y, ' ', color);
                continue;
            }

            enshow = 1U;
        }

        lcd_show_char((uint16_t)(x + (LCD_CHAR_WIDTH * t)), y, (char)('0' + digit), color);
    }
}

uint16_t lcd_get_id(void)
{
    if (g_lcd_mode == LCD_MODE_RGB)
    {
        return g_lcd_rgb_id;
    }

    return g_mcu_id;
}

void lcd_init(void)
{
    g_lcd_rgb_id = ltdc_panelid_read();

    if (g_lcd_rgb_id == LTDC_PANEL_ID_4384)
    {
        g_lcd_mode = LCD_MODE_RGB;
        ltdc_init();
        g_mcu_id = 0U;
    }
    else
    {
        g_lcd_mode = LCD_MODE_MCU;
        lcd_bus_init();
        lcd_read_id();

        if (g_mcu_id == SSD1963_ID_1963)
        {
            lcd_ssd1963_reginit();
        }
        else
        {
            /* other controllers intentionally not implemented. */
        }

        lcd_bus_speedup();
        LCD_BL_ON();
    }

    lcd_display_dir(LCD_DIR_PORTRAIT);
    lcd_clear(WHITE);
}
