/**
 * @file    main.c
 * @brief   lwip_17_aliyun_mqtt: Aliyun IoT MQTT client (ALIENTEK experiment 17).
 *
 * Plain MQTT (port 1883). Password = lowercase hex of HMAC-SHA1(device_secret,
 * content). Temperature/humidity are random, as in the ALIENTEK demo.
 *
 * The credentials below are PLACEHOLDERS - replace them with your own.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "lcd.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/apps/mqtt.h"
#include "lwip/netdb.h"

#include "lwip_comm.h"
#include "lwip_crypto.h"
#include "lwip_demo_ui.h"
#include "lwip_mqtt_common.h"

#define DEMO_TASK_PRIO      11
#define DEMO_TASK_STK_SIZE  1024

/* ---- Placeholder credentials (replace before use) ---- */
#define PRODUCT_KEY         "YOUR_PRODUCT_KEY"
#define DEVICE_NAME         "YOUR_DEVICE_NAME"
#define DEVICE_SECRET       "YOUR_DEVICE_SECRET"

#define HOST_NAME           PRODUCT_KEY ".iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define HOST_PORT           1883
#define CONTENT             "clientId"DEVICE_NAME"deviceName"DEVICE_NAME"productKey"PRODUCT_KEY"timestamp789"
#define CLIENT_ID           DEVICE_NAME"|securemode=3,signmethod=hmacsha1,timestamp=789|"
#define USER_NAME           DEVICE_NAME"&"PRODUCT_KEY
#define DEVICE_PUBLISH      "/sys/"PRODUCT_KEY"/"DEVICE_NAME"/thing/event/property/post"
#define DEVICE_SUBSCRIBE    "/sys/"PRODUCT_KEY"/"DEVICE_NAME"/thing/service/property/set"

static mqtt_client_t *s_client;
static ip_addr_t s_broker;

static void ui_display(const char *text)
{
    char line[200];

    strncpy(line, (text != NULL) ? text : "", sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    xQueueSend(g_display_queue, line, 0);
}

static void ui_state(const char *text)
{
    lwip_demo_ui_state(text, BLUE);
}

static void hex_str(const uint8_t *bin, int n, char *out)
{
    int i;

    for (i = 0; i < n; i++)
    {
        sprintf(out + i * 2, "%02x", bin[i]);
    }
    out[n * 2] = '\0';
}

static void demo_task(void *arg)
{
    struct hostent *he;
    struct mqtt_connect_client_info_t ci;
    uint8_t digest[LWIP_HMAC_SHA1_LEN];
    static char password[LWIP_HMAC_SHA1_LEN * 2 + 1];
    lwip_mqtt_cfg_t cfg;

    (void)arg;

    lwip_comm_wait_ip();
    lwip_demo_ui_ip(ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));
    lwip_demo_ui_speed("Ethernet Speed:100M");

    he = gethostbyname(HOST_NAME);
    if (he == NULL)
    {
        lwip_demo_ui_retry();
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }
    memcpy(&s_broker, he->h_addr, he->h_length);

    (void)lwip_hmac_sha1((const uint8_t *)DEVICE_SECRET, strlen(DEVICE_SECRET),
                         (const uint8_t *)CONTENT, strlen(CONTENT), digest);
    hex_str(digest, LWIP_HMAC_SHA1_LEN, password);

    memset(&ci, 0, sizeof(ci));
    ci.client_id   = CLIENT_ID;
    ci.client_user = USER_NAME;
    ci.client_pass = password;
    ci.keep_alive  = 60;

    s_client = mqtt_client_new();
    if (s_client == NULL)
    {
        lwip_demo_ui_retry();
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    cfg.sub_topic = DEVICE_SUBSCRIBE;
    cfg.pub_topic = DEVICE_PUBLISH;
    cfg.display   = ui_display;
    cfg.state     = ui_state;
    lwip_mqtt_init(&cfg);

    mqtt_client_connect(s_client, &s_broker, HOST_PORT, lwip_mqtt_conn_cb, NULL, &ci);

    for (;;)
    {
        if (lwip_mqtt_poll(s_client))
        {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(500));
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

    if (lwip_comm_init() != 0)
    {
        lwip_demo_ui_retry();
        for (;;)
        {
        }
    }
    lwip_demo_ui_start("lwIP MQTTAliyun");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
