/**
 * @file    main.c
 * @brief   lwip_12_1_socket_tcp_server_multi: multi-connection TCP server
 *          (ALIENTEK experiment 12-1). One task per accepted client.
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "lwip_comm.h"

#define DEMO_TASK_PRIO      4
#define DEMO_TASK_STK_SIZE  512
#define CLIENT_STK_SIZE     384
#define CLIENT_PRIO         4
#define DEMO_PORT           8080

static void client_task(void *arg)
{
    int c = (int)(intptr_t)arg;
    char buf[256];
    int n;

    while ((n = recv(c, buf, sizeof(buf), 0)) > 0)
    {
        (void)send(c, buf, (size_t)n, 0);
    }
    closesocket(c);
    vTaskDelete(NULL);
}

static void demo_task(void *arg)
{
    int s;
    int c;
    struct sockaddr_in local;
    struct sockaddr_in remote;
    socklen_t rlen = sizeof(remote);

    (void)arg;

    lwip_comm_wait_ip();
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    s = socket(AF_INET, SOCK_STREAM, 0);
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(DEMO_PORT);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(s, (struct sockaddr *)&local, sizeof(local));
    listen(s, 4);
    printf("net: multi-connection tcp echo server on port %u\r\n", DEMO_PORT);

    for (;;)
    {
        rlen = sizeof(remote);
        c = accept(s, (struct sockaddr *)&remote, &rlen);
        if (c >= 0)
        {
            sys_thread_new("client", client_task, (void *)(intptr_t)c,
                           CLIENT_STK_SIZE, CLIENT_PRIO);
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
