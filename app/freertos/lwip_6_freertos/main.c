/**
 * @file    main.c
 * @brief   lwip_6_freertos: lwIP bring-up on FreeRTOS (ALIENTEK experiment 6).
 *
 * Brings up the Ethernet MAC + PHY, obtains an address via DHCP (static
 * fallback) and reports the result. The stack replies to ICMP echo (ping).
 */

#include <stdio.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

#include "lwip_comm.h"

#define DEMO_TASK_PRIO      4
#define DEMO_TASK_STK_SIZE  384

static void demo_task(void *arg)
{
    (void)arg;

    lwip_comm_wait_ip();

    printf("net: link %s ip %s\r\n",
           netif_is_link_up(&g_lwip_netif) ? "up" : "down",
           ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");

    lwip_comm_init();

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
