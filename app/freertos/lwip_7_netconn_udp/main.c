/**
 * @file    main.c
 * @brief   lwip_7_netconn_udp: netconn UDP peer (ALIENTEK experiment 7).
 *
 * Binds 8080, connects to the PC peer, sends "ALIENTEK DATA" on KEY0 and shows
 * received data on the LCD (mirrors the ALIENTEK demo).
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

#define DEST_IP_ADDR0       192
#define DEST_IP_ADDR1       168
#define DEST_IP_ADDR2       1
#define DEST_IP_ADDR3       111
#define LWIP_DEMO_PORT      8080

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
    netconn_bind(conn, IP_ADDR_ANY, LWIP_DEMO_PORT);
    IP4_ADDR(&destipaddr, DEST_IP_ADDR0, DEST_IP_ADDR1, DEST_IP_ADDR2, DEST_IP_ADDR3);
    netconn_connect(conn, &destipaddr, LWIP_DEMO_PORT);
    conn->recv_timeout = LWIP_DEMO_RECV_TIMEOUT_MS;

    for (;;)
    {
        if ((g_lwip_send_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA)
        {
            struct netbuf *sentbuf = netbuf_new();
            netbuf_ref(sentbuf, s_sendbuf, sizeof(s_sendbuf) - 1);
            netconn_send(conn, sentbuf);
            netbuf_delete(sentbuf);
            g_lwip_send_flag &= ~LWIP_SEND_DATA;
        }

        if (netconn_recv(conn, &recvbuf) == ERR_OK)
        {
            char line[200];
            netbuf_copy(recvbuf, line, sizeof(line) - 1);
            line[sizeof(line) - 1] = '\0';
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
    g_lwip_font_ok = (fonts_init() == 0U) ? 1U : 0U;

    lwip_comm_init();
    lwip_demo_ui_start("lwIP UDP Test");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
