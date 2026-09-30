/**
 * @file    eth_phy.h
 * @brief   Ethernet PHY feature interface (device independent).
 *
 * The board-level MAC driver (eth.c) binds a register read/write pair, then
 * calls into this layer for identification, reset/auto-negotiation and link
 * status. Chip specifics live in eth_phy_<chip>.c.
 */

#ifndef BSP_ETH_PHY_H
#define BSP_ETH_PHY_H

#include <stdint.h>

#define ETH_PHY_SPEED_10M   10U
#define ETH_PHY_SPEED_100M  100U

/** @brief  MDIO register read: 0 on success, <0 on error. */
typedef int32_t (*eth_phy_read_fn)(uint16_t reg, uint16_t *val);

/** @brief  MDIO register write: 0 on success, <0 on error. */
typedef int32_t (*eth_phy_write_fn)(uint16_t reg, uint16_t val);

/** @brief  Bind the MDIO accessors (called by eth_init()). */
void eth_phy_bind(eth_phy_read_fn rd, eth_phy_write_fn wr);

/** @brief  Identify and reset the PHY, then start auto-negotiation.
 *  @return 0 on success, 1 otherwise. */
uint8_t eth_phy_init(void);

/** @brief  1 when the link is up. */
uint8_t eth_phy_link_up(void);

/** @brief  Negotiated speed: ETH_PHY_SPEED_10M or ETH_PHY_SPEED_100M. */
uint8_t eth_phy_speed(void);

/** @brief  1 for full duplex, 0 for half duplex. */
uint8_t eth_phy_full_duplex(void);

#endif /* BSP_ETH_PHY_H */
