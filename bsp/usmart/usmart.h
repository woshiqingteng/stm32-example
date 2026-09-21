/**
 * @file    usmart.h
 * @brief   USMART serial debug console: command parser and function table.
 *
 * Received lines are collected by a byte hook registered with the USART driver,
 * and usmart_scan() parses/executes them. TIM4 provides a 1 us time base used
 * for the optional function run-time measurement.
 */

#ifndef BSP_USMART_USMART_H
#define BSP_USMART_USMART_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"

#define USMART_MAX_FNAME_LEN 30U  /*!< longest function signature text */
#define USMART_MAX_PARM      10U  /*!< maximum parameters per call */
#define USMART_PARM_LEN      200U /*!< total parameter storage (bytes) */
#define USMART_RX_BUF_LEN    200U /*!< command line buffer (bytes) */

/** @brief Highest argument count the caller can dispatch (0..USMART_MAX_ARG_COUNT). */
#define USMART_MAX_ARG_COUNT 4U

/** @brief Parser result codes. */
typedef enum
{
    USMART_RES_OK = 0,
    USMART_RES_FUNCERR,
    USMART_RES_PARMERR,
    USMART_RES_PARMOVER,
    USMART_RES_NOFUNCFIND
} usmart_result_t;

/** @brief Parameter display base. */
typedef enum
{
    USMART_SP_DEC = 0,
    USMART_SP_HEX
} usmart_sptype_t;

/** @brief Stored parameter kind. */
typedef enum
{
    USMART_PARM_NUM = 0,
    USMART_PARM_STR
} usmart_parmtype_t;

/** @brief Command-line reception state. */
typedef enum
{
    USMART_RX_IDLE = 0,
    USMART_RX_RECEIVING,
    USMART_RX_READY
} usmart_rx_state_t;

/** @brief One callable function: entry point plus its declared signature. */
typedef struct
{
    void       *func;
    const char *name;
} usmart_nametab_t;

/** @brief USMART control block. */
typedef struct
{
    const usmart_nametab_t *funs;                     /*!< function table */
    uint8_t                 fnum;                     /*!< number of functions */
    uint8_t                 pnum;                     /*!< parameters of current call */
    uint8_t                 id;                       /*!< current function index */
    usmart_sptype_t         sptype;                   /*!< parameter display base */
    usmart_parmtype_t       parmtype[USMART_MAX_PARM];
    uint8_t                 plentbl[USMART_MAX_PARM];
    uint8_t                 parm[USMART_PARM_LEN];
    bool                    runtimeflag;              /*!< report run time if true */
    uint32_t                runtime;                  /*!< last run time in us */
} usmart_dev_t;

extern usmart_dev_t usmart_dev;

/** @brief  Initialise TIM4 and register the USART receive hook. */
void usmart_init(void);

/** @brief  Poll for a complete command and execute it. */
void usmart_scan(void);

/** @brief  Read a 32-bit word from an absolute address. */
uint32_t read_addr(uint32_t addr);

/** @brief  Write a 32-bit word to an absolute address. */
void write_addr(uint32_t addr, uint32_t val);

#endif /* BSP_USMART_USMART_H */
