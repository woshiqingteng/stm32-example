/**
 * @file    exfuns.c
 * @brief   FatFs application glue, ported from the vendor exfuns middleware.
 *          Only the volume binding and free-space helpers are kept; the file
 *          copy / tester helpers are left out. A static FATFS pool replaces the
 *          vendor heap allocation.
 */

#include "exfuns.h"
#include <string.h>

FATFS *fs[FF_VOLUMES];

static FATFS g_fatfs_pool[FF_VOLUMES];

/* Extension table; the row/column index forms the T_* value. */
#define FILE_MAX_TYPE_NUM 7
#define FILE_MAX_SUBT_NUM 7

static const char *const g_file_type_tbl[FILE_MAX_TYPE_NUM][FILE_MAX_SUBT_NUM] =
{
    { "BIN" },
    { "LRC" },
    { "NES", "SMS" },
    { "TXT", "C", "H" },
    { "WAV", "MP3", "OGG", "FLAC", "AAC", "WMA", "MID" },
    { "BMP", "JPG", "JPEG", "GIF" },
    { "AVI" },
};

static uint8_t exfuns_char_upper(uint8_t c)
{
    if (c < 'A')
    {
        return c;
    }

    if (c >= 'a')
    {
        return (uint8_t)(c - 0x20U);
    }

    return c;
}

file_type_t exfuns_file_type(char *fname)
{
    uint8_t tbuf[5];
    char *attr = 0;
    uint8_t i = 0, j;

    while (i < 250U)
    {
        i++;

        if (*fname == '\0')
        {
            break;
        }

        fname++;
    }

    if (i == 250U)
    {
        return T_UNKNOWN;
    }

    for (i = 0; i < 5U; i++)
    {
        fname--;

        if (*fname == '.')
        {
            fname++;
            attr = fname;
            break;
        }
    }

    if (attr == 0)
    {
        return T_UNKNOWN;
    }

    memset(tbuf, 0, sizeof(tbuf));
    for (i = 0U; (i < (sizeof(tbuf) - 1U)) && (attr[i] != '\0'); i++)
    {
        tbuf[i] = attr[i];
    }

    for (i = 0; i < 4U; i++)
    {
        tbuf[i] = exfuns_char_upper(tbuf[i]);
    }

    for (i = 0; i < FILE_MAX_TYPE_NUM; i++)
    {
        for (j = 0; j < FILE_MAX_SUBT_NUM; j++)
        {
            if (g_file_type_tbl[i][j] == 0)
            {
                break;
            }

            if (strcmp((const char *)g_file_type_tbl[i][j], (const char *)tbuf) == 0)
            {
                return (file_type_t)((i << 4) | j);
            }
        }
    }

    return T_UNKNOWN;
}

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
