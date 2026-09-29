/**
 * @file    lv_port_tick.c
 * @brief   Inject the LVGL tick source (injected into the lvgl module).
 *
 * The switch on USE_FREERTOS stays in the board port, so the lvgl module keeps
 * no dependency on the OS or on this port (no reverse link edge).
 */

#include "lvgl_tick.h"
#include "delay.h"
#include "lv_port_tick.h"

#if USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"

static uint32_t tick_read(void)
{
    return (uint32_t)xTaskGetTickCount();
}
#else
#include "stm32f4xx_hal.h"

static uint32_t tick_read(void)
{
    return (uint32_t)HAL_GetTick();
}
#endif

void lv_port_tick_init(void)
{
    lvgl_tick_set(tick_read);
}
