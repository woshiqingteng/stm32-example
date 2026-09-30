/**
 * @file    main.c
 * @brief   lwip_10_1_udp_broadcast: socket UDP broadcast (experiment 10-1).
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
    int opt = 1;
    struct sockaddr_in baddr;
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
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    memset(&baddr, 0, sizeof(baddr));
    baddr.sin_family = AF_INET;
    baddr.sin_port = htons(LWIP_DEMO_PORT);
    baddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    tv.tv_sec = 0;
    tv.tv_usec = LWIP_DEMO_RECV_TIMEOUT_MS * 1000U;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    for (;;)
    {
        if ((g_lwip_send_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA)
        {
            (void)sendto(s, s_sendbuf, sizeof(s_sendbuf) - 1, 0,
                         (struct sockaddr *)&baddr, sizeof(baddr));
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
    lwip_demo_ui_start("lwIP UDPBroadcastTest");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
