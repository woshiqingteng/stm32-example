/**
 * @file    usbd_cdc_if.h
 * @brief   USB device CDC (virtual COM port) interface.
 */

#ifndef PORT_USBD_CDC_IF_H
#define PORT_USBD_CDC_IF_H

#include "usbd_cdc.h"

#define USB_USART_REC_LEN       200U
#define CDC_POLLING_INTERVAL    1U

/** @brief  Line reception state on the virtual COM port. */
typedef enum
{
    CDC_RX_STATE_IDLE = 0,  /*!< collecting data bytes */
    CDC_RX_STATE_SEEN_CR,   /*!< received CR, expecting LF */
    CDC_RX_STATE_DONE       /*!< a complete line is ready */
} cdc_rx_state_t;

extern uint8_t        g_usb_usart_rx_buffer[USB_USART_REC_LEN];
extern uint16_t       g_usb_usart_rx_len;
extern cdc_rx_state_t g_usb_usart_rx_state;

extern USBD_CDC_ItfTypeDef USBD_CDC_fops;

/** @brief  Send @p len bytes over the virtual COM port. */
void cdc_vcp_data_tx(uint8_t *buf, uint32_t len);

/** @brief  Feed received bytes into the line oriented receive buffer. */
void cdc_vcp_data_rx(uint8_t *buf, uint32_t len);

/** @brief  Format and send a string over the virtual COM port. */
void usb_printf(char *fmt, ...);

#endif /* PORT_USBD_CDC_IF_H */
