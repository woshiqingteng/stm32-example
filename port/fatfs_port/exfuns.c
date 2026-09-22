/**
 * @file    exfuns.c
 * @brief   FatFs application glue, ported from the vendor exfuns middleware.
 *          Only the volume binding and free-space helpers are kept; the file
 *          copy / tester helpers are left out. A static FATFS pool replaces the
 *          vendor heap allocation.
 */

#include "exfuns.h"

FATFS *fs[FF_VOLUMES];

static FATFS g_fatfs_pool[FF_VOLUMES];

uint8_t exfuns_init(void)
{
    uint8_t i;

    for (i = 0U; i < FF_VOLUMES; i++)
    {
        fs[i] = &g_fatfs_pool[i];
    }

    return 0U;
}

uint8_t exfuns_get_free(uint8_t *pdrv, uint32_t *total, uint32_t *free)
{
    FATFS   *fs1;
    uint8_t  res;
    uint32_t fre_clust = 0U;
    uint32_t fre_sect  = 0U;
    uint32_t tot_sect  = 0U;

    res = (uint8_t)f_getfree((const TCHAR *)pdrv, (DWORD *)&fre_clust, &fs1);

    if (res == 0U)
    {
        tot_sect = (fs1->n_fatent - 2U) * fs1->csize;
        fre_sect = fre_clust * fs1->csize;

#if FF_MAX_SS != 512
        tot_sect *= fs1->ssize / 512U;
        fre_sect *= fs1->ssize / 512U;
#endif

        *total = tot_sect >> 1;
        *free  = fre_sect >> 1;
    }

    return res;
}
