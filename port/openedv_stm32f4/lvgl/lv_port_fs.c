/**
 * @file    lv_port_fs.c
 * @brief   LVGL filesystem driver backed by FatFs (drive letter '0').
 *
 * Port of the upstream `lv_fs_fatfs.c` driver into the board port, so the
 * `lvgl` module stays free of any FatFs dependency. Call lv_port_fs_init()
 * after lv_init(); the FatFs volume must be mounted by the app first.
 */

#include <string.h>

#include "lvgl.h"
#include "ff.h"
#include "lv_port.h"

#define LV_FS_LETTER        '0'
#define LV_FS_CACHE_SIZE    0

static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br);
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p);
static void *fs_dir_open(lv_fs_drv_t *drv, const char *path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *dir_p, char *fn);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *dir_p);

void lv_port_fs_init(void)
{
    static lv_fs_drv_t fs_drv;

    lv_fs_drv_init(&fs_drv);
    fs_drv.letter     = LV_FS_LETTER;
    fs_drv.cache_size = LV_FS_CACHE_SIZE;

    fs_drv.open_cb      = fs_open;
    fs_drv.close_cb     = fs_close;
    fs_drv.read_cb      = fs_read;
    fs_drv.write_cb     = fs_write;
    fs_drv.seek_cb      = fs_seek;
    fs_drv.tell_cb      = fs_tell;
    fs_drv.dir_open_cb  = fs_dir_open;
    fs_drv.dir_read_cb  = fs_dir_read;
    fs_drv.dir_close_cb = fs_dir_close;

    lv_fs_drv_register(&fs_drv);
}

static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    uint8_t flags = 0;
    FIL *f;

    (void)drv;

    if (mode == LV_FS_MODE_WR)
    {
        flags = FA_WRITE | FA_OPEN_ALWAYS;
    }
    else if (mode == LV_FS_MODE_RD)
    {
        flags = FA_READ;
    }
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
    {
        flags = FA_READ | FA_WRITE | FA_OPEN_ALWAYS;
    }

    f = lv_mem_alloc(sizeof(FIL));
    if (f == NULL)
    {
        return NULL;
    }

    if (f_open(f, path, flags) == FR_OK)
    {
        return f;
    }

    lv_mem_free(f);
    return NULL;
}

static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p)
{
    (void)drv;
    f_close(file_p);
    lv_mem_free(file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    (void)drv;
    return (f_read(file_p, buf, btr, (UINT *)br) == FR_OK) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw)
{
    (void)drv;
    return (f_write(file_p, buf, btw, (UINT *)bw) == FR_OK) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
    (void)drv;

    switch (whence)
    {
    case LV_FS_SEEK_SET:
        f_lseek(file_p, pos);
        break;
    case LV_FS_SEEK_CUR:
        f_lseek(file_p, f_tell((FIL *)file_p) + pos);
        break;
    case LV_FS_SEEK_END:
        f_lseek(file_p, f_size((FIL *)file_p) + pos);
        break;
    default:
        break;
    }

    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    (void)drv;
    *pos_p = f_tell((FIL *)file_p);
    return LV_FS_RES_OK;
}

static void *fs_dir_open(lv_fs_drv_t *drv, const char *path)
{
    DIR *d;

    (void)drv;

    d = lv_mem_alloc(sizeof(DIR));
    if (d == NULL)
    {
        return NULL;
    }

    if (f_opendir(d, path) != FR_OK)
    {
        lv_mem_free(d);
        return NULL;
    }

    return d;
}

static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *dir_p, char *fn)
{
    FRESULT res;
    FILINFO fno;

    (void)drv;

    fn[0] = '\0';

    do
    {
        res = f_readdir(dir_p, &fno);
        if (res != FR_OK)
        {
            return LV_FS_RES_UNKNOWN;
        }

        if (fno.fattrib & AM_DIR)
        {
            fn[0] = '/';
            strcpy(&fn[1], fno.fname);
        }
        else
        {
            strcpy(fn, fno.fname);
        }
    } while (strcmp(fn, "/.") == 0 || strcmp(fn, "/..") == 0);

    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *dir_p)
{
    (void)drv;
    f_closedir(dir_p);
    lv_mem_free(dir_p);
    return LV_FS_RES_OK;
}
