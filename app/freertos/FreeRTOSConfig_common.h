/**
 * @file    FreeRTOSConfig_common.h
 * @brief   Shared FreeRTOS configuration for all FreeRTOS apps.
 *
 * Each app has its own FreeRTOSConfig.h (found first on the include path) that
 * includes this file and only overrides the settings that differ, e.g.:
 *
 *     #define configTOTAL_HEAP_SIZE ((size_t)(32 * 1024))
 *     #include "FreeRTOSConfig_common.h"
 */

#ifndef FREERTOS_CONFIG_COMMON_H
#define FREERTOS_CONFIG_COMMON_H

#include <stdint.h>
#include <stdio.h>

extern uint32_t SystemCoreClock;

/* 基础配置项 */
#ifndef configUSE_PREEMPTION
#define configUSE_PREEMPTION                            1
#endif
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
#define configUSE_PORT_OPTIMISED_TASK_SELECTION         1
#endif
#ifndef configUSE_TICKLESS_IDLE
#define configUSE_TICKLESS_IDLE                         0
#endif
#ifndef configCPU_CLOCK_HZ
#define configCPU_CLOCK_HZ                              SystemCoreClock
#endif
#ifndef configTICK_RATE_HZ
#define configTICK_RATE_HZ                              1000
#endif
#ifndef configMAX_PRIORITIES
#define configMAX_PRIORITIES                            32
#endif
#ifndef configMINIMAL_STACK_SIZE
#define configMINIMAL_STACK_SIZE                        128
#endif
#ifndef configMAX_TASK_NAME_LEN
#define configMAX_TASK_NAME_LEN                         16
#endif
#ifndef configUSE_16_BIT_TICKS
#define configUSE_16_BIT_TICKS                          0
#endif
#ifndef configIDLE_SHOULD_YIELD
#define configIDLE_SHOULD_YIELD                         1
#endif
#ifndef configUSE_TASK_NOTIFICATIONS
#define configUSE_TASK_NOTIFICATIONS                    1
#endif
#ifndef configTASK_NOTIFICATION_ARRAY_ENTRIES
#define configTASK_NOTIFICATION_ARRAY_ENTRIES           1
#endif
#ifndef configUSE_MUTEXES
#define configUSE_MUTEXES                               1
#endif
#ifndef configUSE_RECURSIVE_MUTEXES
#define configUSE_RECURSIVE_MUTEXES                     1
#endif
#ifndef configUSE_COUNTING_SEMAPHORES
#define configUSE_COUNTING_SEMAPHORES                   1
#endif
#ifndef configUSE_ALTERNATIVE_API
#define configUSE_ALTERNATIVE_API                       0
#endif
#ifndef configQUEUE_REGISTRY_SIZE
#define configQUEUE_REGISTRY_SIZE                       8
#endif
#ifndef configUSE_QUEUE_SETS
#define configUSE_QUEUE_SETS                            1
#endif
#ifndef configUSE_TIME_SLICING
#define configUSE_TIME_SLICING                          1
#endif
#ifndef configUSE_NEWLIB_REENTRANT
#define configUSE_NEWLIB_REENTRANT                      0
#endif
#ifndef configENABLE_BACKWARD_COMPATIBILITY
#define configENABLE_BACKWARD_COMPATIBILITY             0
#endif
#ifndef configNUM_THREAD_LOCAL_STORAGE_POINTERS
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS         0
#endif
#ifndef configSTACK_DEPTH_TYPE
#define configSTACK_DEPTH_TYPE                          uint16_t
#endif
#ifndef configMESSAGE_BUFFER_LENGTH_TYPE
#define configMESSAGE_BUFFER_LENGTH_TYPE                size_t
#endif

/* 内存分配相关定义 */
#ifndef configSUPPORT_STATIC_ALLOCATION
#define configSUPPORT_STATIC_ALLOCATION                 0
#endif
#ifndef configSUPPORT_DYNAMIC_ALLOCATION
#define configSUPPORT_DYNAMIC_ALLOCATION                1
#endif
#ifndef configTOTAL_HEAP_SIZE
#define configTOTAL_HEAP_SIZE                           ((size_t)(10 * 1024))
#endif
#ifndef configAPPLICATION_ALLOCATED_HEAP
#define configAPPLICATION_ALLOCATED_HEAP                0
#endif
#ifndef configSTACK_ALLOCATION_FROM_SEPARATE_HEAP
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP       0
#endif

/* 钩子函数相关定义 */
#ifndef configUSE_IDLE_HOOK
#define configUSE_IDLE_HOOK                             0
#endif
#ifndef configUSE_TICK_HOOK
#define configUSE_TICK_HOOK                             0
#endif
#ifndef configCHECK_FOR_STACK_OVERFLOW
#define configCHECK_FOR_STACK_OVERFLOW                  0
#endif
#ifndef configUSE_MALLOC_FAILED_HOOK
#define configUSE_MALLOC_FAILED_HOOK                    0
#endif
#ifndef configUSE_DAEMON_TASK_STARTUP_HOOK
#define configUSE_DAEMON_TASK_STARTUP_HOOK              0
#endif

/* 运行时间和任务状态统计相关定义 */
#ifndef configGENERATE_RUN_TIME_STATS
#define configGENERATE_RUN_TIME_STATS                   0
#endif
#ifndef configUSE_TRACE_FACILITY
#define configUSE_TRACE_FACILITY                        1
#endif
#ifndef configUSE_STATS_FORMATTING_FUNCTIONS
#define configUSE_STATS_FORMATTING_FUNCTIONS            1
#endif

/* 协程相关定义 */
#ifndef configUSE_CO_ROUTINES
#define configUSE_CO_ROUTINES                           0
#endif
#ifndef configMAX_CO_ROUTINE_PRIORITIES
#define configMAX_CO_ROUTINE_PRIORITIES                 2
#endif

/* 软件定时器相关定义 */
#ifndef configUSE_TIMERS
#define configUSE_TIMERS                                1
#endif
#ifndef configTIMER_TASK_PRIORITY
#define configTIMER_TASK_PRIORITY                       ( configMAX_PRIORITIES - 1 )
#endif
#ifndef configTIMER_QUEUE_LENGTH
#define configTIMER_QUEUE_LENGTH                        5
#endif
#ifndef configTIMER_TASK_STACK_DEPTH
#define configTIMER_TASK_STACK_DEPTH                    ( configMINIMAL_STACK_SIZE * 2 )
#endif

/* 可选函数, 1: 使能 */
#ifndef INCLUDE_vTaskPrioritySet
#define INCLUDE_vTaskPrioritySet                        1
#endif
#ifndef INCLUDE_uxTaskPriorityGet
#define INCLUDE_uxTaskPriorityGet                       1
#endif
#ifndef INCLUDE_vTaskDelete
#define INCLUDE_vTaskDelete                             1
#endif
#ifndef INCLUDE_vTaskSuspend
#define INCLUDE_vTaskSuspend                            1
#endif
#ifndef INCLUDE_xResumeFromISR
#define INCLUDE_xResumeFromISR                          1
#endif
#ifndef INCLUDE_vTaskDelayUntil
#define INCLUDE_vTaskDelayUntil                         1
#endif
#ifndef INCLUDE_vTaskDelay
#define INCLUDE_vTaskDelay                              1
#endif
#ifndef INCLUDE_xTaskGetSchedulerState
#define INCLUDE_xTaskGetSchedulerState                  1
#endif
#ifndef INCLUDE_xTaskGetCurrentTaskHandle
#define INCLUDE_xTaskGetCurrentTaskHandle               1
#endif
#ifndef INCLUDE_uxTaskGetStackHighWaterMark
#define INCLUDE_uxTaskGetStackHighWaterMark             1
#endif
#ifndef INCLUDE_xTaskGetIdleTaskHandle
#define INCLUDE_xTaskGetIdleTaskHandle                  1
#endif
#ifndef INCLUDE_eTaskGetState
#define INCLUDE_eTaskGetState                           1
#endif
#ifndef INCLUDE_xEventGroupSetBitFromISR
#define INCLUDE_xEventGroupSetBitFromISR                1
#endif
#ifndef INCLUDE_xTimerPendFunctionCall
#define INCLUDE_xTimerPendFunctionCall                  1
#endif
#ifndef INCLUDE_xTaskAbortDelay
#define INCLUDE_xTaskAbortDelay                         1
#endif
#ifndef INCLUDE_xTaskGetHandle
#define INCLUDE_xTaskGetHandle                          1
#endif
#ifndef INCLUDE_xTaskResumeFromISR
#define INCLUDE_xTaskResumeFromISR                      1
#endif

/* 中断嵌套行为配置 */
#ifndef configPRIO_BITS
#ifdef __NVIC_PRIO_BITS
#define configPRIO_BITS __NVIC_PRIO_BITS
#else
#define configPRIO_BITS 4
#endif
#endif

#ifndef configLIBRARY_LOWEST_INTERRUPT_PRIORITY
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#endif
#ifndef configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#endif
#ifndef configKERNEL_INTERRUPT_PRIORITY
#define configKERNEL_INTERRUPT_PRIORITY                 ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#endif
#ifndef configMAX_SYSCALL_INTERRUPT_PRIORITY
#define configMAX_SYSCALL_INTERRUPT_PRIORITY            ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#endif
#ifndef configMAX_API_CALL_INTERRUPT_PRIORITY
#define configMAX_API_CALL_INTERRUPT_PRIORITY           configMAX_SYSCALL_INTERRUPT_PRIORITY
#endif

/* FreeRTOS中断服务函数相关定义 */
#ifndef xPortPendSVHandler
#define xPortPendSVHandler                              PendSV_Handler
#endif
#ifndef vPortSVCHandler
#define vPortSVCHandler                                 SVC_Handler
#endif

/* 断言 */
#ifndef vAssertCalled
#define vAssertCalled(char, int) printf("Error: %s, %d\r\n", char, int)
#endif
#ifndef configASSERT
#define configASSERT( x ) if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ )
#endif

#endif /* FREERTOS_CONFIG_COMMON_H */
