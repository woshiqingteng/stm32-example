/**
 * @file    main.c
 * @brief   lwip_10_1_udp_broadcast: socket UDP broadcast (experiment 10-1).
 */

#include <stdio.h>
#include <string.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "lwip_comm.h"

#define DEMO_TASK_PRIO      4
#define DEMO_TASK_STK_SIZE  512
#define DEMO_PORT           8080

static const char s_msg[] = "STM32F429 UDP broadcast\r\n";

static void demo_task(void *arg)
{
    int s;
    int opt = 1;
    struct sockaddr_in broadcast_addr;

    (void)arg;

    lwip_comm_wait_ip();
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    s = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(DEMO_PORT);
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    printf("net: udp broadcast to 255.255.255.255:%u\r\n", DEMO_PORT);

    for (;;)
    {
        (void)sendto(s, s_msg, sizeof(s_msg) - 1, 0,
                     (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    bsp_init();
    printf(APP_BANNER "\r\n");

    lwip_comm_init();

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
