/**
 * @file    FreeRTOSConfig.h
 * @brief   11_2_run_time_stats specific FreeRTOS configuration.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Run-time statistics: the counter is maintained by a TIM6 ISR in the app. */
#define configGENERATE_RUN_TIME_STATS                   1
extern volatile uint32_t g_rtos_run_ticks;
void rtos_runtime_timer_init(void);
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()        rtos_runtime_timer_init()
#define portGET_RUN_TIME_COUNTER_VALUE()                g_rtos_run_ticks

#include "FreeRTOSConfig_common.h"

#endif /* FREERTOS_CONFIG_H */
