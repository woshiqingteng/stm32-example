/**
 * @file    vApplicationHooks.c
 * @brief   FreeRTOS application hooks provided by the board port.
 *
 * Kept in the port so every FreeRTOS app links them without duplicating code;
 * `configCHECK_FOR_STACK_OVERFLOW = 2` is enabled in FreeRTOSConfig_common.h.
 *
 * The signature matches FreeRTOS' vApplicationStackOverflowHook(TaskHandle_t,
 * char*) without pulling in the FreeRTOS headers (TaskHandle_t is a pointer),
 * so this library does not depend on the `freertos` target. The port's CMake
 * force-references the symbol (-Wl,-u) so the object is always pulled in.
 */

#include <stdio.h>

void vApplicationStackOverflowHook(void *xTask, char *pcTaskName)
{
    (void)xTask;

    printf("stack overflow: %s\r\n", (pcTaskName != NULL) ? pcTaskName : "?");

    for (;;)
    {
    }
}
