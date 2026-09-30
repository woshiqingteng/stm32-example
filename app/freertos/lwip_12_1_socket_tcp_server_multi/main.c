/**
 * @file    main.c
 * @brief   lwip_12_1_socket_tcp_server_multi: multi-connection TCP server
 *          (ALIENTEK experiment 12-1). One task per accepted client.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "lwip_comm.h"
#include "lwip_demo_ui.h"

#define DEMO_TASK_PRIO      11
#define DEMO_TASK_STK_SIZE  1024
#define CLIENT_PRIO         11
#define CLIENT_STK_SIZE     512
#define LWIP_DEMO_PORT      8080

static const char s_sendbuf[] = "ALIENTEK DATA\r\n";

static void client_task(void *arg)
{
    int c = (int)(intptr_t)arg;
    char line[200];
    int n;

    for (;;)
    {
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
    vTaskDelete(NULL);
}

static void demo_task(void *arg)
{
    int s;
    int c;
    struct sockaddr_in local;
    struct sockaddr_in remote;
    socklen_t addr_len;

    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed("Ethernet Speed:100M");

    s = socket(AF_INET, SOCK_STREAM, 0);
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(LWIP_DEMO_PORT);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(s, (struct sockaddr *)&local, sizeof(local));
    listen(s, 4);

    for (;;)
    {
        addr_len = sizeof(remote);
        c = accept(s, (struct sockaddr *)&remote, &addr_len);
        if (c >= 0)
        {
            sys_thread_new("client", client_task, (void *)(intptr_t)c,
                           CLIENT_STK_SIZE, CLIENT_PRIO);
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

    lwip_comm_init();
    lwip_demo_ui_start("lwIP TCPServer MUTLienk Test");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
