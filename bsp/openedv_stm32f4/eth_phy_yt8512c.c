/**
 * @file    eth_phy_yt8512c.c
 * @brief   YT8512C PHY identification.
 */

#include "eth_phy_yt8512c.h"

uint8_t eth_phy_yt8512c_probe(uint16_t reg2, uint16_t reg3)
{
    return (reg2 == ETH_PHY_YT8512C_REG2_ID && reg3 == ETH_PHY_YT8512C_REG3_ID) ? 1U : 0U;
}
