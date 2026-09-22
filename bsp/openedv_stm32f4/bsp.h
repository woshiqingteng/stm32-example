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
#include "exti.h"
#include "wdg.h"
#include "btim.h"
#include "gtim.h"
#include "atim.h"
#include "tpad.h"
#include "adc.h"
#include "dac.h"
#include "pwmdac.h"
#include "stmflash.h"
#include "rtc.h"
#include "rng.h"
#include "pwr.h"
#include "sdram.h"
#include "lcd.h"
#include "oled.h"
#include "iic.h"
#include "24cxx.h"
#include "pcf8574.h"
#include "ap3216c.h"
#include "touch.h"
#include "spa06.h"
#include "qmi8658a.h"

/** @brief  Initialise HAL, system clock, delay, USART1, LEDs and keys. */
void bsp_init(void);

#endif /* BSP_BSP_H */
