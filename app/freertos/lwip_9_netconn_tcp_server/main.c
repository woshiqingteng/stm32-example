/**
 * @file    main.c
 * @brief   lwip_9_netconn_tcp_server: netconn TCP echo server (experiment 9).
 */

#include <stdio.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"

#include "lwip_comm.h"

#define DEMO_TASK_PRIO      4
#define DEMO_TASK_STK_SIZE  512
#define DEMO_PORT           8080

static void demo_task(void *arg)
{
    struct netconn *conn;
    struct netconn *newconn;
    struct netbuf *buf;

    (void)arg;

    lwip_comm_wait_ip();
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, IP_ADDR_ANY, DEMO_PORT);
    netconn_listen(conn);
    printf("net: tcp echo server on port %u\r\n", DEMO_PORT);

    for (;;)
    {
        if (netconn_accept(conn, &newconn) == ERR_OK)
        {
            while (netconn_recv(newconn, &buf) == ERR_OK)
            {
                netconn_write(newconn, buf->p->payload, buf->p->len, NETCONN_COPY);
                netbuf_delete(buf);
            }
            netconn_close(newconn);
            netconn_delete(newconn);
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
