/**
 * @file    main.c
 * @brief   56_usb_device_cdc (FreeRTOS + CherryUSB): USB CDC virtual COM port.
 *          Bytes received on the virtual COM port are printed on USART1 and
 *          echoed back to the host.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "usbd_core.h"
#include "usbd_cdc_acm.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define CDC_IN_EP    0x81U
#define CDC_OUT_EP   0x01U
#define CDC_INT_EP   0x83U

#define USBD_VID       0x0483U
#define USBD_PID       0x5740U
#define USBD_MAX_POWER 500U

#define CDC_RX_BUFFER_SIZE 256U
#define CDC_TASK_STK_SIZE  512U
#define CDC_TASK_PRIO      3U

/* STM32 unique device ID registers (used for the serial number string). */
#define DEVICE_UID0  (*(const uint32_t *)0x1FFF7A10U)
#define DEVICE_UID1  (*(const uint32_t *)0x1FFF7A14U)

/* ------------------------------------------------------------------ */
/* USB descriptors                                                     */
/* ------------------------------------------------------------------ */

#define USB_CONFIG_SIZE (9U + CDC_ACM_DESCRIPTOR_LEN)

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
};

static const uint8_t config_descriptor_fs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x00),
};

static const uint8_t device_quality_descriptor[] = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, 0x01),
};

static const uint8_t other_speed_config_descriptor_fs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x00),
};

static const char s_langid[] = { (char)0x09, (char)0x04 };
static char s_serial[25];

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return config_descriptor_fs;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_quality_descriptor;
}

static const uint8_t *other_speed_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return other_speed_config_descriptor_fs;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;

    switch (index)
    {
        case 0U:
            return s_langid;
        case 1U:
            return "STMicroelectronics";
        case 2U:
            return "ALIENTEK STM32F4 Virtual COM";
        case 3U:
            (void)snprintf(s_serial, sizeof(s_serial), "%08lX%08lX",
                           (unsigned long)DEVICE_UID0, (unsigned long)DEVICE_UID1);
            return s_serial;
        default:
            return NULL;
    }
}

static const struct usb_descriptor cdc_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback = other_speed_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback,
};

/* ------------------------------------------------------------------ */
/* CDC endpoints                                                       */
/* ------------------------------------------------------------------ */

static uint8_t g_rx_buffer[CDC_RX_BUFFER_SIZE];
static volatile uint32_t g_rx_len;
static volatile bool g_tx_busy;
static volatile bool g_connected;

static SemaphoreHandle_t g_rx_sem;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event)
    {
        case USBD_EVENT_RESET:
        case USBD_EVENT_DISCONNECTED:
            g_tx_busy = false;
            g_connected = false;
            break;

        case USBD_EVENT_CONFIGURED:
            g_connected = true;
            usbd_ep_start_read(busid, CDC_OUT_EP, g_rx_buffer, CDC_RX_BUFFER_SIZE);
            break;

        default:
            break;
    }
}

static void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    BaseType_t hp_woken = pdFALSE;

    (void)busid;
    (void)ep;

    g_rx_len = nbytes;
    (void)xSemaphoreGiveFromISR(g_rx_sem, &hp_woken);
    portYIELD_FROM_ISR(hp_woken);
}

static void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    if ((nbytes != 0U) && ((nbytes % usbd_get_ep_mps(busid, ep)) == 0U))
    {
        /* Send a zero-length packet to terminate a full-packet transfer. */
        (void)usbd_ep_start_write(busid, CDC_IN_EP, NULL, 0U);
    }
    else
    {
        g_tx_busy = false;
    }
}

static struct usbd_endpoint cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out,
};

static struct usbd_endpoint cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in,
};

static struct usbd_interface intf0;
static struct usbd_interface intf1;

static void cdc_send(const uint8_t *data, uint32_t len)
{
    while (g_tx_busy)
    {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    g_tx_busy = true;
    (void)usbd_ep_start_write(0, CDC_IN_EP, data, len);
}

/* Handle the received line, then print/echo it and re-arm the OUT transfer. */
static void cdc_handle_rx(void)
{
    uint32_t len = g_rx_len;
    char     line[CDC_RX_BUFFER_SIZE + 1U];

    if (len > CDC_RX_BUFFER_SIZE)
    {
        len = CDC_RX_BUFFER_SIZE;
    }

    (void)memcpy(line, (const void *)g_rx_buffer, len);
    line[len] = '\0';

    printf("usb rx %u bytes: %s\r\n", (unsigned)len, line);

    cdc_send(g_rx_buffer, len);

    g_rx_len = 0U;
    (void)usbd_ep_start_read(0, CDC_OUT_EP, g_rx_buffer, CDC_RX_BUFFER_SIZE);
}

static void cdc_task(void *argument)
{
    bool     connected = false;
    uint32_t blink     = 0U;

    (void)argument;

    for (;;)
    {
        if (xSemaphoreTake(g_rx_sem, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            cdc_handle_rx();
        }

        if (g_connected != connected)
        {
            connected = g_connected;
            printf(connected ? "USB Connected\r\n" : "USB DisConnected\r\n");
        }

        if (++blink >= 10U)
        {
            blink = 0U;
            led_toggle(LED0);
        }
    }
}

int main(void)
{
    bsp_init();

    /* USB OTG FS needs an exact 48 MHz kernel clock: 336 / 7 = 48 MHz while
     * keeping the core at 168 MHz. Must run before the scheduler starts. */
    (void)sys_clk_reconfig(336U, 25U, 2U, 7U);
    delay_init(168U);
    /* USART1 (APB2) changed with the clock, re-init to keep the baud rate. */
    usart_init(&(usart_cfg_t){ USART_CFG_DEFAULT(USART_ID_1) });

    printf(APP_BANNER "\r\n");

    g_rx_sem = xSemaphoreCreateBinary();
    configASSERT(g_rx_sem != NULL);

    usbd_desc_register(0, &cdc_descriptor);
    usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &intf0));
    usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &intf1));
    usbd_add_endpoint(0, &cdc_out_ep);
    usbd_add_endpoint(0, &cdc_in_ep);
    (void)usbd_initialize(0, USB_OTG_FS_PERIPH_BASE, usbd_event_handler);

    (void)xTaskCreate(cdc_task, "cdc", CDC_TASK_STK_SIZE, NULL, CDC_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
