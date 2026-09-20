/**
 * @file    usart.h
 * @brief   USART1 interface.
 */

#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>

/** @brief  Initialise USART1 (PA9 TX / PA10 RX). @param baudrate Baud rate in bps. */
void usart_init(uint32_t baudrate);

#endif /* BSP_USART_H */
