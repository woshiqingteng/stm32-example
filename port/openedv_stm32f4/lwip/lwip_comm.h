/**
 * @file    lwip_comm.h
 * @brief   lwIP stack/network interface bring-up helpers.
 */

#ifndef PORT_LWIP_COMM_H
#define PORT_LWIP_COMM_H

#include <stdint.h>
#include "lwip/netif.h"

/* DHCP state (g_lwipdev.dhcp_status). */
#define LWIP_DHCP_OFF               0U  /* DHCP disabled / not started */
#define LWIP_DHCP_START             1U  /* DHCP discovery in progress */
#define LWIP_DHCP_ADDRESS_ASSIGNED  2U  /* an address has been bound */

typedef struct
{
    uint8_t mac[6];         /* station address */
    uint8_t ip[4];          /* local address (or DHCP result) */
    uint8_t netmask[4];
    uint8_t gateway[4];
    uint8_t dhcp_used;      /* 1: start DHCP after netif up */
    uint8_t dhcp_status;    /* one of LWIP_DHCP_* */
    uint8_t link_up;
} lwip_dev_t;

extern lwip_dev_t     g_lwipdev;
extern struct netif   g_lwip_netif;

/** @brief  Load the built-in default addresses into @p dev. */
void    lwip_comm_default_ip_set(lwip_dev_t *dev);

/** @brief  Initialise lwIP, add the netif and (optionally) start DHCP.
 *  @return 0 on success, 1 otherwise. */
uint8_t lwip_comm_init(void);

/** @brief  Stop DHCP and apply the built-in static address (DHCP fallback). */
void    lwip_comm_fallback_ip(void);

/** @brief  Block until an address is ready (DHCP, with a static fallback).
 *  Call from a task after the scheduler has started. */
void    lwip_comm_wait_ip(void);

#endif /* PORT_LWIP_COMM_H */
