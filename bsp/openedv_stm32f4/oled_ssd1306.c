/**
 * @file    oled_ssd1306.c
 * @brief   SSD1306 controller protocol plus the physical bus transport.
 *
 * The transport is selected at compile time with OLED_BUS (oled_ssd1306.h):
 * 8080 parallel (default), 4-wire SPI, or I2C (placeholder). The controller
 * framing (command set, page/column addressing, flush) is bus independent.
 */

#include "stm32f4xx_hal.h"
#include "oled_ssd1306.h"
#include "delay.h"

/* Command/data flag for one bus byte. */
typedef enum
{
    OLED_ARG_CMD  = 0,
    OLED_ARG_DATA = 1
} oled_arg_t;

#define SSD1306_RESET_DELAY_MS 100U

/* Reset line is shared by every interface (active low). */
#define SSD1306_RST_PORT GPIOA
#define SSD1306_RST_PIN  GPIO_PIN_15

/* ============================ bus transport ============================ */

#if OLED_BUS == OLED_BUS_8080

/* 8080 parallel bus control pins. */
#define OLED_CS_PORT    GPIOB
#define OLED_CS_PIN     GPIO_PIN_7

#define OLED_RS_PORT    GPIOB
#define OLED_RS_PIN     GPIO_PIN_4

#define OLED_WR_PORT    GPIOH
#define OLED_WR_PIN     GPIO_PIN_8

#define OLED_RD_PORT    GPIOB
#define OLED_RD_PIN     GPIO_PIN_3

/* Data bus: D0-D7 -> PC6, PC7, PC8, PC9, PC11, PD3, PB8, PB9. */
#define OLED_D0_PORT    GPIOC
#define OLED_D0_PIN     GPIO_PIN_6
#define OLED_D1_PORT    GPIOC
#define OLED_D1_PIN     GPIO_PIN_7
#define OLED_D2_PORT    GPIOC
#define OLED_D2_PIN     GPIO_PIN_8
#define OLED_D3_PORT    GPIOC
#define OLED_D3_PIN     GPIO_PIN_9
#define OLED_D4_PORT    GPIOC
#define OLED_D4_PIN     GPIO_PIN_11
#define OLED_D5_PORT    GPIOD
#define OLED_D5_PIN     GPIO_PIN_3
#define OLED_D6_PORT    GPIOB
#define OLED_D6_PIN     GPIO_PIN_8
#define OLED_D7_PORT    GPIOB
#define OLED_D7_PIN     GPIO_PIN_9

/* Data bus init groups (per GPIO port). */
#define OLED_DATA_GPIOB_PINS (OLED_D6_PIN | OLED_D7_PIN)
#define OLED_DATA_GPIOC_PINS (OLED_D0_PIN | OLED_D1_PIN | OLED_D2_PIN | OLED_D3_PIN | OLED_D4_PIN)
#define OLED_DATA_GPIOD_PINS (OLED_D5_PIN)

#define OLED_DATA_BIT(data, bit, port, pin) \
    HAL_GPIO_WritePin((port), (pin), ((((data) >> (bit)) & 0x01U) != 0U) ? \
                      GPIO_PIN_SET : GPIO_PIN_RESET)

static void oled_data_out(uint8_t data)
{
    OLED_DATA_BIT(data, 0U, OLED_D0_PORT, OLED_D0_PIN);
    OLED_DATA_BIT(data, 1U, OLED_D1_PORT, OLED_D1_PIN);
    OLED_DATA_BIT(data, 2U, OLED_D2_PORT, OLED_D2_PIN);
    OLED_DATA_BIT(data, 3U, OLED_D3_PORT, OLED_D3_PIN);
    OLED_DATA_BIT(data, 4U, OLED_D4_PORT, OLED_D4_PIN);
    OLED_DATA_BIT(data, 5U, OLED_D5_PORT, OLED_D5_PIN);
    OLED_DATA_BIT(data, 6U, OLED_D6_PORT, OLED_D6_PIN);
    OLED_DATA_BIT(data, 7U, OLED_D7_PORT, OLED_D7_PIN);
}

static void oled_bus_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    /* Reset line first. */
    gpio.Pin = SSD1306_RST_PIN;
    HAL_GPIO_Init(SSD1306_RST_PORT, &gpio);

    /* Control lines. */
    gpio.Pin = OLED_CS_PIN;
    HAL_GPIO_Init(OLED_CS_PORT, &gpio);
    gpio.Pin = OLED_RS_PIN;
    HAL_GPIO_Init(OLED_RS_PORT, &gpio);
    gpio.Pin = OLED_RD_PIN;
    HAL_GPIO_Init(OLED_RD_PORT, &gpio);
    gpio.Pin = OLED_WR_PIN;
    HAL_GPIO_Init(OLED_WR_PORT, &gpio);

    /* Data bus: D0-D7 (GPIOB/C/D). */
    gpio.Pin = OLED_DATA_GPIOB_PINS;
    HAL_GPIO_Init(OLED_D6_PORT, &gpio);
    gpio.Pin = OLED_DATA_GPIOC_PINS;
    HAL_GPIO_Init(OLED_D0_PORT, &gpio);
    gpio.Pin = OLED_DATA_GPIOD_PINS;
    HAL_GPIO_Init(OLED_D5_PORT, &gpio);

    HAL_GPIO_WritePin(OLED_WR_PORT, OLED_WR_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_RD_PORT, OLED_RD_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_RS_PORT, OLED_RS_PIN, GPIO_PIN_SET);

    HAL_GPIO_WritePin(SSD1306_RST_PORT, SSD1306_RST_PIN, GPIO_PIN_RESET);
    delay_ms(SSD1306_RESET_DELAY_MS);
    HAL_GPIO_WritePin(SSD1306_RST_PORT, SSD1306_RST_PIN, GPIO_PIN_SET);
}

static void oled_bus_write(uint8_t data, oled_arg_t arg)
{
    oled_data_out(data);

    HAL_GPIO_WritePin(OLED_RS_PORT, OLED_RS_PIN,
                      (arg != OLED_ARG_CMD) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OLED_WR_PORT, OLED_WR_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OLED_WR_PORT, OLED_WR_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_RS_PORT, OLED_RS_PIN, GPIO_PIN_SET);
}

#elif OLED_BUS == OLED_BUS_SPI

/* 4-wire SPI pins (as on the ALIENTEK 0.96" module). */
#define OLED_CS_PORT    GPIOB
#define OLED_CS_PIN     GPIO_PIN_7

#define OLED_DC_PORT    GPIOB
#define OLED_DC_PIN     GPIO_PIN_4

#define OLED_SCLK_PORT  GPIOC
#define OLED_SCLK_PIN   GPIO_PIN_6

#define OLED_SDIN_PORT  GPIOC
#define OLED_SDIN_PIN   GPIO_PIN_7

static void oled_bus_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = SSD1306_RST_PIN;
    HAL_GPIO_Init(SSD1306_RST_PORT, &gpio);

    gpio.Pin = OLED_CS_PIN;
    HAL_GPIO_Init(OLED_CS_PORT, &gpio);

    gpio.Pin = OLED_DC_PIN;
    HAL_GPIO_Init(OLED_DC_PORT, &gpio);

    gpio.Pin = OLED_SCLK_PIN;
    HAL_GPIO_Init(OLED_SCLK_PORT, &gpio);

    gpio.Pin = OLED_SDIN_PIN;
    HAL_GPIO_Init(OLED_SDIN_PORT, &gpio);

    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_SCLK_PORT, OLED_SCLK_PIN, GPIO_PIN_SET);

    HAL_GPIO_WritePin(SSD1306_RST_PORT, SSD1306_RST_PIN, GPIO_PIN_RESET);
    delay_ms(SSD1306_RESET_DELAY_MS);
    HAL_GPIO_WritePin(SSD1306_RST_PORT, SSD1306_RST_PIN, GPIO_PIN_SET);
}

static void oled_bus_write(uint8_t data, oled_arg_t arg)
{
    uint8_t i;

    HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN,
                      (arg != OLED_ARG_CMD) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET);

    for (i = 0U; i < 8U; i++)
    {
        HAL_GPIO_WritePin(OLED_SCLK_PORT, OLED_SCLK_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(OLED_SDIN_PORT, OLED_SDIN_PIN,
                          ((data & 0x80U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(OLED_SCLK_PORT, OLED_SCLK_PIN, GPIO_PIN_SET);
        data = (uint8_t)(data << 1);
    }

    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_SET);
}

#elif OLED_BUS == OLED_BUS_I2C

/* TODO(I2C): choose SCL/SDA per the board wiring; module straps select I2C.
 * Control byte 0x00 = command, 0x40 = data; 7-bit address 0x3C (D/C# = SA0). */
#define SSD1306_I2C_SCL_PORT GPIOB
#define SSD1306_I2C_SCL_PIN  GPIO_PIN_6
#define SSD1306_I2C_SDA_PORT GPIOB
#define SSD1306_I2C_SDA_PIN  GPIO_PIN_7

static void oled_bus_init(void)
{
    /* TODO(I2C): configure SCL/SDA as outputs and pulse the reset line. */
    HAL_GPIO_WritePin(SSD1306_RST_PORT, SSD1306_RST_PIN, GPIO_PIN_RESET);
    delay_ms(SSD1306_RESET_DELAY_MS);
    HAL_GPIO_WritePin(SSD1306_RST_PORT, SSD1306_RST_PIN, GPIO_PIN_SET);
}

static void oled_bus_write(uint8_t data, oled_arg_t arg)
{
    /* TODO(I2C): START, addr<<1, control byte, byte, STOP. */
    (void)data;
    (void)arg;
}

#else
#error "OLED_BUS must be OLED_BUS_8080, OLED_BUS_SPI or OLED_BUS_I2C"
#endif

/* =========================== SSD1306 protocol =========================== */

typedef enum
{
    SSD1306_CMD_DISPLAY_OFF    = 0xAE,
    SSD1306_CMD_CLK_DIV        = 0xD5,
    SSD1306_CMD_MULTIPLEX      = 0xA8,
    SSD1306_CMD_DISPLAY_OFFSET = 0xD3,
    SSD1306_CMD_START_LINE     = 0x40,
    SSD1306_CMD_CHARGE_PUMP    = 0x8D,
    SSD1306_CMD_MEMORY_MODE    = 0x20,
    SSD1306_CMD_SEG_REMAP      = 0xA1,
    SSD1306_CMD_COM_SCAN_DIR   = 0xC8,
    SSD1306_CMD_COM_PINS       = 0xDA,
    SSD1306_CMD_CONTRAST       = 0x81,
    SSD1306_CMD_PRECHARGE      = 0xD9,
    SSD1306_CMD_VCOMH          = 0xDB,
    SSD1306_CMD_ENTIRE_ON      = 0xA4,
    SSD1306_CMD_NORMAL_DISPLAY = 0xA6,
    SSD1306_CMD_DISPLAY_ON     = 0xAF,
    SSD1306_CMD_LOW_COLUMN     = 0x00,
    SSD1306_CMD_HIGH_COLUMN    = 0x10,
    SSD1306_CMD_PAGE_ADDR      = 0xB0
} ssd1306_cmd_t;

#define SSD1306_CLK_DIV_VALUE       0x50U
#define SSD1306_MULTIPLEX_VALUE     0x3FU
#define SSD1306_OFFSET_VALUE        0x00U
#define SSD1306_CHARGE_PUMP_ENABLE  0x14U
#define SSD1306_CHARGE_PUMP_DISABLE 0x10U
#define SSD1306_MEMORY_MODE_PAGE    0x02U
#define SSD1306_COM_PINS_VALUE      0x12U
#define SSD1306_CONTRAST_VALUE      0xEFU
#define SSD1306_PRECHARGE_VALUE     0xF1U
#define SSD1306_VCOMH_VALUE         0x30U

void oled_ssd1306_init(void)
{
    oled_bus_init();

    oled_bus_write(SSD1306_CMD_DISPLAY_OFF, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_CLK_DIV, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CLK_DIV_VALUE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_MULTIPLEX, OLED_ARG_CMD);
    oled_bus_write(SSD1306_MULTIPLEX_VALUE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_DISPLAY_OFFSET, OLED_ARG_CMD);
    oled_bus_write(SSD1306_OFFSET_VALUE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_START_LINE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_CHARGE_PUMP, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CHARGE_PUMP_ENABLE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_MEMORY_MODE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_MEMORY_MODE_PAGE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_SEG_REMAP, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_COM_SCAN_DIR, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_COM_PINS, OLED_ARG_CMD);
    oled_bus_write(SSD1306_COM_PINS_VALUE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_CONTRAST, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CONTRAST_VALUE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_PRECHARGE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_PRECHARGE_VALUE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_VCOMH, OLED_ARG_CMD);
    oled_bus_write(SSD1306_VCOMH_VALUE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_ENTIRE_ON, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_NORMAL_DISPLAY, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_DISPLAY_ON, OLED_ARG_CMD);
}

void oled_ssd1306_display_on(void)
{
    oled_bus_write(SSD1306_CMD_CHARGE_PUMP, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CHARGE_PUMP_ENABLE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_DISPLAY_ON, OLED_ARG_CMD);
}

void oled_ssd1306_display_off(void)
{
    oled_bus_write(SSD1306_CMD_CHARGE_PUMP, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CHARGE_PUMP_DISABLE, OLED_ARG_CMD);
    oled_bus_write(SSD1306_CMD_DISPLAY_OFF, OLED_ARG_CMD);
}

void oled_ssd1306_flush(const uint8_t *gram, uint8_t width, uint8_t pages)
{
    uint8_t page;
    uint8_t col;

    for (page = 0U; page < pages; page++)
    {
        oled_bus_write((uint8_t)(SSD1306_CMD_PAGE_ADDR + page), OLED_ARG_CMD);
        oled_bus_write(SSD1306_CMD_LOW_COLUMN, OLED_ARG_CMD);
        oled_bus_write(SSD1306_CMD_HIGH_COLUMN, OLED_ARG_CMD);

        for (col = 0U; col < width; col++)
        {
            oled_bus_write(gram[(uint32_t)col * pages + page], OLED_ARG_DATA);
        }
    }
}
