/**
 * @file    main.c
 * @brief   lwip_12_socket_tcp_server: socket TCP server (experiment 12).
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "lwip_comm.h"
#include "lwip_demo_ui.h"

#define DEMO_TASK_PRIO      11
#define DEMO_TASK_STK_SIZE  1024
#define LWIP_DEMO_PORT      8080

static const char s_sendbuf[] = "ALIENTEK DATA\r\n";

static void demo_task(void *arg)
{
    int s;
    struct sockaddr_in local;
    struct sockaddr_in remote;
    socklen_t addr_len;

    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed("Ethernet Speed:100M");

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        lwip_demo_ui_retry();
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(LWIP_DEMO_PORT);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(s, (struct sockaddr *)&local, sizeof(local));
    listen(s, 4);

    for (;;)
    {
        int c;
        struct timeval tv;

        addr_len = sizeof(remote);
        c = accept(s, (struct sockaddr *)&remote, &addr_len);
        if (c < 0)
        {
            continue;
        }

        lwip_demo_ui_state("State:Connection Successful", BLUE);

        tv.tv_sec = 0;
        tv.tv_usec = LWIP_DEMO_RECV_TIMEOUT_MS * 1000U;
        setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        for (;;)
        {
            char line[200];
            int n;

            if ((g_lwip_send_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA)
            {
                (void)send(c, s_sendbuf, sizeof(s_sendbuf) - 1, 0);
                g_lwip_send_flag &= ~LWIP_SEND_DATA;
            }

            n = recv(c, line, sizeof(line) - 1, 0);
            if (n > 0)
            {
                line[n] = '\0';
                xQueueSend(g_display_queue, line, 0);
            }
            else if (n == 0)
            {
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }

        closesocket(c);
        lwip_demo_ui_state("State:Disconnect", BLUE);
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
    lwip_demo_ui_start("lwIP TCPServer Test");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
