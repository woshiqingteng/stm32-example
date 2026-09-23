/**
 * @file    main.c
 * @brief   42_fatfs: FatFs over the SD card. The volume is mounted (formatted
 *          on the fly if it carries no filesystem), the root directory is
 *          listed, a text file is created, written, read back and reported on
 *          USART1.
 */

#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "ff.h"
#include "exfuns.h"

#define DRIVE           "0:"
#define TEST_FILE       "0:/ALIENTEK.TXT"
#define BLINK_PERIOD_MS 500U

static FIL  g_file;
static char g_readbuf[64];
static BYTE g_work[FF_MAX_SS];

static void fatfs_list_root(void)
{
    DIR     dir;
    FILINFO fno;
    FRESULT res;

    res = f_opendir(&dir, DRIVE "/");

    if (res != FR_OK)
    {
        printf("opendir failed (%d)\r\n", (int)res);
        return;
    }

    printf("root of %s:\r\n", DRIVE);

    for (;;)
    {
        res = f_readdir(&dir, &fno);

        if ((res != FR_OK) || (fno.fname[0] == 0))
        {
            break;
        }

        printf("  %s\r\n", fno.fname);
    }

    (void)f_closedir(&dir);
}

int main(void)
{
    FRESULT  res;
    UINT     bw = 0U;
    UINT     br = 0U;
    uint32_t total = 0U;
    uint32_t free_kb = 0U;
    char     line[64];

    bsp_init();

    printf("42_fatfs ready\r\n");

    if (sdio_init() != 0U)
    {
        printf("SD init failed\r\n");
    }
    else
    {
        (void)exfuns_init();

        res = f_mount(fs[0], DRIVE, 1);

        if (res == FR_NO_FILESYSTEM)
        {
            printf("no filesystem, formatting %s\r\n", DRIVE);
            res = f_mkfs(DRIVE, 0, g_work, sizeof(g_work));

            if (res == FR_OK)
            {
                res = f_mount(fs[0], DRIVE, 1);
            }
        }

        if (res != FR_OK)
        {
            sprintf(line, "mount failed (%d)", (int)res);
            printf("%s\r\n", line);
        }
        else
        {
            printf("FATFS OK\r\n");
            fatfs_list_root();

            res = f_open(&g_file, TEST_FILE, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);

            if (res == FR_OK)
            {
                const char *msg = "ALIENTEK FATFS TEST\r\n";

                (void)f_write(&g_file, msg, (UINT)strlen(msg), &bw);
                (void)f_sync(&g_file);
                (void)f_lseek(&g_file, 0);

                memset(g_readbuf, 0, sizeof(g_readbuf));
                (void)f_read(&g_file, g_readbuf, sizeof(g_readbuf) - 1U, &br);
                (void)f_close(&g_file);

                printf("file wrote %u, read %u: %s", (unsigned)bw, (unsigned)br, g_readbuf);
            }
            else
            {
                sprintf(line, "open failed (%d)", (int)res);
                printf("%s\r\n", line);
            }

            if (exfuns_get_free((uint8_t *)DRIVE, &total, &free_kb) == 0U)
            {
                sprintf(line, "SD %lu MB free %lu MB",
                        (unsigned long)(total >> 10), (unsigned long)(free_kb >> 10));
                printf("%s\r\n", line);
            }
        }
    }

    for (;;)
    {
        led_toggle(LED0);
        delay_ms(BLINK_PERIOD_MS);
    }
}
