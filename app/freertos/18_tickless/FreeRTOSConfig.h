/**
 * @file    FreeRTOSConfig.h
 * @brief   18_tickless specific FreeRTOS configuration.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_TICKLESS_IDLE                         1

#include "FreeRTOSConfig_common.h"

/* Tickless sleep hooks (implemented in main.c). */
void tickless_pre_sleep(void);
void tickless_post_sleep(void);
#define configPRE_SLEEP_PROCESSING(x)                   tickless_pre_sleep()
#define configPOST_SLEEP_PROCESSING(x)                  tickless_post_sleep()

#endif /* FREERTOS_CONFIG_H */
