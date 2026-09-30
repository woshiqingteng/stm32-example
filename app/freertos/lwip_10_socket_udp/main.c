/**
 * @file    main.c
 * @brief   lwip_10_socket_udp: socket UDP echo server (ALIENTEK experiment 10).
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

static void demo_task(void *arg)
{
    int s;
    struct sockaddr_in addr;
    struct sockaddr_in remote;
    socklen_t rlen = sizeof(remote);
    char buf[256];

    (void)arg;

    lwip_comm_wait_ip();
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    s = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEMO_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    printf("net: udp echo on port %u\r\n", DEMO_PORT);

    for (;;)
    {
        int n = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&remote, &rlen);
        if (n > 0)
        {
            (void)sendto(s, buf, (size_t)n, 0, (struct sockaddr *)&remote, rlen);
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
