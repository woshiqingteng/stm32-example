/**
 * @file    main.c
 * @brief   lwip_9_netconn_tcp_server: netconn TCP server (experiment 9).
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"
#include "text.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"

#include "lwip_comm.h"
#include "lwip_demo_ui.h"

#define DEMO_TASK_PRIO      11
#define DEMO_TASK_STK_SIZE  1024
#define LWIP_DEMO_PORT      8080

static const char s_sendbuf[] = "ALIENTEK DATA\r\n";

static void demo_task(void *arg)
{
    struct netconn *conn;
    struct netconn *newconn;
    struct netbuf *recvbuf;

    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed("Ethernet Speed:100M");

    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, IP_ADDR_ANY, LWIP_DEMO_PORT);
    netconn_listen(conn);
    conn->recv_timeout = LWIP_DEMO_RECV_TIMEOUT_MS;

    for (;;)
    {
        if (netconn_accept(conn, &newconn) == ERR_OK)
        {
            lwip_demo_ui_state("State:Connection Successful", BLUE);
            newconn->recv_timeout = LWIP_DEMO_RECV_TIMEOUT_MS;

            for (;;)
            {
                if ((g_lwip_send_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA)
                {
                    netconn_write(newconn, s_sendbuf, sizeof(s_sendbuf) - 1, NETCONN_COPY);
                    g_lwip_send_flag &= ~LWIP_SEND_DATA;
                }

                if (netconn_recv(newconn, &recvbuf) == ERR_OK)
                {
                    char line[200];
                    netbuf_copy(recvbuf, line, sizeof(line) - 1);
                    line[sizeof(line) - 1] = '\0';
                    xQueueSend(g_display_queue, line, 0);
                    netbuf_delete(recvbuf);
                }
                else if (netconn_err(newconn) == ERR_CLSD)
                {
                    break;
                }

                vTaskDelay(pdMS_TO_TICKS(10));
            }

            netconn_close(newconn);
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
    lcd_display_dir(LCD_DIR_LANDSCAPE);
    lcd_clear(WHITE);
    g_lwip_font_ok = (fonts_init() == 0U) ? 1U : 0U;

    lwip_comm_init();
    lwip_demo_ui_start("lwIP TCPServer Test");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
