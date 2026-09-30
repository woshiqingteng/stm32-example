/**
 * @file    eth_phy.c
 * @brief   Ethernet PHY feature logic (device independent).
 */

#include "stm32f4xx_hal.h"
#include "eth_phy.h"
#include "eth_phy_yt8512c.h"

#define ETH_PHY_RESET_TIMEOUT_MS    500U
#define ETH_PHY_AUTONEGO_WAIT_MS    2000U

static eth_phy_read_fn  s_read;
static eth_phy_write_fn s_write;

void eth_phy_bind(eth_phy_read_fn rd, eth_phy_write_fn wr)
{
    s_read  = rd;
    s_write = wr;
}

uint8_t eth_phy_init(void)
{
    uint16_t reg2 = 0;
    uint16_t reg3 = 0;
    uint16_t bcr = 0;
    uint32_t start;

    if (!s_read || !s_write)
    {
        return 1U;
    }

    /* Identify the chip: only the YT8512C is supported so far. */
    if (s_read(2U, &reg2) != 0)
    {
        return 1U;
    }
    if (s_read(3U, &reg3) != 0)
    {
        return 1U;
    }
    if (!eth_phy_yt8512c_probe(reg2, reg3))
    {
        return 1U;
    }

    /* Software reset and wait for it to self-clear. */
    if (s_write(ETH_PHY_YT8512C_REG_BCR, ETH_PHY_YT8512C_BCR_RESET) != 0)
    {
        return 1U;
    }

    start = HAL_GetTick();
    do
    {
        if (s_read(ETH_PHY_YT8512C_REG_BCR, &bcr) != 0)
        {
            return 1U;
        }
        if ((HAL_GetTick() - start) > ETH_PHY_RESET_TIMEOUT_MS)
        {
            return 1U;
        }
    } while (bcr & ETH_PHY_YT8512C_BCR_RESET);

    /* Enable auto-negotiation. */
    if (s_write(ETH_PHY_YT8512C_REG_BCR, ETH_PHY_YT8512C_BCR_AUTONEGO) != 0)
    {
        return 1U;
    }

    /* Give auto-negotiation time to settle. */
    HAL_Delay(ETH_PHY_AUTONEGO_WAIT_MS);

    return 0U;
}

uint8_t eth_phy_link_up(void)
{
    uint16_t bsr = 0;

    if (!s_read || s_read(ETH_PHY_YT8512C_REG_BSR, &bsr) != 0)
    {
        return 0U;
    }

    return (bsr & ETH_PHY_YT8512C_BSR_LINK) ? 1U : 0U;
}

uint8_t eth_phy_speed(void)
{
    uint16_t val = 0;

    if (!s_read || s_read(ETH_PHY_YT8512C_REG_PHYSCSR, &val) != 0)
    {
        return ETH_PHY_SPEED_10M;
    }

    return (val & ETH_PHY_YT8512C_SPEED_MASK) ? ETH_PHY_SPEED_100M : ETH_PHY_SPEED_10M;
}

uint8_t eth_phy_full_duplex(void)
{
    uint16_t val = 0;

    if (!s_read || s_read(ETH_PHY_YT8512C_REG_PHYSCSR, &val) != 0)
    {
        return 0U;
    }

    return (val & ETH_PHY_YT8512C_DUPLEX_MASK) ? 1U : 0U;
}
