/**
 * @file    main.c
 * @brief   lwip_10_2_udp_multicast: socket UDP multicast (experiment 10-2).
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "lwip_comm.h"

#define DEMO_TASK_PRIO      4
#define DEMO_TASK_STK_SIZE  512
#define DEMO_PORT           8080
#define GROUP_IP            "224.0.1.0"

static void demo_task(void *arg)
{
    int s;
    struct sockaddr_in local;
    struct ip_mreq mreq;
    char buf[256];

    (void)arg;

    lwip_comm_wait_ip();
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    s = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(DEMO_PORT);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(s, (struct sockaddr *)&local, sizeof(local));

    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = inet_addr(GROUP_IP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    printf("net: joined multicast group %s:%u\r\n", GROUP_IP, DEMO_PORT);

    for (;;)
    {
        int n = recv(s, buf, sizeof(buf), 0);
        if (n > 0)
        {
            printf("net: multicast rx %d bytes\r\n", n);
        }
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
