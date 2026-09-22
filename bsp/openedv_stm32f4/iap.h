/**
 * @file    iap.h
 * @brief   In-application programming: USART firmware receive, flash program
 *          and application jump.
 */

#ifndef BSP_IAP_H
#define BSP_IAP_H

#include <stdint.h>

/** @brief Application base address (after the bootloader). */
#define IAP_APP_ADDR 0x08010000U

typedef enum
{
    IAP_OK = 0,
    IAP_ERR_PARAM,
    IAP_ERR_ERASE,
    IAP_ERR_WRITE,
    IAP_ERR_TIMEOUT,
    IAP_ERR_FRAME,
} iap_status_t;

/** @brief  Erase the sectors covering [addr, addr+len) and program the buffer. */
iap_status_t iap_write_appbin(uint32_t addr, const uint8_t *buf, uint32_t len);

/** @brief  Jump to the application at addr (never returns on success). */
void iap_jump(uint32_t addr);

/**
 * @brief  Receive one firmware image over USART1 and program it at IAP_APP_ADDR.
 *         Frame: 0x5A 0xA5 <len_lo> <len_hi> <data...> <sum8>.
 */
iap_status_t iap_receive_usart(void);

#endif /* BSP_IAP_H */
