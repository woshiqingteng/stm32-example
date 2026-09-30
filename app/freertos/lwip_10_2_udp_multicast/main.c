/**
 * @file    main.c
 * @brief   lwip_10_2_udp_multicast: socket UDP multicast (experiment 10-2).
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

#define GROUP_IP            "224.0.1.0"

static const char s_sendbuf[] = "ALIENTEK DATA\r\n";

static void demo_task(void *arg)
{
    int s;
    struct sockaddr_in local;
    struct sockaddr_in group;
    struct ip_mreq mreq;
    struct timeval tv;
    char line[200];

    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed("Ethernet Speed:100M");

    s = socket(AF_INET, SOCK_DGRAM, 0);
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

    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = inet_addr(GROUP_IP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    memset(&group, 0, sizeof(group));
    group.sin_family = AF_INET;
    group.sin_port = htons(LWIP_DEMO_PORT);
    group.sin_addr.s_addr = inet_addr(GROUP_IP);

    tv.tv_sec = 0;
    tv.tv_usec = LWIP_DEMO_RECV_TIMEOUT_MS * 1000U;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    for (;;)
    {
        if ((g_lwip_send_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA)
        {
            (void)sendto(s, s_sendbuf, sizeof(s_sendbuf) - 1, 0,
                         (struct sockaddr *)&group, sizeof(group));
            g_lwip_send_flag &= ~LWIP_SEND_DATA;
        }

        int n = recv(s, line, sizeof(line) - 1, 0);
        if (n > 0)
        {
            line[n] = '\0';
            xQueueSend(g_display_queue, line, 0);
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
    lwip_demo_ui_start("lwIP UDPMulticastTest");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
