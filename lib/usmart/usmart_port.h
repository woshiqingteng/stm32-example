/**
 * @file    usmart_port.h
 * @brief   USMART port layer: TIM4 1 us time base and USART1 byte input.
 *
 * Everything board specific lives here: the configuration macros, the command
 * line buffer fed by the USART1 receive hook, and the free running TIM4 counter
 * used to measure the execution time of the dispatched functions.
 */

#ifndef BSP_USMART_USMART_PORT_H
#define BSP_USMART_USMART_PORT_H

#include <stdint.h>
#include <stdio.h>

/* ---- User configuration ---- */

#define MAX_FNAME_LEN      30U   /*!< longest function signature text */
#define MAX_PARM           10U   /*!< maximum parameters per call */
#define PARM_LEN           200U  /*!< total parameter storage (bytes) */

#define USMART_ENTIMX_SCAN 1     /*!< 1: 1 us timer time base enabled for run-time */
#define USMART_USE_HELP    1     /*!< 0: drop the help text to save flash */
#define USMART_USE_WRFUNS  1     /*!< 1: expose read_addr()/write_addr() */

#define USMART_PRINTF      printf /*!< console output */

/* ---- TIM4 time base ---- */

#define USMART_TIMX_TICK_HZ      1000000U
#define USMART_TIMX_PERIOD       0xFFFFU

/** @brief  Return the pending command line, or 0 when none is ready. */
char *usmart_get_input_string(void);

/**
 * @brief  Start the TIM4 time base and register the USART1 receive hook.
 * @param  tclk System clock in MHz. The TIM4 input clock is derived from the
 *              APB1 prescaler so the tick is always 1 us.
 */
void usmart_port_init(uint16_t tclk);

/** @brief  Reset the run-time counter and its timer flag. */
void usmart_timx_reset_time(void);

/** @brief  Read the elapsed run-time in microseconds. */
uint32_t usmart_timx_get_time(void);

#endif /* BSP_USMART_USMART_PORT_H */
