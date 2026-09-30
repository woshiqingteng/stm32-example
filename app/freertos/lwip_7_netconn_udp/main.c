/**
 * @file    main.c
 * @brief   lwip_7_netconn_udp: netconn UDP echo server (ALIENTEK experiment 7).
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
    struct netbuf *buf;

    (void)arg;

    lwip_comm_wait_ip();
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    conn = netconn_new(NETCONN_UDP);
    netconn_bind(conn, IP_ADDR_ANY, DEMO_PORT);
    printf("net: udp echo on port %u\r\n", DEMO_PORT);

    for (;;)
    {
        if (netconn_recv(conn, &buf) == ERR_OK)
        {
            netconn_sendto(conn, buf, netbuf_fromaddr(buf), netbuf_fromport(buf));
            netbuf_delete(buf);
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
