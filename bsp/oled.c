/**
 * @file    oled.c
 * @brief   SSD1306 128x64 OLED driver over the 8080 8-bit parallel bus.
 */

#include "stm32f4xx_hal.h"
#include "oled.h"
#include "oledfont.h"
#include "delay.h"

/* Interface selection: only the 8080 parallel path is implemented. */
#define OLED_IF_SELECT  OLED_IF_8080

/* 8080 parallel bus control pins. */
#define OLED_RST_PORT   GPIOA
#define OLED_RST_PIN    GPIO_PIN_15

#define OLED_CS_PORT    GPIOB
#define OLED_CS_PIN     GPIO_PIN_7

#define OLED_RS_PORT    GPIOB
#define OLED_RS_PIN     GPIO_PIN_4

#define OLED_WR_PORT    GPIOH
#define OLED_WR_PIN     GPIO_PIN_8

#define OLED_RD_PORT    GPIOB
#define OLED_RD_PIN     GPIO_PIN_3

/* Data bus: D0-D3 -> PC6-PC9, D4 -> PC11, D5 -> PD3, D6-D7 -> PB8-PB9. */
#define OLED_DAT_D0_D3_MASK   0x03C0U
#define OLED_DAT_D0_D3_SHIFT  6U
#define OLED_DAT_D4_MASK      0x0800U
#define OLED_DAT_D4_SHIFT     11U
#define OLED_DAT_D5_MASK      0x0008U
#define OLED_DAT_D5_SHIFT     3U
#define OLED_DAT_D6_D7_MASK   0x0300U
#define OLED_DAT_D6_D7_SHIFT  8U

/* SSD1306 command bytes. */
typedef enum
{
    OLED_CMD_DISPLAY_OFF    = 0xAE,
    OLED_CMD_CLK_DIV        = 0xD5,
    OLED_CMD_MULTIPLEX      = 0xA8,
    OLED_CMD_DISPLAY_OFFSET = 0xD3,
    OLED_CMD_START_LINE     = 0x40,
    OLED_CMD_CHARGE_PUMP    = 0x8D,
    OLED_CMD_MEMORY_MODE    = 0x20,
    OLED_CMD_SEG_REMAP      = 0xA1,
    OLED_CMD_COM_SCAN_DIR   = 0xC8,
    OLED_CMD_COM_PINS       = 0xDA,
    OLED_CMD_CONTRAST       = 0x81,
    OLED_CMD_PRECHARGE      = 0xD9,
    OLED_CMD_VCOMH          = 0xDB,
    OLED_CMD_ENTIRE_ON      = 0xA4,
    OLED_CMD_NORMAL_DISPLAY = 0xA6,
    OLED_CMD_DISPLAY_ON     = 0xAF,
    OLED_CMD_LOW_COLUMN     = 0x00,
    OLED_CMD_HIGH_COLUMN    = 0x10,
    OLED_CMD_PAGE_ADDR      = 0xB0
} oled_cmd_t;

/* SSD1306 command parameters. */
#define OLED_CLK_DIV_VALUE       0x50U
#define OLED_MULTIPLEX_VALUE     0x3FU
#define OLED_OFFSET_VALUE        0x00U
#define OLED_START_LINE_VALUE    0x40U
#define OLED_CHARGE_PUMP_ENABLE  0x14U
#define OLED_CHARGE_PUMP_DISABLE 0x10U
#define OLED_MEMORY_MODE_PAGE    0x02U
#define OLED_COM_PINS_VALUE      0x12U
#define OLED_CONTRAST_VALUE      0xEFU
#define OLED_PRECHARGE_VALUE     0xF1U
#define OLED_VCOMH_VALUE         0x30U

/* Panel geometry. */
#define OLED_WIDTH  128U
#define OLED_HEIGHT 64U
#define OLED_PAGES  (OLED_HEIGHT / 8U)

/* Character metrics. */
#define OLED_6X8_WIDTH   6U
#define OLED_8X16_WIDTH  8U
#define OLED_ASCII_FIRST 0x20U
#define OLED_ASCII_LAST  0x7EU

typedef enum
{
    OLED_ARG_CMD  = 0,
    OLED_ARG_DATA = 1
} oled_arg_t;

static uint8_t g_oled_gram[OLED_WIDTH][OLED_PAGES];

static void oled_wr_byte(uint8_t data, uint8_t arg);
static void oled_draw_point(uint8_t x, uint8_t y, uint8_t dot);
static void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size);

static void oled_data_out(uint8_t data)
{
    GPIOC->ODR = (GPIOC->ODR & ~OLED_DAT_D0_D3_MASK) |
                 ((uint32_t)(data & 0x0FU) << OLED_DAT_D0_D3_SHIFT);
    GPIOC->ODR = (GPIOC->ODR & ~OLED_DAT_D4_MASK) |
                 ((uint32_t)((data >> 4) & 0x01U) << OLED_DAT_D4_SHIFT);
    GPIOD->ODR = (GPIOD->ODR & ~OLED_DAT_D5_MASK) |
                 ((uint32_t)((data >> 5) & 0x01U) << OLED_DAT_D5_SHIFT);
    GPIOB->ODR = (GPIOB->ODR & ~OLED_DAT_D6_D7_MASK) |
                 ((uint32_t)((data >> 6) & 0x03U) << OLED_DAT_D6_D7_SHIFT);
}

static void oled_wr_byte(uint8_t data, uint8_t arg)
{
    if (OLED_IF_SELECT == OLED_IF_8080)
    {
        oled_data_out(data);

        HAL_GPIO_WritePin(OLED_RS_PORT, OLED_RS_PIN,
                          (arg != (uint8_t)OLED_ARG_CMD) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(OLED_WR_PORT, OLED_WR_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(OLED_WR_PORT, OLED_WR_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(OLED_RS_PORT, OLED_RS_PIN, GPIO_PIN_SET);
    }
    else
    {
        /* SPI path intentionally left unimplemented. */
    }
}

static void oled_draw_point(uint8_t x, uint8_t y, uint8_t dot)
{
    uint8_t page;
    uint8_t bit;

    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT))
    {
        return;
    }

    page = (uint8_t)(y / 8U);
    bit  = (uint8_t)(1U << (y % 8U));

    if (dot != 0U)
    {
        g_oled_gram[x][page] |= bit;
    }
    else
    {
        g_oled_gram[x][page] &= (uint8_t)(~bit);
    }
}

static uint8_t oled_char_width(uint8_t size)
{
    return (size == (uint8_t)OLED_FONT_6X8) ? OLED_6X8_WIDTH : OLED_8X16_WIDTH;
}

static uint8_t oled_font_byte_bit(uint8_t byte, uint8_t row)
{
    return (uint8_t)((byte & (uint8_t)(0x80U >> row)) ? 1U : 0U);
}

static void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size)
{
    uint8_t idx;
    uint8_t col;
    uint8_t row;
    uint8_t font_byte;

    if ((chr < OLED_ASCII_FIRST) || (chr > OLED_ASCII_LAST))
    {
        return;
    }

    idx = (uint8_t)(chr - OLED_ASCII_FIRST);

    if (size == (uint8_t)OLED_FONT_6X8)
    {
        for (col = 0; col < OLED_6X8_WIDTH; col++)
        {
            font_byte = oled_font_6x8[idx][col];
            for (row = 0; row < 8U; row++)
            {
                oled_draw_point((uint8_t)(x + col), (uint8_t)(y + row),
                                oled_font_byte_bit(font_byte, row));
            }
        }
    }
    else if (size == (uint8_t)OLED_FONT_8X16)
    {
        for (col = 0; col < OLED_8X16_WIDTH; col++)
        {
            font_byte = oled_asc2_1608[idx][2U * col];
            for (row = 0; row < 8U; row++)
            {
                oled_draw_point((uint8_t)(x + col), (uint8_t)(y + row),
                                oled_font_byte_bit(font_byte, row));
            }

            font_byte = oled_asc2_1608[idx][(2U * col) + 1U];
            for (row = 0; row < 8U; row++)
            {
                oled_draw_point((uint8_t)(x + col), (uint8_t)(y + 8U + row),
                                oled_font_byte_bit(font_byte, row));
            }
        }
    }
    else
    {
        /* Unsupported font height. */
    }
}

void oled_refresh(void)
{
    uint8_t page;
    uint8_t col;

    for (page = 0; page < OLED_PAGES; page++)
    {
        oled_wr_byte((uint8_t)(OLED_CMD_PAGE_ADDR + page), OLED_ARG_CMD);
        oled_wr_byte(OLED_CMD_LOW_COLUMN, OLED_ARG_CMD);
        oled_wr_byte(OLED_CMD_HIGH_COLUMN, OLED_ARG_CMD);

        for (col = 0; col < OLED_WIDTH; col++)
        {
            oled_wr_byte(g_oled_gram[col][page], OLED_ARG_DATA);
        }
    }
}

void oled_display_on(void)
{
    oled_wr_byte(OLED_CMD_CHARGE_PUMP, OLED_ARG_CMD);
    oled_wr_byte(OLED_CHARGE_PUMP_ENABLE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_DISPLAY_ON, OLED_ARG_CMD);
}

void oled_display_off(void)
{
    oled_wr_byte(OLED_CMD_CHARGE_PUMP, OLED_ARG_CMD);
    oled_wr_byte(OLED_CHARGE_PUMP_DISABLE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_DISPLAY_OFF, OLED_ARG_CMD);
}

void oled_clear(void)
{
    uint8_t page;
    uint8_t col;

    for (page = 0; page < OLED_PAGES; page++)
    {
        for (col = 0; col < OLED_WIDTH; col++)
        {
            g_oled_gram[col][page] = 0x00U;
        }
    }

    oled_refresh();
}

void oled_show_string(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    uint8_t width = oled_char_width(size);

    while ((*str >= (char)OLED_ASCII_FIRST) && (*str <= (char)OLED_ASCII_LAST))
    {
        if (x > (uint8_t)(OLED_WIDTH - width))
        {
            x = 0U;
            y = (uint8_t)(y + size);
        }

        if (y > (uint8_t)(OLED_HEIGHT - size))
        {
            x = 0U;
            y = 0U;
            oled_clear();
        }

        oled_show_char(x, y, (uint8_t)*str, size);
        x = (uint8_t)(x + width);
        str++;
    }
}

static uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1U;

    while (n-- != 0U)
    {
        result *= m;
    }

    return result;
}

void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t width = oled_char_width(size);
    uint8_t t;
    uint8_t digit;
    uint8_t enshow = 0U;

    for (t = 0; t < len; t++)
    {
        digit = (uint8_t)((num / oled_pow(10U, (uint8_t)(len - t - 1U))) % 10U);

        if ((enshow == 0U) && (t < (uint8_t)(len - 1U)))
        {
            if (digit == 0U)
            {
                oled_show_char((uint8_t)(x + (width * t)), y, (uint8_t)' ', size);
                continue;
            }

            enshow = 1U;
        }

        oled_show_char((uint8_t)(x + (width * t)), y, (uint8_t)('0' + digit), size);
    }
}

void oled_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* MSP begin */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = OLED_RST_PIN;
    HAL_GPIO_Init(OLED_RST_PORT, &gpio);

    gpio.Pin = OLED_CS_PIN | OLED_RS_PIN | OLED_RD_PIN | GPIO_PIN_8 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = OLED_WR_PIN;
    HAL_GPIO_Init(OLED_WR_PORT, &gpio);

    HAL_GPIO_WritePin(OLED_WR_PORT, OLED_WR_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_RD_PORT, OLED_RD_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_RS_PORT, OLED_RS_PIN, GPIO_PIN_SET);
    /* MSP end */

    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_RESET);
    delay_ms(100);
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_SET);

    oled_wr_byte(OLED_CMD_DISPLAY_OFF, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_CLK_DIV, OLED_ARG_CMD);
    oled_wr_byte(OLED_CLK_DIV_VALUE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_MULTIPLEX, OLED_ARG_CMD);
    oled_wr_byte(OLED_MULTIPLEX_VALUE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_DISPLAY_OFFSET, OLED_ARG_CMD);
    oled_wr_byte(OLED_OFFSET_VALUE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_START_LINE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_CHARGE_PUMP, OLED_ARG_CMD);
    oled_wr_byte(OLED_CHARGE_PUMP_ENABLE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_MEMORY_MODE, OLED_ARG_CMD);
    oled_wr_byte(OLED_MEMORY_MODE_PAGE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_SEG_REMAP, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_COM_SCAN_DIR, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_COM_PINS, OLED_ARG_CMD);
    oled_wr_byte(OLED_COM_PINS_VALUE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_CONTRAST, OLED_ARG_CMD);
    oled_wr_byte(OLED_CONTRAST_VALUE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_PRECHARGE, OLED_ARG_CMD);
    oled_wr_byte(OLED_PRECHARGE_VALUE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_VCOMH, OLED_ARG_CMD);
    oled_wr_byte(OLED_VCOMH_VALUE, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_ENTIRE_ON, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_NORMAL_DISPLAY, OLED_ARG_CMD);
    oled_wr_byte(OLED_CMD_DISPLAY_ON, OLED_ARG_CMD);

    oled_clear();
}
