/**
 * @file    main.c
 * @brief   lwip_11_socket_tcp_client: socket TCP client (ALIENTEK experiment 11).
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

#define DEST_IP_ADDR0       192
#define DEST_IP_ADDR1       168
#define DEST_IP_ADDR2       1
#define DEST_IP_ADDR3       111
#define DEMO_PORT           8080

static const char s_msg[] = "Hello from STM32F429 (socket TCP client)\r\n";

static void demo_task(void *arg)
{
    int s;
    struct sockaddr_in remote;
    char buf[256];

    (void)arg;

    lwip_comm_wait_ip();
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    s = socket(AF_INET, SOCK_STREAM, 0);
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(DEMO_PORT);
    remote.sin_addr.s_addr = inet_addr("192.168.1.111");

    if (connect(s, (struct sockaddr *)&remote, sizeof(remote)) == 0)
    {
        printf("net: connected to 192.168.1.111:%u\r\n", DEMO_PORT);
        (void)send(s, s_msg, sizeof(s_msg) - 1, 0);
        (void)recv(s, buf, sizeof(buf), 0);
    }
    else
    {
        printf("net: connect failed\r\n");
    }

    closesocket(s);

    for (;;)
    {
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
