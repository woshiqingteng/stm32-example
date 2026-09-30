/**
 * @file    main.c
 * @brief   lwip_16_http: lwIP netconn HTTP server (ALIENTEK experiment 16).
 */

#include <stdio.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/api.h"

#include "lwip_comm.h"
#include "lwip_demo_ui.h"

#define DEMO_TASK_PRIO      11
#define DEMO_TASK_STK_SIZE  1024

#define HTTP_PORT           80

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

static void demo_task(void *arg)
{
    struct netconn *conn;
    struct netconn *newconn;

    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed("Ethernet Speed:100M");

    conn = netconn_new(NETCONN_TCP);
    if (conn == NULL)
    {
        lwip_demo_ui_retry();
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }
    netconn_bind(conn, IP_ADDR_ANY, HTTP_PORT);
    netconn_listen(conn);

    for (;;)
    {
        if (netconn_accept(conn, &newconn) == ERR_OK)
        {
            lwip_demo_ui_state("State:Connection Successful", BLUE);
            http_serve(newconn);
            netconn_delete(newconn);
            lwip_demo_ui_state("State:Disconnect", BLUE);
        }
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");

    sdram_init();
    lcd_init();
    lcd_display_dir(LCD_DIR_PORTRAIT);
    lcd_clear(WHITE);

    if (lwip_comm_init() != 0)
    {
        lwip_demo_ui_retry();
        for (;;)
        {
        }
    }
    lwip_demo_ui_start("lwIP HTTP Test");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
