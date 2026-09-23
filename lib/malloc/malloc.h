/**
 * @file    malloc.h
 * @brief   ALIENTEK dynamic memory manager (ported from the vendor MALLOC
 *          middleware). Three banks are managed: internal SRAM, CCM RAM and the
 *          external SDRAM. The allocator is block based (64-byte blocks).
 */

#ifndef LIB_MALLOC_H
#define LIB_MALLOC_H

#include <stdbool.h>
#include <stdint.h>

#ifndef NULL
#define NULL 0
#endif

/* Selectable memory banks. */
#define SRAMIN   0  /* internal SRAM */
#define SRAMCCM  1  /* core coupled memory (CPU only, not reachable by DMA) */
#define SRAMEX   2  /* external SDRAM */

/* Invalid allocation offset / no free block. */
#define MEM_ALLOC_INVALID       0xFFFFFFFFU

/* Block index is uint32_t so the SDRAM bank can be covered. */
#define MT_TYPE  uint32_t

#define SRAMBANK 3  /* number of supported banks */

/* mem1: internal SRAM. */
#define MEM1_BLOCK_SIZE         64
#define MEM1_MAX_SIZE           (160 * 1024)
#define MEM1_ALLOC_TABLE_SIZE   (MEM1_MAX_SIZE / MEM1_BLOCK_SIZE)

/* mem2: CCM RAM. */
#define MEM2_BLOCK_SIZE         64
#define MEM2_MAX_SIZE           (60 * 1024)
#define MEM2_ALLOC_TABLE_SIZE   (MEM2_MAX_SIZE / MEM2_BLOCK_SIZE)

/* mem3: external SDRAM (the first 2000 KB are reserved for the LTDC frame
 * buffer, so the heap starts at 0xC01F4000). */
#define MEM3_BLOCK_SIZE         64
#define MEM3_MAX_SIZE           (28912 * 1024)
#define MEM3_ALLOC_TABLE_SIZE   (MEM3_MAX_SIZE / MEM3_BLOCK_SIZE)

#define MEM2_BASE_ADDR          0x10000000UL
#define MEM2_MAP_ADDR           0x1000F000UL
#define MEM3_BASE_ADDR          0xC01F4000UL
#define MEM3_MAP_ADDR           (MEM3_BASE_ADDR + MEM3_MAX_SIZE)

/** @brief  Result of my_mem_free(). */
typedef enum
{
    MEM_OK = 0,        /*!< block released */
    MEM_NOT_READY,     /*!< pool was not initialised (it is now) */
    MEM_INVALID,       /*!< offset outside the pool */
} mem_status_t;

/** @brief  Memory manager descriptor. */
struct _m_mallco_dev
{
    void (*init)(uint8_t);              /* initialise */
    uint16_t (*perused)(uint8_t);       /* usage in 0.1 % units */
    uint8_t  *membase[SRAMBANK];        /* pool base addresses */
    uint32_t *memmap[SRAMBANK];         /* blocks in use table */
    bool      memrdy[SRAMBANK];         /* pool initialised flag */
};

extern struct _m_mallco_dev mallco_dev;

void        my_mem_set(void *s, uint8_t c, uint32_t count);
void        my_mem_copy(void *des, void *src, uint32_t n);
void        my_mem_init(uint8_t memx);
uint32_t    my_mem_malloc(uint8_t memx, uint32_t size);
mem_status_t my_mem_free(uint8_t memx, uint32_t offset);
uint16_t    my_mem_perused(uint8_t memx);

void    *mymalloc(uint8_t memx, uint32_t size);
void     myfree(uint8_t memx, void *ptr);
void    *myrealloc(uint8_t memx, void *ptr, uint32_t size);

#endif /* LIB_MALLOC_H */
