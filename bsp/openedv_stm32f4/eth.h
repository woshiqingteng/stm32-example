/**
 * @file    eth.h
 * @brief   On-board Ethernet MAC (STM32F4 ETH + RMII + YT8512C PHY).
 *
 * The MAC lives on the board side; PHY specifics are handled by eth_phy.<c/h>
 * and eth_phy_yt8512c.<c/h>. The lwIP port (ethernetif.c) drives the handle.
 */

#ifndef BSP_ETH_H
#define BSP_ETH_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief  ETH MAC handle shared with the lwIP port. */
extern ETH_HandleTypeDef g_eth_handle;

/** @brief  Reset the PHY, configure and start the MAC in RMII mode.
 *  @param  mac  6-byte station address.
 *  @return 0 on success, 1 on failure. */
uint8_t eth_init(uint8_t mac[6]);

#endif /* BSP_ETH_H */
