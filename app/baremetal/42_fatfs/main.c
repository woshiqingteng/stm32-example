/**
 * @file    main.c
 * @brief   42_fatfs: FatFs over three volumes - SD ("0:"), SPI NOR ("1:") and
 *          NAND ("2:"). Each is mounted (formatted when it carries no
 *          filesystem), its root listed, a text file written/read back and the
 *          free space reported on USART1. The NAND volume is reached through
 *          the FTL (lib_nand_storage).
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"
#include "nand_storage.h"

#define BLINK_PERIOD_MS 500U

static const char *const g_drv[3]  = { "0:", "1:", "2:" };
static const char *const g_name[3] = { "SD", "NOR", "NAND" };

static FIL  g_file;
static char g_readbuf[64];
static BYTE g_work[FF_MAX_SS];

static void fatfs_list_root(const char *drv)
{
    DIR     dir;
    FILINFO fno;
    FRESULT res;
    char    path[8];

    (void)sprintf(path, "%s/", drv);
    res = f_opendir(&dir, path);

    if (res != FR_OK)
    {
        printf("  opendir failed (%d)\r\n", (int)res);
        return;
    }

    printf("  root of %s:\r\n", drv);

    for (;;)
    {
        res = f_readdir(&dir, &fno);

        if ((res != FR_OK) || (fno.fname[0] == 0))
        {
            break;
        }

        printf("    %s\r\n", fno.fname);
    }

    (void)f_closedir(&dir);
}

static void fatfs_test(uint8_t idx)
{
    const char *drv = g_drv[idx];
    char        path[16];
    FRESULT     res;
    UINT        bw = 0U;
    UINT        br = 0U;
    uint32_t    total = 0U;
    uint32_t    free_kb = 0U;

    res = f_mount(fs[idx], drv, 1);

    if (res == FR_NO_FILESYSTEM)
    {
        printf("no filesystem on %s (%s), formatting\r\n", drv, g_name[idx]);
        res = f_mkfs(drv, 0, g_work, sizeof(g_work));

        if (res == FR_OK)
        {
            res = f_mount(fs[idx], drv, 1);
        }
    }

    if (res != FR_OK)
    {
        printf("%s (%s) mount failed (%d)\r\n", drv, g_name[idx], (int)res);
        return;
    }

    printf("%s (%s) OK\r\n", drv, g_name[idx]);
    fatfs_list_root(drv);

    (void)sprintf(path, "%s/ALIENTEK.TXT", drv);
    res = f_open(&g_file, path, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);

    if (res == FR_OK)
    {
        const char *msg = "ALIENTEK FATFS TEST\r\n";

        (void)f_write(&g_file, msg, (UINT)strlen(msg), &bw);
        (void)f_sync(&g_file);
        (void)f_lseek(&g_file, 0);

        memset(g_readbuf, 0, sizeof(g_readbuf));
        (void)f_read(&g_file, g_readbuf, sizeof(g_readbuf) - 1U, &br);
        (void)f_close(&g_file);

        printf("  file wrote %u, read %u: %s", (unsigned)bw, (unsigned)br, g_readbuf);
    }
    else
    {
        printf("  open failed (%d)\r\n", (int)res);
    }

    if (exfuns_get_free((uint8_t *)drv, &total, &free_kb) == 0U)
    {
        printf("  %s total %lu MB free %lu MB\r\n",
               g_name[idx], (unsigned long)(total >> 10), (unsigned long)(free_kb >> 10));
    }
}

int main(void)
{
    uint8_t i;

    bsp_init();

    nand_storage_activate();

    printf("42_fatfs ready (0:SD 1:NOR 2:NAND)\r\n");

    (void)exfuns_init();

    for (i = 0U; i < 3U; i++)
    {
        fatfs_test(i);
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
