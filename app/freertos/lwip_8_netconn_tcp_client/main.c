/**
 * @file    main.c
 * @brief   lwip_8_netconn_tcp_client: netconn TCP client (experiment 8).
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
#define RETRY_DELAY_MS      1000

static const char s_sendbuf[] = "ALIENTEK DATA\r\n";

static void demo_task(void *arg)
{
    ip_addr_t destipaddr;

    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed("Ethernet Speed:100M");

    IP4_ADDR(&destipaddr, DEST_IP_ADDR0, DEST_IP_ADDR1, DEST_IP_ADDR2, DEST_IP_ADDR3);

    for (;;)    /* reconnect loop */
    {
        struct netconn *conn = netconn_new(NETCONN_TCP);

        if (conn != NULL && netconn_connect(conn, &destipaddr, LWIP_DEMO_PORT) == ERR_OK)
        {
            struct netbuf *recvbuf;

            lwip_demo_ui_state("State:Connection Successful", BLUE);
            conn->recv_timeout = LWIP_DEMO_RECV_TIMEOUT_MS;

            for (;;)
            {
                if ((g_lwip_send_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA)
                {
                    netconn_write(conn, s_sendbuf, sizeof(s_sendbuf) - 1, NETCONN_COPY);
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
                else if (netconn_err(conn) == ERR_CLSD)
                {
                    break;
                }

                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }

        if (conn != NULL)
        {
            netconn_close(conn);
            netconn_delete(conn);
        }

        lwip_demo_ui_state("State:Disconnect", BLUE);
        vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
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
    lwip_demo_ui_start("lwIP TCPClient Test");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
