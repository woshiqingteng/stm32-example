/**
 * @file    lv_file.h
 * @brief   File manager.
 */

#ifndef LV_FILE_H
#define LV_FILE_H
#include "lvgl.h"
#include "exfuns.h"

#define LIST_SIZE    100   /* max list rows / stored paths */
#define FILE_SEZE    1992  /* text read buffer size */

typedef struct
{
    lv_obj_t * list;          /* list widget */
    lv_obj_t *lv_page_obj;    /* paging main object */
    lv_obj_t * list_btn[LIST_SIZE]; /* list buttons */
    uint8_t list_flie_nuber;  /* number of list rows/files */
    FRESULT fr;               /* FatFs result */
    DIR lv_dir;               /* directory handle */
    FILINFO SD_fno;           /* file info */
    char *pname;              /* path + file name */
    char *lv_pname;           /* file name */
    char *lv_pname_shift;     /* file name scratch */
    const char* lv_pash;      /* current path */
    int lv_suffix_flag;       /* suffix flag */
    int lv_prev_file_flag;    /* previous-file path flag */
    char *lv_prev_file[LIST_SIZE];  /* stored file paths */
    const void *image_scr;    /* image displayed with the file */
    lv_obj_t * lv_back_obj;   /* back/menu button object */
    lv_obj_t * lv_prev_btn;   /* back button */
    lv_obj_t * lv_back_btn;   /* menu button */
    lv_obj_t *lv_page_cont;   /* text content area */
    char rbuf[FILE_SEZE];     /* text read buffer */
    lv_obj_t *lv_return_page; /* returned page */
    lv_obj_t *lv_image_read;  /* read image object */
    lv_obj_t *lv_file_cont;
}lv_file_struct;

/* Assert helper: hangs when term is 0 (used for the file manager). */
#define FILE_ASSERT(term)                                                                                   \
do                                                                                                          \
{                                                                                                           \
    if (!(term))                                                                                            \
    {                                                                                                       \
        printf("Assert failed. Condition(%s). [%s][%d]\r\n", term, __FUNCTION__, __LINE__);                 \
        while(1)                                                                                            \
        {                                                                                                   \
            ;                                                                                               \
        }                                                                                                   \
    }                                                                                                       \
} while (0)

void lv_file_demo(void);

#endif
