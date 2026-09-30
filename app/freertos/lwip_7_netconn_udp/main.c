/**
 * @file    main.c
 * @brief   lwip_7_netconn_udp: netconn UDP peer (ALIENTEK experiment 7).
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
#define LWIP_DEMO_PORT      8080

/* Peer (PC) address. */
#define DEST_IP_ADDR0       192
#define DEST_IP_ADDR1       168
#define DEST_IP_ADDR2       2
#define DEST_IP_ADDR3       8

static const char s_sendbuf[] = "ALIENTEK DATA\r\n";

static void demo_task(void *arg)
{
    struct netconn *conn;
    struct netbuf *recvbuf;
    ip_addr_t destipaddr;

    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed("Ethernet Speed:100M");

    conn = netconn_new(NETCONN_UDP);
    if (conn == NULL)
    {
        lwip_demo_ui_retry();
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    IP4_ADDR(&destipaddr, DEST_IP_ADDR0, DEST_IP_ADDR1, DEST_IP_ADDR2, DEST_IP_ADDR3);
    netconn_bind(conn, IP_ADDR_ANY, LWIP_DEMO_PORT);
    netconn_connect(conn, &destipaddr, LWIP_DEMO_PORT);
    conn->recv_timeout = LWIP_DEMO_RECV_TIMEOUT_MS;

    for (;;)
    {
        if ((g_lwip_send_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA)
        {
            struct netbuf *sentbuf = netbuf_new();

            if (sentbuf != NULL)
            {
                netbuf_ref(sentbuf, s_sendbuf, sizeof(s_sendbuf) - 1);
                netconn_send(conn, sentbuf);
                netbuf_delete(sentbuf);
            }
            g_lwip_send_flag &= ~LWIP_SEND_DATA;
        }

        if (netconn_recv(conn, &recvbuf) == ERR_OK)
        {
            char line[200];
            u16_t n = netbuf_copy(recvbuf, line, sizeof(line) - 1);

            line[n] = '\0';
            xQueueSend(g_display_queue, line, 0);
            netbuf_delete(recvbuf);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
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
    lwip_demo_ui_start("lwIP UDP Test");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
