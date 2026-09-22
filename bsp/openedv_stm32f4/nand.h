/**
 * @file    nand.h
 * @brief   NAND FLASH driver over FMC (bank3, 8-bit bus), ported from the
 *          vendor NAND example. Hardware ECC is disabled; raw page access is
 *          provided for the NAND experiment and the FatFs port.
 */

#ifndef BSP_NAND_H
#define BSP_NAND_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/* Ready/Busy pin. */
#define NAND_RB_GPIO_PORT    GPIOD
#define NAND_RB_GPIO_PIN     GPIO_PIN_6
#define NAND_RB              HAL_GPIO_ReadPin(NAND_RB_GPIO_PORT, NAND_RB_GPIO_PIN)

/* Upper bound for the largest page (main + spare). */
#define NAND_MAX_PAGE_SIZE      4096
#define NAND_ECC_SECTOR_SIZE    512

/* Timing delays, in loop iterations / microseconds as noted. */
#define NAND_TADL_DELAY         30
#define NAND_TWHR_DELAY         25
#define NAND_TRHW_DELAY         35
#define NAND_TPROG_DELAY        200
#define NAND_TBERS_DELAY        4

/* FMC NAND bank3 address window and register offsets. */
#define NAND_ADDRESS            0x80000000UL
#define NAND_CMD                (1UL << 16)
#define NAND_ADDR               (1UL << 17)

/* Command set. */
#define nand_readID             0x90
#define NAND_FEATURE            0xEF
#define NAND_RESET              0xFF
#define NAND_READSTA            0x70
#define NAND_AREA_A             0x00
#define NAND_AREA_TRUE1         0x30
#define NAND_WRITE0             0x80
#define NAND_WRITE_TURE1        0x10
#define NAND_ERASE0             0x60
#define NAND_ERASE1             0xD0
#define NAND_MOVEDATA_CMD0      0x00
#define NAND_MOVEDATA_CMD1      0x35
#define NAND_MOVEDATA_CMD2      0x85
#define NAND_MOVEDATA_CMD3      0x10

/* Status values. */
#define NSTA_READY              0x40
#define NSTA_ERROR              0x01
#define NSTA_TIMEOUT            0x02
#define NSTA_ECC1BITERR         0x03
#define NSTA_ECC2BITERR         0x04

/* Supported device IDs. */
#define MT29F4G08ABADA          0xDC909556UL
#define MT29F16G08ABABA         0x48002689UL
#define FSNS8B004G              0xDC00A262UL

/** @brief  NAND device geometry / state. */
typedef struct
{
    uint16_t  page_totalsize;   /* main + spare bytes per page */
    uint16_t  page_mainsize;    /* main area bytes per page */
    uint16_t  page_sparesize;   /* spare area bytes per page */
    uint8_t   block_pagenum;    /* pages per block */
    uint16_t  plane_blocknum;   /* blocks per plane */
    uint16_t  block_totalnum;   /* total blocks */
    uint16_t  good_blocknum;    /* good blocks found */
    uint16_t  valid_blocknum;   /* logical blocks in use */
    uint32_t  id;               /* device ID */
    uint16_t *lut;              /* logical -> physical block table */
} nand_attriute;

extern NAND_HandleTypeDef g_nand_handle;
extern nand_attriute      nand_dev;

uint8_t  nand_init(void);
uint8_t  nand_modeset(uint8_t mode);
uint32_t nand_readid(void);
uint8_t  nand_readstatus(void);
uint8_t  nand_wait_for_ready(void);
uint8_t  nand_reset(void);
uint8_t  nand_waitrb(volatile uint8_t rb);
void     nand_delay(volatile uint32_t i);
uint8_t  nand_readpage(uint32_t pagenum, uint16_t colnum, uint8_t *pbuffer, uint16_t numbyte_to_read);
uint8_t  nand_readpagecomp(uint32_t pagenum, uint16_t colnum, uint32_t cmpval, uint16_t numbyte_to_read, uint16_t *numbyte_equal);
uint8_t  nand_writepage(uint32_t pagenum, uint16_t colnum, uint8_t *pbuffer, uint16_t numbyte_to_write);
uint8_t  nand_write_pageconst(uint32_t pagenum, uint16_t colnum, uint32_t cval, uint16_t numbyte_to_write);
uint8_t  nand_readspare(uint32_t pagenum, uint16_t colnum, uint8_t *pbuffer, uint16_t numbyte_to_read);
uint8_t  nand_writespare(uint32_t pagenum, uint16_t colnum, uint8_t *pbuffer, uint16_t numbyte_to_write);
uint8_t  nand_eraseblock(uint32_t blocknum);
void     nand_erasechip(void);

#endif /* BSP_NAND_H */
