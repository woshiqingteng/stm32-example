/**
 * @file    lv_port.h
 * @brief   LVGL port for the openedv_stm32f4 board: display, input and tick.
 */

#ifndef LV_PORT_H
#define LV_PORT_H

#include <stdint.h>

void     lv_port_disp_init(void);     /* RGB/LTDC display driver */
void     lv_port_indev_init(void);    /* GT9147 touch input driver */
void     lv_port_fs_init(void);       /* FatFs filesystem driver ('0:') */
uint32_t lv_port_tick_get(void);      /* ms tick used by LV_TICK_CUSTOM */

#endif /* LV_PORT_H */
