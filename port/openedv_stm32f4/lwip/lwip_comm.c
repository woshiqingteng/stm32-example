/**
 * @file    lwip_comm.c
 * @brief   lwIP stack/network interface bring-up.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stm32f4xx_hal.h"

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

#define LWIP_DHCP_WAIT_MS   10000U

/* LWIP_RAND(): use the STM32 RNG when available, else fall back to rand(). */
static uint8_t s_rng_ready;

unsigned int lwip_rand(void)
{
    uint32_t i;

    if (s_rng_ready == 0U)
    {
        __HAL_RCC_RNG_CLK_ENABLE();
        RNG->CR |= RNG_CR_RNGEN;
        s_rng_ready = 1U;
    }

    for (i = 0; i < 1000U; i++)
    {
        if ((RNG->SR & RNG_SR_DRDY) != 0U)
        {
            return (unsigned int)RNG->DR;
        }
    }

    RNG->SR &= ~(RNG_SR_SEIS | RNG_SR_CEIS);   /* clear seed/clock error flags */

    return (unsigned int)rand();
}

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
};

struct netif g_lwip_netif;

uint8_t lwip_comm_init(void)
{
    ip4_addr_t ip;
    ip4_addr_t netmask;
    ip4_addr_t gateway;

    tcpip_init(NULL, NULL);

    IP4_ADDR(&ip,      g_lwipdev.ip[0],      g_lwipdev.ip[1],      g_lwipdev.ip[2],      g_lwipdev.ip[3]);
    IP4_ADDR(&netmask, g_lwipdev.netmask[0], g_lwipdev.netmask[1], g_lwipdev.netmask[2], g_lwipdev.netmask[3]);
    IP4_ADDR(&gateway, g_lwipdev.gateway[0], g_lwipdev.gateway[1], g_lwipdev.gateway[2], g_lwipdev.gateway[3]);

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

    if (g_lwipdev.dhcp_used)
    {
        dhcp_start(&g_lwip_netif);
        g_lwipdev.dhcp_status = LWIP_DHCP_START;
    }

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
