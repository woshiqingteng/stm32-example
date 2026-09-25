/**
 * @file    usbd_cdc_if.h
 * @brief   USB device CDC (virtual COM port) interface.
 */

#ifndef PORT_USBD_CDC_IF_H
#define PORT_USBD_CDC_IF_H

#include "usbd_cdc.h"

#define USB_USART_REC_LEN       200U
#define CDC_POLLING_INTERVAL    1U

extern uint8_t  g_usb_usart_rx_buffer[USB_USART_REC_LEN];
extern uint16_t g_usb_usart_rx_sta;

extern USBD_CDC_ItfTypeDef USBD_CDC_fops;

/** @brief  Send @p len bytes over the virtual COM port. */
void cdc_vcp_data_tx(uint8_t *buf, uint32_t len);

/** @brief  Feed received bytes into the line oriented receive buffer. */
void cdc_vcp_data_rx(uint8_t *buf, uint32_t len);

/** @brief  Format and send a string over the virtual COM port. */
void usb_printf(char *fmt, ...);

#endif /* PORT_USBD_CDC_IF_H */
