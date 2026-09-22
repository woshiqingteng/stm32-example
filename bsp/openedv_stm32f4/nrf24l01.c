/**
 * @file    nrf24l01.c
 * @brief   NRF24L01 driver over the shared SPI2 bus, ported from the vendor
 *          NRF24L01 example. CE=PG12, CSN=PG10, IRQ=PI11.
 */

#include "stm32f4xx_hal.h"
#include "spi.h"
#include "nrf24l01.h"

#define NRF24L01_CE_HIGH()   HAL_GPIO_WritePin(NRF24L01_CE_GPIO_PORT, NRF24L01_CE_GPIO_PIN, GPIO_PIN_SET)
#define NRF24L01_CE_LOW()    HAL_GPIO_WritePin(NRF24L01_CE_GPIO_PORT, NRF24L01_CE_GPIO_PIN, GPIO_PIN_RESET)
#define NRF24L01_CSN_HIGH()  HAL_GPIO_WritePin(NRF24L01_CSN_GPIO_PORT, NRF24L01_CSN_GPIO_PIN, GPIO_PIN_SET)
#define NRF24L01_CSN_LOW()   HAL_GPIO_WritePin(NRF24L01_CSN_GPIO_PORT, NRF24L01_CSN_GPIO_PIN, GPIO_PIN_RESET)
#define NRF24L01_IRQ_READ()  HAL_GPIO_ReadPin(NRF24L01_IRQ_GPIO_PORT, NRF24L01_IRQ_GPIO_PIN)

/* SPI commands. */
#define NRF24L01_READ_REG       0x00U
#define NRF24L01_WRITE_REG      0x20U
#define NRF24L01_RD_RX_PLOAD    0x61U
#define NRF24L01_WR_TX_PLOAD    0xA0U
#define NRF24L01_FLUSH_TX       0xE1U
#define NRF24L01_FLUSH_RX       0xE2U

/* Register map. */
#define NRF24L01_REG_CONFIG     0x00U
#define NRF24L01_REG_EN_AA      0x01U
#define NRF24L01_REG_EN_RXADDR  0x02U
#define NRF24L01_REG_SETUP_RETR 0x04U
#define NRF24L01_REG_RF_CH      0x05U
#define NRF24L01_REG_RF_SETUP   0x06U
#define NRF24L01_REG_STATUS     0x07U
#define NRF24L01_REG_RX_ADDR_P0 0x0AU
#define NRF24L01_REG_TX_ADDR    0x10U
#define NRF24L01_REG_RX_PW_P0   0x11U

#define NRF24L01_STATUS_MAX_TX  0x10U
#define NRF24L01_STATUS_TX_OK   0x20U
#define NRF24L01_STATUS_RX_OK   0x40U

#define NRF24L01_CHECK_BYTE     0xA5U
#define NRF24L01_RF_CHANNEL     40U
#define NRF24L01_RF_SETUP_VAL   0x0FU
#define NRF24L01_TX_TIMEOUT     0xFFFFFFFFU

static const uint8_t g_tx_address[NRF24L01_TX_ADR_WIDTH] = {0x34U, 0x43U, 0x10U, 0x10U, 0x01U};
static const uint8_t g_rx_address[NRF24L01_RX_ADR_WIDTH] = {0x34U, 0x43U, 0x10U, 0x10U, 0x01U};

static uint8_t nrf24l01_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t status;

    NRF24L01_CSN_LOW();
    status = spi_read_write_byte(SPI_BUS_NRF24L01, reg);
    (void)spi_read_write_byte(SPI_BUS_NRF24L01, value);
    NRF24L01_CSN_HIGH();

    return status;
}

static uint8_t nrf24l01_read_reg(uint8_t reg)
{
    uint8_t value;

    NRF24L01_CSN_LOW();
    (void)spi_read_write_byte(SPI_BUS_NRF24L01, reg);
    value = spi_read_write_byte(SPI_BUS_NRF24L01, 0xFFU);
    NRF24L01_CSN_HIGH();

    return value;
}

static uint8_t nrf24l01_write_buf(uint8_t reg, const uint8_t *pbuf, uint8_t len)
{
    uint8_t status;
    uint8_t i;

    NRF24L01_CSN_LOW();
    status = spi_read_write_byte(SPI_BUS_NRF24L01, reg);

    for (i = 0U; i < len; i++)
    {
        (void)spi_read_write_byte(SPI_BUS_NRF24L01, pbuf[i]);
    }

    NRF24L01_CSN_HIGH();

    return status;
}

static uint8_t nrf24l01_read_buf(uint8_t reg, uint8_t *pbuf, uint8_t len)
{
    uint8_t status;
    uint8_t i;

    NRF24L01_CSN_LOW();
    status = spi_read_write_byte(SPI_BUS_NRF24L01, reg);

    for (i = 0U; i < len; i++)
    {
        pbuf[i] = spi_read_write_byte(SPI_BUS_NRF24L01, 0xFFU);
    }

    NRF24L01_CSN_HIGH();

    return status;
}

void nrf24l01_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* ---- MSP begin: CE/CSN/IRQ clock + pins ---- */
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio_init.Pin = NRF24L01_CE_GPIO_PIN;
    HAL_GPIO_Init(NRF24L01_CE_GPIO_PORT, &gpio_init);

    gpio_init.Pin = NRF24L01_CSN_GPIO_PIN;
    HAL_GPIO_Init(NRF24L01_CSN_GPIO_PORT, &gpio_init);

    gpio_init.Pin  = NRF24L01_IRQ_GPIO_PIN;
    gpio_init.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(NRF24L01_IRQ_GPIO_PORT, &gpio_init);
    /* ---- MSP end ---- */

    spi_init(SPI_BUS_NRF24L01);

    NRF24L01_CE_LOW();
    NRF24L01_CSN_HIGH();
}

uint8_t nrf24l01_check(void)
{
    uint8_t buf[NRF24L01_TX_ADR_WIDTH];
    uint8_t i;

    for (i = 0U; i < NRF24L01_TX_ADR_WIDTH; i++)
    {
        buf[i] = NRF24L01_CHECK_BYTE;
    }

    spi_set_speed(SPI_BUS_NRF24L01, SPI_BAUDRATEPRESCALER_32);

    (void)nrf24l01_write_buf(NRF24L01_WRITE_REG + NRF24L01_REG_TX_ADDR, buf, NRF24L01_TX_ADR_WIDTH);
    (void)nrf24l01_read_buf(NRF24L01_REG_TX_ADDR, buf, NRF24L01_TX_ADR_WIDTH);

    for (i = 0U; i < NRF24L01_TX_ADR_WIDTH; i++)
    {
        if (buf[i] != NRF24L01_CHECK_BYTE)
        {
            return 1U;
        }
    }

    return 0U;
}

uint8_t nrf24l01_tx_packet(uint8_t *ptxbuf)
{
    uint8_t status;
    uint8_t rval = 0xFFU;
    uint32_t timeout = NRF24L01_TX_TIMEOUT;

    NRF24L01_CE_LOW();
    (void)nrf24l01_write_buf(NRF24L01_WR_TX_PLOAD, ptxbuf, NRF24L01_TX_PLOAD_WIDTH);
    NRF24L01_CE_HIGH();

    while ((NRF24L01_IRQ_READ() != GPIO_PIN_RESET) && (timeout != 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        return rval;
    }

    status = nrf24l01_read_reg(NRF24L01_REG_STATUS);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_STATUS, status);

    if ((status & NRF24L01_STATUS_MAX_TX) != 0U)
    {
        (void)nrf24l01_write_reg(NRF24L01_FLUSH_TX, 0xFFU);
        rval = 1U;
    }

    if ((status & NRF24L01_STATUS_TX_OK) != 0U)
    {
        rval = 0U;
    }

    return rval;
}

uint8_t nrf24l01_rx_packet(uint8_t *prxbuf)
{
    uint8_t status;
    uint8_t rval = 1U;

    status = nrf24l01_read_reg(NRF24L01_REG_STATUS);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_STATUS, status);

    if ((status & NRF24L01_STATUS_RX_OK) != 0U)
    {
        (void)nrf24l01_read_buf(NRF24L01_RD_RX_PLOAD, prxbuf, NRF24L01_RX_PLOAD_WIDTH);
        (void)nrf24l01_write_reg(NRF24L01_FLUSH_RX, 0xFFU);
        rval = 0U;
    }

    return rval;
}

void nrf24l01_rx_mode(void)
{
    NRF24L01_CE_LOW();

    (void)nrf24l01_write_buf(NRF24L01_WRITE_REG + NRF24L01_REG_RX_ADDR_P0,
                             g_rx_address, NRF24L01_RX_ADR_WIDTH);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_EN_AA, 0x01U);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_EN_RXADDR, 0x01U);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_RF_CH, NRF24L01_RF_CHANNEL);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_RX_PW_P0, NRF24L01_RX_PLOAD_WIDTH);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_RF_SETUP, NRF24L01_RF_SETUP_VAL);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_CONFIG, 0x0FU);

    NRF24L01_CE_HIGH();
}

void nrf24l01_tx_mode(void)
{
    NRF24L01_CE_LOW();

    (void)nrf24l01_write_buf(NRF24L01_WRITE_REG + NRF24L01_REG_TX_ADDR,
                             g_tx_address, NRF24L01_TX_ADR_WIDTH);
    (void)nrf24l01_write_buf(NRF24L01_WRITE_REG + NRF24L01_REG_RX_ADDR_P0,
                             g_rx_address, NRF24L01_RX_ADR_WIDTH);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_EN_AA, 0x01U);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_EN_RXADDR, 0x01U);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_SETUP_RETR, 0x1AU);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_RF_CH, NRF24L01_RF_CHANNEL);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_RF_SETUP, NRF24L01_RF_SETUP_VAL);
    (void)nrf24l01_write_reg(NRF24L01_WRITE_REG + NRF24L01_REG_CONFIG, 0x0EU);

    NRF24L01_CE_HIGH();
}
