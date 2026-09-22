/**
 * @file    sram.h
 * @brief   External SRAM driver (FMC NOR/SRAM bank1, chip select NE3).
 */

#ifndef BSP_SRAM_H
#define BSP_SRAM_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/* Control pins. */
#define SRAM_WR_GPIO_PORT    GPIOD
#define SRAM_WR_GPIO_PIN     GPIO_PIN_5
#define SRAM_RD_GPIO_PORT    GPIOD
#define SRAM_RD_GPIO_PIN     GPIO_PIN_4
#define SRAM_CS_GPIO_PORT    GPIOG
#define SRAM_CS_GPIO_PIN     GPIO_PIN_10

/* FMC chip select used by the SRAM (FMC_NE1..4). */
#define SRAM_FMC_NEX         3

/* Base address of the FMC bank1 sub-region selected by SRAM_FMC_NEX. */
#define SRAM_BASE_ADDR       (0x60000000UL + (0x4000000UL * (SRAM_FMC_NEX - 1)))

extern SRAM_HandleTypeDef g_sram_handler;

/** @brief  Initialise the FMC NOR/SRAM controller for the external SRAM. */
void sram_init(void);

/** @brief  Write @p datalen bytes to the SRAM at byte offset @p addr. */
void sram_write(uint8_t *pbuf, uint32_t addr, uint32_t datalen);

/** @brief  Read @p datalen bytes from the SRAM at byte offset @p addr. */
void sram_read(uint8_t *pbuf, uint32_t addr, uint32_t datalen);

/** @brief  Write one byte (test helper). */
void sram_test_write(uint32_t addr, uint8_t data);

/** @brief  Read one byte (test helper). */
uint8_t sram_test_read(uint32_t addr);

/**
 * @brief  Walking pattern test over @p len bytes starting at @p addr.
 * @return Number of mismatching bytes (0 = pass).
 */
uint32_t sram_test(uint32_t addr, uint32_t len);

#endif /* BSP_SRAM_H */
