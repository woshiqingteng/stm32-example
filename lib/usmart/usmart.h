/**
 * @file    usmart.h
 * @brief   USMART serial debug console: function table and public API.
 *
 * USMART lets a serial terminal call any function listed in usmart_nametab[]
 * with numeric (decimal/hex, signed), string or pointer arguments and prints the
 * return value. Each table entry also carries an exact-typed trampoline so the
 * dispatcher never calls through an incompatible function-pointer type.
 */

#ifndef BSP_USMART_USMART_H
#define BSP_USMART_USMART_H

#include <stdbool.h>
#include <stdint.h>
#include "usmart_port.h"

/** @brief Parser / dispatcher status codes. */
typedef enum
{
    USMART_OK = 0,      /*!< success */
    USMART_FUNCERR,     /*!< malformed function or unknown command */
    USMART_PARMERR,     /*!< malformed parameter */
    USMART_PARMOVER,    /*!< too many parameters */
    USMART_NOFUNCFIND   /*!< no matching function in the table */
} usmart_status_t;

/** @brief Non-string parameter display base. */
typedef enum
{
    SP_TYPE_DEC = 0,    /*!< decimal display */
    SP_TYPE_HEX = 1     /*!< hexadecimal display */
} usmart_sptype_t;

/** @brief Run-time reporting state. */
typedef enum
{
    USMART_RUNTIME_OFF = 0, /*!< do not report run-time */
    USMART_RUNTIME_ON  = 1  /*!< report run-time */
} usmart_runtime_t;

/** @brief Exact-typed trampoline: invoke func with the parsed arguments. */
typedef uint32_t (*usmart_call_t)(void *func, const uint32_t *args);

/** @brief One callable function: entry point, signature text and trampoline. */
struct _m_usmart_nametab
{
    void          *func;
    const char    *name;
    usmart_call_t  call;
};

/** @brief USMART control block. */
struct _m_usmart_dev
{
    struct _m_usmart_nametab *funs;                 /*!< function table */
    void (*init)(uint16_t tclk);                    /*!< initialise */
    usmart_status_t (*cmd_rec)(char *str);          /*!< match function and args */
    void (*exe)(void);                              /*!< execute */
    void (*scan)(void);                             /*!< poll the input stream */

    uint8_t         fnum;                           /*!< number of functions */
    uint8_t         pnum;                           /*!< parameters of current call */
    uint8_t         id;                             /*!< current function index */
    usmart_sptype_t sptype;                         /*!< parameter display base */
    uint16_t        parmtype;                       /*!< per-argument type bits (1 = string) */
    uint8_t         plentbl[MAX_PARM];              /*!< per-argument length scratch */
    uint8_t         parm[PARM_LEN];                 /*!< packed argument storage */
    usmart_runtime_t runtimeflag;                   /*!< report run-time state */
    uint32_t        runtime;                        /*!< last run-time in microseconds */
};

extern struct _m_usmart_nametab usmart_nametab[];
extern struct _m_usmart_dev usmart_dev;

/** @brief  Initialise the port and set the default parameter display base. */
void usmart_init(uint16_t tclk);

/** @brief  Match a received line against the function table and store its args. */
usmart_status_t usmart_cmd_rec(char *str);

/** @brief  Execute the function selected by the last usmart_cmd_rec(). */
void usmart_exe(void);

/** @brief  Poll for a complete command and run it (call from the main loop). */
void usmart_scan(void);

/** @brief  Read a 32-bit word from an absolute address. */
uint32_t read_addr(uint32_t addr);

/** @brief  Write a 32-bit word to an absolute address. */
void write_addr(uint32_t addr, uint32_t val);

#endif /* BSP_USMART_USMART_H */
