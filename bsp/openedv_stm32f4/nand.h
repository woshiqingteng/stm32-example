/**
 * @file    nand.h
 * @brief   NAND FLASH driver over FMC (bank3, 8-bit bus): device-independent
 *          ONFI command, page and ECC algorithms. Device geometry comes from
 *          the chip header (nand_mt29f4g08.h) and is filled into nand_dev by
 *          nand_init().
 */

#ifndef BSP_NAND_H
#define BSP_NAND_H

#include <stdint.h>

/* Upper bound for the largest page (main + spare). */
#define NAND_MAX_PAGE_SIZE_BYTE      4096
#define NAND_ECC_SECTOR_SIZE_BYTE    512

/* Timing delays, in loop iterations / microseconds as noted. */
#define NAND_TADL_DELAY_COUNT         30
#define NAND_TWHR_DELAY_COUNT         25
#define NAND_TRHW_DELAY_COUNT         35
#define NAND_TPROG_DELAY_US        200
#define NAND_TBERS_DELAY_MS        4

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
    uint16_t  spare_ecc_offset; /* spare-area offset of the ECC bytes */
    uint32_t  id;               /* device ID */
    uint16_t *lut;              /* logical -> physical block table */
    uint32_t  ecc_hard;         /* last hardware ECC value */
    uint32_t  ecc_hdbuf[NAND_MAX_PAGE_SIZE_BYTE / NAND_ECC_SECTOR_SIZE_BYTE]; /* computed ECC per sector */
    uint32_t  ecc_rdbuf[NAND_MAX_PAGE_SIZE_BYTE / NAND_ECC_SECTOR_SIZE_BYTE]; /* ECC read back from spare */
} nand_attriute;

extern nand_attriute nand_dev;

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
uint8_t  nand_copypage_withoutwrite(uint32_t source_pagenum, uint32_t dest_pagenum);
uint8_t  nand_copypage_withwrite(uint32_t source_pagenum, uint32_t dest_pagenum, uint16_t colnum, uint8_t *pbuffer, uint16_t numbyte_to_write);
uint8_t  nand_readspare(uint32_t pagenum, uint16_t colnum, uint8_t *pbuffer, uint16_t numbyte_to_read);
uint8_t  nand_writespare(uint32_t pagenum, uint16_t colnum, uint8_t *pbuffer, uint16_t numbyte_to_write);
uint8_t  nand_eraseblock(uint32_t blocknum);
void     nand_erasechip(void);

uint16_t nand_ecc_get_oe(uint8_t oe, uint32_t eccval);
uint8_t  nand_ecc_correction(uint8_t *data_buf, uint32_t eccrd, uint32_t ecccl);

#endif /* BSP_NAND_H */
