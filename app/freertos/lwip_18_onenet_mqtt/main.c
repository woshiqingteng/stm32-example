/**
 * @file    main.c
 * @brief   lwip_18_onenet_mqtt: OneNET MQTT client (ALIENTEK experiment 18).
 *
 * Plain MQTT (port 1883). The OneNET authorization token is derived with the
 * port's mbedTLS helpers; temperature/humidity are random, as in the demo.
 * (OneNET recommends TLS on mqtts.heclouds.com:8883 for production; this demo
 * stays plaintext to mirror the ALIENTEK example.)
 *
 * The credentials below are PLACEHOLDERS - replace them with your own.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
#define PRODUCT_ID          "YOUR_PRODUCT_ID"
#define DEVICE_NAME         "YOUR_DEVICE_NAME"
#define DEVICE_KEY          "YOUR_DEVICE_KEY"     /* base64 access key */
#define AUTH_VERSION        "2018-10-31"
#define TOKEN_EXPIRE_ET     1893456000U           /* placeholder: 2030-01-01 UTC */

#define HOST_NAME           "mqtt.heclouds.com"
#define HOST_PORT           1883
#define METHOD              "sha1"
#define ONENET_PUB_TOPIC    "$sys/"PRODUCT_ID"/"DEVICE_NAME"/thing/property/post"
#define ONENET_SUB_TOPIC    "$sys/"PRODUCT_ID"/"DEVICE_NAME"/thing/property/set"

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

static void url_encode(char *sign)
{
    char tmp[64];
    unsigned char i;
    unsigned char j = 0;
    unsigned char len = (unsigned char)strlen(sign);

    if (sign == NULL || len < 28)
    {
        return;
    }

    for (i = 0; i < len; i++)
    {
        tmp[i] = sign[i];
        sign[i] = 0;
    }
    tmp[i] = 0;

    for (i = 0; i < len; i++)
    {
        switch (tmp[i])
        {
        case '+': strcat(sign + j, "%2B"); j += 3; break;
        case ' ': strcat(sign + j, "%20"); j += 3; break;
        case '/': strcat(sign + j, "%2F"); j += 3; break;
        case '?': strcat(sign + j, "%3F"); j += 3; break;
        case '%': strcat(sign + j, "%25"); j += 3; break;
        case '#': strcat(sign + j, "%23"); j += 3; break;
        case '&': strcat(sign + j, "%26"); j += 3; break;
        case '=': strcat(sign + j, "%3D"); j += 3; break;
        default:  sign[j] = tmp[i]; j++; break;
        }
    }
    sign[j] = 0;
}

static int onenet_authorization(const char *ver, const char *res, unsigned int et,
                                const char *access_key, const char *dev_name,
                                char *buf, int buf_len, int flag)
{
    char hmac_buf[64];
    char key_bin[64];
    char sign_buf[64];
    char str_sig[80];
    size_t olen = 0;

    if (ver == NULL || res == NULL || access_key == NULL || buf == NULL || buf_len < 120)
    {
        return 1;
    }

    memset(key_bin, 0, sizeof(key_bin));
    if (lwip_base64_decode((uint8_t *)key_bin, sizeof(key_bin), &olen,
                           (const uint8_t *)access_key, strlen(access_key)) != 0)
    {
        return 1;
    }

    memset(str_sig, 0, sizeof(str_sig));
    if (flag)
    {
        snprintf(str_sig, sizeof(str_sig), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
    }
    else
    {
        snprintf(str_sig, sizeof(str_sig), "%d\n%s\nproducts/%s/devices/%s\n%s",
                 et, METHOD, res, dev_name, ver);
    }

    memset(hmac_buf, 0, sizeof(hmac_buf));
    if (lwip_hmac_sha1((const uint8_t *)key_bin, olen,
                       (const uint8_t *)str_sig, strlen(str_sig),
                       (uint8_t *)hmac_buf) != 0)
    {
        return 1;
    }

    olen = 0;
    memset(sign_buf, 0, sizeof(sign_buf));
    if (lwip_base64_encode((uint8_t *)sign_buf, sizeof(sign_buf), &olen,
                           (const uint8_t *)hmac_buf, LWIP_HMAC_SHA1_LEN) != 0)
    {
        return 1;
    }
    url_encode(sign_buf);

    if (flag)
    {
        snprintf(buf, buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s",
                 ver, res, et, METHOD, sign_buf);
    }
    else
    {
        snprintf(buf, buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s",
                 ver, res, dev_name, et, METHOD, sign_buf);
    }

    return 0;
}

static void demo_task(void *arg)
{
    struct hostent *he;
    struct mqtt_connect_client_info_t ci;
    static char token[256];
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

    if (onenet_authorization(AUTH_VERSION, PRODUCT_ID, TOKEN_EXPIRE_ET,
                             DEVICE_KEY, DEVICE_NAME, token, sizeof(token), 0) != 0
        || token[0] == '\0')   /* placeholder/invalid key: cannot build a token */
    {
        printf("mqtt: token build failed (set PRODUCT_ID/DEVICE_NAME/DEVICE_KEY)\r\n");
        lwip_demo_ui_retry();
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    memset(&ci, 0, sizeof(ci));
    ci.client_id   = DEVICE_NAME;
    ci.client_user = PRODUCT_ID;
    ci.client_pass = token;
    ci.keep_alive  = 60;

    s_client = mqtt_client_new();
    if (s_client == NULL)
    {
        lwip_demo_ui_retry();
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    cfg.sub_topic = ONENET_SUB_TOPIC;
    cfg.pub_topic = ONENET_PUB_TOPIC;
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
    lwip_demo_ui_start("lwIP MQTTOneNET");

    xTaskCreate(demo_task, "demo", DEMO_TASK_STK_SIZE, NULL, DEMO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
