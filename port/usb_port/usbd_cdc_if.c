/**
 * @file    usbd_cdc_if.c
 * @brief   USB device CDC (virtual COM port) interface. Incoming bytes are
 *          collected line by line; the application forwards them to USART1 and
 *          the LCD. Ported from the vendor usbd_cdc_interface.c.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "usbd_cdc_if.h"
#include "delay.h"

#define USB_USART_CMD_END   0x8000U
#define USB_USART_CR        0x4000U

extern USBD_HandleTypeDef USBD_Device;

static USBD_CDC_LineCodingTypeDef g_line_coding = {
    115200U,    /* bitrate   */
    0x00U,      /* stop bits */
    0x00U,      /* parity    */
    0x08U       /* data bits */
};

static uint8_t g_usb_printf_buffer[USB_USART_REC_LEN];
static uint8_t g_usb_rx_buffer[USB_USART_REC_LEN];

uint8_t  g_usb_usart_rx_buffer[USB_USART_REC_LEN];
uint16_t g_usb_usart_rx_sta = 0U;

static int8_t CDC_Itf_Init(void);
static int8_t CDC_Itf_DeInit(void);
static int8_t CDC_Itf_Control(uint8_t cmd, uint8_t *pbuf, uint16_t length);
static int8_t CDC_Itf_Receive(uint8_t *buf, uint32_t *len);
static int8_t CDC_Itf_TransmitCplt(uint8_t *buf, uint32_t *len, uint8_t epnum);

USBD_CDC_ItfTypeDef USBD_CDC_fops = {
    CDC_Itf_Init,
    CDC_Itf_DeInit,
    CDC_Itf_Control,
    CDC_Itf_Receive,
    CDC_Itf_TransmitCplt,
};

static int8_t CDC_Itf_Init(void)
{
    (void)USBD_CDC_SetRxBuffer(&USBD_Device, g_usb_rx_buffer);
    return (int8_t)USBD_OK;
}

static int8_t CDC_Itf_DeInit(void)
{
    return (int8_t)USBD_OK;
}

static int8_t CDC_Itf_Control(uint8_t cmd, uint8_t *pbuf, uint16_t length)
{
    (void)length;

    switch (cmd)
    {
        case CDC_SET_LINE_CODING:
            g_line_coding.bitrate    = (uint32_t)(pbuf[0] | (pbuf[1] << 8) |
                                                  (pbuf[2] << 16) | (pbuf[3] << 24));
            g_line_coding.format     = pbuf[4];
            g_line_coding.paritytype = pbuf[5];
            g_line_coding.datatype   = pbuf[6];
            break;

        case CDC_GET_LINE_CODING:
            pbuf[0] = (uint8_t)(g_line_coding.bitrate);
            pbuf[1] = (uint8_t)(g_line_coding.bitrate >> 8);
            pbuf[2] = (uint8_t)(g_line_coding.bitrate >> 16);
            pbuf[3] = (uint8_t)(g_line_coding.bitrate >> 24);
            pbuf[4] = g_line_coding.format;
            pbuf[5] = g_line_coding.paritytype;
            pbuf[6] = g_line_coding.datatype;
            break;

        default:
            break;
    }

    return (int8_t)USBD_OK;
}

static int8_t CDC_Itf_Receive(uint8_t *buf, uint32_t *len)
{
    (void)USBD_CDC_ReceivePacket(&USBD_Device);
    cdc_vcp_data_rx(buf, *len);
    return (int8_t)USBD_OK;
}

static int8_t CDC_Itf_TransmitCplt(uint8_t *buf, uint32_t *len, uint8_t epnum)
{
    (void)buf;
    (void)len;
    (void)epnum;
    return (int8_t)USBD_OK;
}

void cdc_vcp_data_rx(uint8_t *buf, uint32_t len)
{
    uint32_t i;
    uint8_t  res;

    for (i = 0U; i < len; i++)
    {
        res = buf[i];

        if ((g_usb_usart_rx_sta & USB_USART_CMD_END) == 0U)
        {
            if ((g_usb_usart_rx_sta & USB_USART_CR) != 0U)
            {
                if (res != 0x0AU)
                {
                    g_usb_usart_rx_sta = 0U;
                }
                else
                {
                    g_usb_usart_rx_sta |= USB_USART_CMD_END;
                }
            }
            else if (res == 0x0DU)
            {
                g_usb_usart_rx_sta |= USB_USART_CR;
            }
            else
            {
                g_usb_usart_rx_buffer[g_usb_usart_rx_sta & 0x3FFFU] = res;
                g_usb_usart_rx_sta++;

                if (g_usb_usart_rx_sta > (USB_USART_REC_LEN - 1U))
                {
                    g_usb_usart_rx_sta = 0U;
                }
            }
        }
    }
}

void cdc_vcp_data_tx(uint8_t *buf, uint32_t len)
{
    (void)USBD_CDC_SetTxBuffer(&USBD_Device, buf, len);
    (void)USBD_CDC_TransmitPacket(&USBD_Device);
    delay_ms(CDC_POLLING_INTERVAL);
}

void usb_printf(char *fmt, ...)
{
    va_list ap;
    uint16_t len;

    va_start(ap, fmt);
    (void)vsprintf((char *)g_usb_printf_buffer, fmt, ap);
    va_end(ap);

    len = (uint16_t)strlen((const char *)g_usb_printf_buffer);
    cdc_vcp_data_tx(g_usb_printf_buffer, len);
}
