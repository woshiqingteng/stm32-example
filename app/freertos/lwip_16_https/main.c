/**
 * @file    main.c
 * @brief   lwip_16_https: lwIP netconn HTTP server over FreeRTOS.
 *
 * Brings up the on-board Ethernet MAC + YT8512C PHY, obtains an address via
 * DHCP (with a static fallback) and serves a small HTML page on port 80.
 */

#include <stdio.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/dhcp.h"
#include "lwip/ip_addr.h"

#include "lwip_comm.h"

#define HTTP_TASK_PRIO      4
#define HTTP_TASK_STK_SIZE  512
#define DHCP_WAIT_MS        10000

static const char s_http_hdr[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n\r\n";

static const char s_http_page[] =
    "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
    "<title>STM32F429 lwIP</title></head><body>"
    "<h2>STM32F429 + lwIP HTTP server</h2>"
    "<p>Hello from the ALIENTEK F429 board.</p>"
    "</body></html>";

static void http_serve(struct netconn *conn)
{
    struct netbuf *inbuf;
    void *buf;
    u16_t buflen;

    if (netconn_recv(conn, &inbuf) == ERR_OK)
    {
        netbuf_data(inbuf, &buf, &buflen);
        (void)buf;
        (void)buflen;

        netconn_write(conn, s_http_hdr, sizeof(s_http_hdr) - 1, NETCONN_NOCOPY);
        netconn_write(conn, s_http_page, sizeof(s_http_page) - 1, NETCONN_NOCOPY);
        netbuf_delete(inbuf);
    }

    netconn_close(conn);
}

static void http_task(void *pvParameters)
{
    struct netconn *conn;
    struct netconn *newconn;
    TickType_t start;

    (void)pvParameters;

    if (g_lwipdev.dhcp_used)
    {
        start = xTaskGetTickCount();
        while (!dhcp_supplied_address(&g_lwip_netif))
        {
            if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(DHCP_WAIT_MS))
            {
                printf("net: dhcp timeout, using static ip\r\n");
                lwip_comm_fallback_ip();
                vTaskDelay(pdMS_TO_TICKS(200));
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    printf("net: link %s ip %s\r\n",
           netif_is_link_up(&g_lwip_netif) ? "up" : "down",
           ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    conn = netconn_new(NETCONN_TCP);
    if (conn == NULL)
    {
        printf("net: no memory\r\n");
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    netconn_bind(conn, IP_ADDR_ANY, 80);
    netconn_listen(conn);
    printf("net: http server listening on port 80\r\n");

    for (;;)
    {
        if (netconn_accept(conn, &newconn) == ERR_OK)
        {
            http_serve(newconn);
            netconn_delete(newconn);
        }
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");

    if (lwip_comm_init() != 0U)
    {
        printf("net: init failed\r\n");
    }

    xTaskCreate(http_task, "http", HTTP_TASK_STK_SIZE, NULL, HTTP_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
