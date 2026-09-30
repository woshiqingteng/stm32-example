/**
 * @file    eth.c
 * @brief   On-board Ethernet MAC: RMII GPIO, PHY reset and MAC bring-up.
 */

#include "stm32f4xx_hal.h"
#include "eth.h"
#include "eth_phy.h"
#include "eth_phy_yt8512c.h"
#include "io_expand.h"
#include "delay.h"

#define ETH_RX_BUFFER_LEN   1524U

ETH_HandleTypeDef g_eth_handle;

static ETH_DMADescTypeDef s_eth_rx_desc[ETH_RX_DESC_CNT];
static ETH_DMADescTypeDef s_eth_tx_desc[ETH_TX_DESC_CNT];

static int32_t eth_phy_read(uint16_t reg, uint16_t *val)
{
    uint32_t tmp = 0;

    if (HAL_ETH_ReadPHYRegister(&g_eth_handle, ETH_PHY_YT8512C_ADDR, reg, &tmp) != HAL_OK)
    {
        return -1;
    }

    *val = (uint16_t)tmp;
    return 0;
}

static int32_t eth_phy_write(uint16_t reg, uint16_t val)
{
    if (HAL_ETH_WritePHYRegister(&g_eth_handle, ETH_PHY_YT8512C_ADDR, reg, val) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    GPIO_InitTypeDef gpio = {0};
    (void)heth;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_ETH_CLK_ENABLE();

    /* RMII wiring on the ALIENTEK F429 board:
     *   REF_CLK/PA1, MDIO/PA2, CRS_DV/PA7, MDC/PC1, RXD0/PC4, RXD1/PC5,
     *   TX_EN/PB11, TXD0/PG13, TXD1/PG14. */
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF11_ETH;

    gpio.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin = GPIO_PIN_11;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = GPIO_PIN_13 | GPIO_PIN_14;
    HAL_GPIO_Init(GPIOG, &gpio);

    HAL_NVIC_SetPriority(ETH_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
}

void ETH_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&g_eth_handle);
}

uint8_t eth_init(uint8_t mac[6])
{
    /* The on-board PHY reset is driven by the PCF8574 expander (active low). */
    io_expand_init();
    io_expand_write_bit(PCF8574_ETH_RESET_IO, 0U);
    delay_ms(100);
    io_expand_write_bit(PCF8574_ETH_RESET_IO, 1U);
    delay_ms(100);

    g_eth_handle.Instance         = ETH;
    g_eth_handle.Init.MACAddr     = mac;
    g_eth_handle.Init.MediaInterface = HAL_ETH_RMII_MODE;
    g_eth_handle.Init.RxDesc      = s_eth_rx_desc;
    g_eth_handle.Init.TxDesc      = s_eth_tx_desc;
    g_eth_handle.Init.RxBuffLen   = ETH_RX_BUFFER_LEN;

    if (HAL_ETH_Init(&g_eth_handle) != HAL_OK)
    {
        return 1U;
    }

    HAL_ETH_SetMDIOClockRange(&g_eth_handle);

    eth_phy_bind(eth_phy_read, eth_phy_write);

    return eth_phy_init();
}
