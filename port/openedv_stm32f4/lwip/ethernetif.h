/**
 * @file    ethernetif.h
 * @brief   lwIP netif driver entry point for the STM32F4 ETH MAC.
 */

#ifndef PORT_ETHERNETIF_H
#define PORT_ETHERNETIF_H

#include "lwip/err.h"
#include "lwip/netif.h"

/** @brief  lwIP netif init callback (passed to netif_add()). */
err_t ethernetif_init(struct netif *netif);

#endif /* PORT_ETHERNETIF_H */
