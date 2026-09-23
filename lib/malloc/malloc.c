/**
 * @file    malloc.c
 * @brief   ALIENTEK dynamic memory manager (ported from the vendor MALLOC
 *          middleware and adapted for GCC). The block state tables for the CCM
 *          and SDRAM banks live next to their pools instead of using absolute
 *          linker sections.
 */

#include "malloc.h"

/* Pool storage. mem1 is a plain internal SRAM array; mem2/mem3 are addressed
 * directly so they do not consume internal SRAM. */
static __attribute__((aligned(32))) uint8_t mem1base[MEM1_MAX_SIZE];

/* Block state tables. */
static MT_TYPE mem1mapbase[MEM1_ALLOC_TABLE_SIZE];

/* Bank geometry: table size and block size. */
static const uint32_t memtblsize[SRAMBANK] =
    {MEM1_ALLOC_TABLE_SIZE, MEM2_ALLOC_TABLE_SIZE, MEM3_ALLOC_TABLE_SIZE};
static const uint32_t memblksize[SRAMBANK] =
    {MEM1_BLOCK_SIZE, MEM2_BLOCK_SIZE, MEM3_BLOCK_SIZE};
static const uint32_t memsize[SRAMBANK] =
    {MEM1_MAX_SIZE, MEM2_MAX_SIZE, MEM3_MAX_SIZE};

/* Memory manager instance. The cast expressions are compile-time constants so
 * the CCM/SDRAM pool and table addresses can be initialised statically. */
struct _m_mallco_dev mallco_dev =
{
    my_mem_init,
    my_mem_perused,
    {mem1base, (uint8_t *)MEM2_BASE_ADDR, (uint8_t *)MEM3_BASE_ADDR},
    {mem1mapbase, (uint32_t *)MEM2_MAP_ADDR,
     (uint32_t *)(MEM3_BASE_ADDR + MEM3_MAX_SIZE)},
    {false, false, false},
};

void my_mem_copy(void *des, void *src, uint32_t n)
{
    uint8_t *xdes = des;
    uint8_t *xsrc = src;

    while (n-- != 0U)
    {
        *xdes++ = *xsrc++;
    }
}

void my_mem_set(void *s, uint8_t c, uint32_t count)
{
    uint8_t *xs = s;

    while (count-- != 0U)
    {
        *xs++ = c;
    }
}

void my_mem_init(uint8_t memx)
{
    my_mem_set(mallco_dev.memmap[memx], 0, memtblsize[memx] * 4U);
    mallco_dev.memrdy[memx] = true;
}

uint16_t my_mem_perused(uint8_t memx)
{
    uint32_t used = 0U;
    uint32_t i;

    for (i = 0U; i < memtblsize[memx]; i++)
    {
        if (mallco_dev.memmap[memx][i] != 0U)
        {
            used++;
        }
    }

    return (uint16_t)((used * 1000U) / memtblsize[memx]);
}

uint32_t my_mem_malloc(uint8_t memx, uint32_t size)
{
    signed long offset = 0;
    uint32_t nmemb;
    uint32_t cmemb = 0U;
    uint32_t i;

    if (!mallco_dev.memrdy[memx])
    {
        mallco_dev.init(memx);
    }

    if (size == 0U)
    {
        return MEM_ALLOC_INVALID;
    }

    nmemb = size / memblksize[memx];

    if ((size % memblksize[memx]) != 0U)
    {
        nmemb++;
    }

    for (offset = (signed long)memtblsize[memx] - 1; offset >= 0; offset--)
    {
        if (mallco_dev.memmap[memx][offset] == 0U)
        {
            cmemb++;
        }
        else
        {
            cmemb = 0U;
        }

        if (cmemb == nmemb)
        {
            for (i = 0U; i < nmemb; i++)
            {
                mallco_dev.memmap[memx][offset + i] = nmemb;
            }

            return (uint32_t)offset * memblksize[memx];
        }
    }

    return MEM_ALLOC_INVALID;
}

mem_status_t my_mem_free(uint8_t memx, uint32_t offset)
{
    int i;

    if (!mallco_dev.memrdy[memx])
    {
        mallco_dev.init(memx);
        return MEM_NOT_READY;
    }

    if (offset < memsize[memx])
    {
        int index = (int)(offset / memblksize[memx]);
        int nmemb = (int)mallco_dev.memmap[memx][index];

        for (i = 0; i < nmemb; i++)
        {
            mallco_dev.memmap[memx][index + i] = 0U;
        }

        return MEM_OK;
    }

    return MEM_INVALID;
}

void myfree(uint8_t memx, void *ptr)
{
    uint32_t offset;

    if (ptr == NULL)
    {
        return;
    }

    offset = (uint32_t)ptr - (uint32_t)mallco_dev.membase[memx];
    (void)my_mem_free(memx, offset);
}

void *mymalloc(uint8_t memx, uint32_t size)
{
    uint32_t offset;

    offset = my_mem_malloc(memx, size);

    if (offset == MEM_ALLOC_INVALID)
    {
        return NULL;
    }

    return (void *)((uint32_t)mallco_dev.membase[memx] + offset);
}

void *myrealloc(uint8_t memx, void *ptr, uint32_t size)
{
    uint32_t offset;

    offset = my_mem_malloc(memx, size);

    if (offset == MEM_ALLOC_INVALID)
    {
        return NULL;
    }

    my_mem_copy((void *)((uint32_t)mallco_dev.membase[memx] + offset), ptr, size);
    myfree(memx, ptr);

    return (void *)((uint32_t)mallco_dev.membase[memx] + offset);
}
