/**
 * @file    main.c
 * @brief   lwip_8_netconn_tcp_client: netconn TCP client (ALIENTEK experiment 8).
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"

#include "lwip_comm.h"

#define DEMO_TASK_PRIO      4
#define DEMO_TASK_STK_SIZE  512

/* Remote server (placeholder: the PC TCP server used by the ALIENTEK demo). */
#define DEST_IP_ADDR0       192
#define DEST_IP_ADDR1       168
#define DEST_IP_ADDR2       1
#define DEST_IP_ADDR3       111
#define DEMO_PORT           8080

static const char s_msg[] = "Hello from STM32F429 (netconn TCP client)\r\n";

static void demo_task(void *arg)
{
    ip_addr_t dst;
    struct netconn *conn;
    struct netbuf *buf;

    (void)arg;

    lwip_comm_wait_ip();
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    IP4_ADDR(&dst, DEST_IP_ADDR0, DEST_IP_ADDR1, DEST_IP_ADDR2, DEST_IP_ADDR3);
    conn = netconn_new(NETCONN_TCP);
    if (netconn_connect(conn, &dst, DEMO_PORT) == ERR_OK)
    {
        printf("net: connected to %s:%u\r\n", ip4addr_ntoa(&dst), DEMO_PORT);
        netconn_write(conn, s_msg, sizeof(s_msg) - 1, NETCONN_COPY);

        if (netconn_recv(conn, &buf) == ERR_OK)
        {
            printf("net: rx %u bytes\r\n", (unsigned)buf->p->tot_len);
            netbuf_delete(buf);
        }
        netconn_close(conn);
    }
    else
    {
        printf("net: connect failed\r\n");
    }
    netconn_delete(conn);

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
