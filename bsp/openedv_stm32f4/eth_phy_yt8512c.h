/**
 * @file    eth_phy_yt8512c.h
 * @brief   YT8512C 10/100M Ethernet PHY (on the ALIENTEK F429 board).
 *
 * The PHY sits at MDIO address 0 on the board. This header holds the
 * chip-specific registers and bit masks; the device-independent logic lives in
 * eth_phy.c.
 */

#ifndef BSP_ETH_PHY_YT8512C_H
#define BSP_ETH_PHY_YT8512C_H

#include <stdint.h>

#define ETH_PHY_YT8512C_ADDR        0x00U   /* MDIO address of the on-board PHY */

#define ETH_PHY_YT8512C_REG_BCR     0x00U   /* basic control register */
#define ETH_PHY_YT8512C_REG_BSR     0x01U   /* basic status register */
#define ETH_PHY_YT8512C_REG_PHYSCSR 0x11U   /* vendor status register (speed/duplex) */

/* Register 2/3 identification values used to probe the chip. */
#define ETH_PHY_YT8512C_REG2_ID     0x0000U
#define ETH_PHY_YT8512C_REG3_ID     0x0128U

/* BCR bits */
#define ETH_PHY_YT8512C_BCR_RESET   0x8000U
#define ETH_PHY_YT8512C_BCR_AUTONEGO 0x1000U

/* BSR bits */
#define ETH_PHY_YT8512C_BSR_LINK    0x0004U

/* PHYSCSR bits (vendor register 0x11) */
#define ETH_PHY_YT8512C_SPEED_MASK  0x4000U /* set: 100M, clear: 10M */
#define ETH_PHY_YT8512C_DUPLEX_MASK 0x2000U /* set: full duplex */

/** @brief  True when register 2/3 identify a YT8512C. */
uint8_t eth_phy_yt8512c_probe(uint16_t reg2, uint16_t reg3);

#endif /* BSP_ETH_PHY_YT8512C_H */
