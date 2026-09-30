/**
 * @file    main.c
 * @brief   lwip_13_ntp: NTP client over UDP (ALIENTEK experiment 13).
 *
 * The DNS name is resolved with lwIP's resolver; the NTP v4 packet is built by
 * hand (no lwIP SNTP app).
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"

#include "lwip_comm.h"

#define DEMO_TASK_PRIO      4
#define DEMO_TASK_STK_SIZE  512

#define NTP_SERVER          "ntp1.aliyun.com"
#define NTP_PORT            123
#define NTP_MSG_LEN         48
#define NTP_TIMESTAMP_DELTA 2208988800UL

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void demo_task(void *arg)
{
    struct hostent *he;
    struct sockaddr_in server;
    int s;
    uint8_t pkt[NTP_MSG_LEN];

    (void)arg;

    lwip_comm_wait_ip();
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    he = gethostbyname(NTP_SERVER);
    if (he == NULL)
    {
        printf("ntp: dns lookup failed\r\n");
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(NTP_PORT);
    memcpy(&server.sin_addr, he->h_addr, he->h_length);

    memset(pkt, 0, sizeof(pkt));
    pkt[0] = 0x1BU; /* LI=0, VN=3, Mode=3 (client) */

    if (sendto(s, pkt, sizeof(pkt), 0, (struct sockaddr *)&server, sizeof(server)) == (int)sizeof(pkt))
    {
        if (recv(s, pkt, sizeof(pkt), 0) >= (int)sizeof(pkt))
        {
            uint32_t secs = be32(&pkt[40]) - (uint32_t)NTP_TIMESTAMP_DELTA;
            printf("ntp: unix time %lu\r\n", (unsigned long)secs);
        }
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
