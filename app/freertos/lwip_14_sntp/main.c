/**
 * @file    main.c
 * @brief   lwip_14_sntp: SNTP client using lwIP's sntp app (experiment 14).
 */

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"
#include "rtc.h"
#include "text.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/apps/sntp.h"

#include "lwip_comm.h"
#include "lwip_demo_ui.h"

#define DEMO_TASK_PRIO      11
#define DEMO_TASK_STK_SIZE  1024

#define SNTP_SERVER_NAME    "ntp1.aliyun.com"

/* Called by lwIP SNTP (via SNTP_SET_SYSTEM_TIME in lwipopts.h). */
volatile uint32_t g_sntp_time;
void lwip_sntp_set_time(uint32_t sec)
{
    g_sntp_time = sec;
}

static void demo_task(void *arg)
{
    uint32_t last = 0;

    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed("Ethernet Speed:100M");

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, SNTP_SERVER_NAME);
    sntp_init();

    for (;;)
    {
        if (g_sntp_time != 0U && g_sntp_time != last)
        {
            time_t t = (time_t)g_sntp_time;
            struct tm *ti = localtime(&t);
            char line[40];

            last = g_sntp_time;

            rtc_set_date((uint8_t)(ti->tm_year + 1900 - 2000), (uint8_t)(ti->tm_mon + 1),
                         (uint8_t)ti->tm_mday, (uint8_t)(ti->tm_wday + 1));
            rtc_set_time((uint8_t)(ti->tm_hour + 8), (uint8_t)ti->tm_min, (uint8_t)ti->tm_sec, RTC_AM_24H);

            sprintf(line, "20%02d-%02d-%02d %02d:%02d:%02d",
                    ti->tm_year % 100, ti->tm_mon + 1, ti->tm_mday,
                    (ti->tm_hour + 8) % 24, ti->tm_min, ti->tm_sec);
            lwip_demo_ui_show(5, 150, 16, line, MAGENTA);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
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

    rtc_init();

    lwip_comm_init();
    lwip_demo_ui_start("lwIP SNTP Test");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
