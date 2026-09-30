/**
 * @file    lwip_comm.c
 * @brief   lwIP stack/network interface bring-up.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwip/opt.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip_comm.h"
#include "ethernetif.h"

#define LWIP_DHCP_WAIT_MS        10000U
#define LWIP_MAC_START_WAIT_MS    2000U

/* LWIP_RAND(): hardware RNG by default, C library rand() when disabled. */
#if LWIP_RAND_USE_HW

#include "stm32f4xx_hal.h"

static RNG_HandleTypeDef s_rng;
static uint8_t           s_rng_state;   /* 0 = unknown, 1 = unavailable, 2 = ready */

unsigned int lwip_rand(void)
{
    uint32_t r;

    if (s_rng_state == 0U)
    {
        __HAL_RCC_RNG_CLK_ENABLE();
        s_rng.Instance = RNG;
        s_rng_state = (HAL_RNG_Init(&s_rng) == HAL_OK) ? 2U : 1U;
    }

    if (s_rng_state == 2U && HAL_RNG_GenerateRandomNumber(&s_rng, &r) == HAL_OK)
    {
        return (unsigned int)r;
    }

    return (unsigned int)rand();
}

#else

unsigned int lwip_rand(void)
{
    return (unsigned int)rand();
}

#endif /* LWIP_RAND_USE_HW */

lwip_dev_t g_lwipdev =
{
    .mac          = { 0x02, 0x00, 0x00, 0x12, 0x34, 0x56 },
    .ip           = { 192, 168, 2, 100 },
    .netmask      = { 255, 255, 255, 0 },
    .gateway      = { 192, 168, 2, 1 },
    .dns          = { 192, 168, 2, 1 },
    .dhcp_used    = 1U,
    .dhcp_status  = LWIP_DHCP_OFF,
    .link_up      = 0U,
    .mac_started  = 0U,
};

struct netif g_lwip_netif;

uint8_t lwip_comm_init(void)
{
    ip4_addr_t ip;
    ip4_addr_t netmask;
    ip4_addr_t gateway;

    tcpip_init(NULL, NULL);

    if (g_lwipdev.dhcp_used)
    {
        /* DHCP needs an unconfigured interface (0.0.0.0); the static address
         * is only applied later by lwip_comm_fallback_ip(). */
        IP4_ADDR(&ip, 0, 0, 0, 0);
        IP4_ADDR(&netmask, 0, 0, 0, 0);
        IP4_ADDR(&gateway, 0, 0, 0, 0);
    }
    else
    {
        IP4_ADDR(&ip,      g_lwipdev.ip[0],      g_lwipdev.ip[1],      g_lwipdev.ip[2],      g_lwipdev.ip[3]);
        IP4_ADDR(&netmask, g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
        IP4_ADDR(&gateway, g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);
    }

    if (netif_add(&g_lwip_netif, &ip, &netmask, &gateway, NULL,
                  ethernetif_init, tcpip_input) == NULL)
    {
        return 1U;
    }

    netif_set_default(&g_lwip_netif);
    netif_set_up(&g_lwip_netif);

    {   /* static DNS; overwritten by DHCP if it succeeds */
        ip_addr_t dns;
        IP_ADDR4(&dns, g_lwipdev.dns[0], g_lwipdev.dns[1], g_lwipdev.dns[2], g_lwipdev.dns[3]);
        dns_setserver(0, &dns);
    }

    /* DHCP is started later, from lwip_comm_wait_ip(), once the scheduler is
     * running and the eth_rx task has started the MAC. */
    return 0U;
}

void lwip_comm_fallback_ip(void)
{
    ip4_addr_t ip;
    ip4_addr_t netmask;
    ip4_addr_t gateway;

    if (g_lwipdev.dhcp_used)
    {
        netifapi_dhcp_stop(&g_lwip_netif);
        g_lwipdev.dhcp_status = LWIP_DHCP_OFF;
    }

    IP4_ADDR(&ip,      g_lwipdev.ip[0],      g_lwipdev.ip[1],      g_lwipdev.ip[2],      g_lwipdev.ip[3]);
    IP4_ADDR(&netmask, g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
    IP4_ADDR(&gateway, g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);

    netifapi_netif_set_addr(&g_lwip_netif, &ip, &netmask, &gateway);

    /* The request runs on the tcpip thread; wait until it is applied so the
     * caller can use the interface immediately. */
    for (uint32_t i = 0; i < 50U; i++)
    {
        if (ip4_addr_cmp(netif_ip4_addr(&g_lwip_netif), &ip))
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void lwip_comm_wait_ip(void)
{
    if (g_lwipdev.dhcp_used)
    {
        TickType_t start = xTaskGetTickCount();

        /* The eth_rx task starts the MAC after the scheduler is running; DHCP
         * must not be started before that, or its first DISCOVER cannot be
         * transmitted (no MAC/DMA), matching the ALIENTEK reference ordering. */
        while ((g_lwipdev.mac_started == 0U) &&
               ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(LWIP_MAC_START_WAIT_MS)))
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        netifapi_dhcp_start(&g_lwip_netif);
        g_lwipdev.dhcp_status = LWIP_DHCP_START;

        start = xTaskGetTickCount();

        while (!dhcp_supplied_address(&g_lwip_netif))
        {
            if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(LWIP_DHCP_WAIT_MS))
            {
                printf("net: dhcp timeout, using static ip\r\n");
                lwip_comm_fallback_ip();
                vTaskDelay(pdMS_TO_TICKS(200));
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (dhcp_supplied_address(&g_lwip_netif))
        {
            g_lwipdev.dhcp_status = LWIP_DHCP_ADDRESS_ASSIGNED;
        }
    }
}
