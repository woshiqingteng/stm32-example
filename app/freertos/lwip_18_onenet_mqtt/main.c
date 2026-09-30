/**
 * @file    main.c
 * @brief   lwip_18_onenet_mqtt: OneNET MQTT client (ALIENTEK experiment 18).
 *
 * The OneNET-MQTTS authorization token is derived here (base64-decode the
 * access key, HMAC-SHA1 the signature string, base64-encode and URL-encode the
 * digest) using the port's mbedTLS helpers.
 *
 * The credentials below are PLACEHOLDERS - replace them with your own.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/apps/mqtt.h"
#include "lwip/netdb.h"

#include "lwip_comm.h"
#include "lwip_crypto.h"

#define DEMO_TASK_PRIO      4
#define DEMO_TASK_STK_SIZE  1024

/* ---- Placeholder credentials (replace before use) ---- */
#define PRODUCT_ID          "YOUR_PRODUCT_ID"
#define DEVICE_NAME         "YOUR_DEVICE_NAME"
#define DEVICE_KEY          "YOUR_DEVICE_KEY"     /* base64 access key */
#define AUTH_VERSION        "2018-10-31"
#define TOKEN_EXPIRE_ET     1893456000U           /* placeholder: 2030-01-01 UTC */

#define HOST_NAME           "mqtts.heclouds.com"
#define HOST_PORT           1883
#define METHOD              "sha1"
#define ONENET_PUB_TOPIC    "$sys/"PRODUCT_ID"/"DEVICE_NAME"/thing/property/post"
#define ONENET_SUB_TOPIC    "$sys/"PRODUCT_ID"/"DEVICE_NAME"/thing/property/set"

static mqtt_client_t *s_client;
static ip_addr_t s_broker;

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
    lwip_base64_encode((uint8_t *)sign_buf, sizeof(sign_buf), &olen,
                       (const uint8_t *)hmac_buf, LWIP_HMAC_SHA1_LEN);
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

static void pub_cb(void *arg, err_t err)
{
    (void)arg;
    printf("mqtt: publish err %d\r\n", (int)err);
}

static void sub_cb(void *arg, err_t err)
{
    (void)arg;
    printf("mqtt: subscribe err %d\r\n", (int)err);
}

static void in_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    (void)arg;
    (void)data;
    (void)flags;
    printf("mqtt: rx %u bytes\r\n", (unsigned)len);
}

static void in_pub_cb(void *arg, const char *topic, u32_t tot_len)
{
    (void)arg;
    printf("mqtt: rx topic %s len %u\r\n", topic, (unsigned)tot_len);
}

static void conn_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    (void)arg;
    printf("mqtt: connect status %d\r\n", (int)status);

    if (status == MQTT_CONNECT_ACCEPTED)
    {
        mqtt_set_inpub_callback(client, in_pub_cb, in_data_cb, NULL);
        mqtt_subscribe(client, ONENET_SUB_TOPIC, 1, sub_cb, NULL);
        mqtt_publish(client, ONENET_PUB_TOPIC, "{\"temp\":25.0}", 13, 1, 0, pub_cb, NULL);
    }
}

static void demo_task(void *arg)
{
    struct hostent *he;
    struct mqtt_connect_client_info_t ci;
    static char token[256];

    (void)arg;

    lwip_comm_wait_ip();
    printf("net: ip %s\r\n", ip4addr_ntoa(netif_ip4_addr(&g_lwip_netif)));

    he = gethostbyname(HOST_NAME);
    if (he == NULL)
    {
        printf("mqtt: dns lookup failed\r\n");
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    memcpy(&s_broker, he->h_addr, he->h_length);

    if (onenet_authorization(AUTH_VERSION, PRODUCT_ID, TOKEN_EXPIRE_ET,
                             DEVICE_KEY, DEVICE_NAME, token, sizeof(token), 0) != 0)
    {
        printf("mqtt: token failed\r\n");
    }

    memset(&ci, 0, sizeof(ci));
    ci.client_id   = DEVICE_NAME;
    ci.client_user = PRODUCT_ID;
    ci.client_pass = token;
    ci.keep_alive  = 60;

    s_client = mqtt_client_new();
    mqtt_client_connect(s_client, &s_broker, HOST_PORT, conn_cb, NULL, &ci);

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
