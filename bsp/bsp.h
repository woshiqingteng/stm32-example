/**
 * @file    bsp.h
 * @brief   Board support package entry point and aggregated driver headers.
 */

#ifndef BSP_BSP_H
#define BSP_BSP_H

#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"

/** @brief  Initialise HAL, system clock, delay, USART1, LEDs and keys. */
void bsp_init(void);

#endif /* BSP_BSP_H */
