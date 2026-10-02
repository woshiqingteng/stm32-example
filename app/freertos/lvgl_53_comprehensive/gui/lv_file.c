/**
 * @file    lv_file.c
 * @brief   File manager.
 */

#include "lv_file.h"
#include "lv_mainstart.h"
#include "exfuns.h"
#include "malloc.h"
#include "lcd.h"
#include "usart.h"
#include "lvgl.h"
#include <stdio.h>


lv_file_struct lv_flie;
/* known suffixes; anything else is treated as unknown */
char *lv_suffix [] ={".txt",".avi",".png","jpeg",".jpg",".bin",".gif",".bmp",".FON",".dat",".sif",".BIN",".xbf",".ttf"};
#define LV_SUFFIX(x)    (int)(sizeof(x)/sizeof(x[0])) /* number of suffixes */

uint16_t lv_scan_files (const char* path,lv_obj_t *parent);
void lv_del_list(lv_obj_t *parent);
void lv_create_list(lv_obj_t *parent);
void lv_mainstart(void);
void list_init(lv_obj_t *parent);
lv_obj_t * lv_create_page(lv_obj_t *parent);
char * lv_pash_joint(void);

/* Report the current file position. */
long lv_tell(lv_fs_file_t *fd)
{
    uint32_t pos = 0;
    lv_fs_tell(fd, &pos);
    printf("\nlv_tcur pos is: %d\n", pos);
    return pos;
}

/* Read a file into lv_flie.rbuf. */
lv_fs_res_t lv_file_read(const char *path)
{
    uint32_t rsize = 0;
    lv_fs_file_t fd;
    lv_fs_res_t res;

    res = lv_fs_open(&fd, path, LV_FS_MODE_RD);
    
    if (res != LV_FS_RES_OK)
    {
        printf("open %s ERROR\n",path);
        return LV_FS_RES_UNKNOWN;
    }

    lv_tell(&fd);
    lv_fs_seek(&fd,0,LV_FS_SEEK_SET);
    lv_tell(&fd);
    res = lv_fs_read(&fd, lv_flie.rbuf, FILE_SEZE, &rsize);

    if (res != LV_FS_RES_OK)
    {
        printf("read %s ERROR\n",path);
        return LV_FS_RES_UNKNOWN;
    }

    lv_tell(&fd);
    
    lv_fs_close(&fd);
    
    return LV_FS_RES_OK;
}

/* Page "Return" button callback. */
void lv_btn_close_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);

    if (code == LV_EVENT_RELEASED)
    {
        if (lv_flie.lv_image_read != NULL)     /* image still open ? */
        {
            lv_obj_del(lv_flie.lv_image_read); /* delete the image */
            lv_flie.lv_image_read = NULL;
        }
        
        lv_flie.lv_prev_file_flag -- ;         /* pop one path */
        
        lv_obj_del(lv_flie.lv_page_cont);      /* delete the page */
    }
}

/* Create a content page with a top bar and Return button. */
lv_obj_t * lv_create_page(lv_obj_t *parent)
{
    lv_flie.lv_page_cont = lv_obj_create(parent);    /* page container */
    lv_obj_set_size(lv_flie.lv_page_cont, lcd_info()->width, lcd_info()->height);
    lv_obj_set_style_radius(lv_flie.lv_page_cont,0,LV_STATE_DEFAULT);/* radius 0 */
    lv_obj_clear_flag(lv_flie.lv_page_cont,LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_clear_flag(lv_flie.lv_page_cont,LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_align_to(lv_flie.lv_page_cont,parent,LV_ALIGN_CENTER,0,0);
  
    lv_obj_t *lv_page_obj = lv_obj_create(lv_flie.lv_page_cont); /* bottom bar */
    lv_obj_set_style_bg_color(lv_page_obj,lv_palette_main(LV_PALETTE_BLUE),LV_STATE_DEFAULT);
    lv_obj_align(lv_page_obj,LV_ALIGN_BOTTOM_MID,0,10);
    lv_obj_set_size(lv_page_obj, lcd_info()->width, lv_font_montserrat_14.line_height);

    lv_obj_t * lv_page_back_btn = lv_label_create(lv_page_obj);  /* Return label */
    lv_obj_set_style_text_font(lv_page_back_btn,&lv_font_montserrat_14,LV_STATE_DEFAULT);
    lv_label_set_text(lv_page_back_btn,"Return");
    lv_obj_align_to(lv_page_back_btn,NULL,LV_ALIGN_CENTER,0,0);
    lv_obj_add_flag(lv_page_back_btn,LV_OBJ_FLAG_CLICKABLE); /* clickable */
    lv_obj_add_event_cb(lv_page_back_btn,lv_btn_close_event,LV_EVENT_ALL,NULL); /* callback */

    return lv_flie.lv_page_cont;
}

/* Show a .txt file. */
void lv_show_filetxt(lv_obj_t *parent)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(label,&lv_font_montserrat_14,LV_STATE_DEFAULT);
    lv_obj_set_width(label, lv_obj_get_width(parent));
    lv_label_set_text(label, (char *)lv_flie.rbuf); /* show the content */
    memset(lv_flie.rbuf,0,sizeof(lv_flie.rbuf));
}

/* Show a .bin image. */
void lv_show_imgbin(lv_obj_t *parent,const char *path)
{
    lv_flie.lv_image_read = lv_img_create(parent);                     /* image widget */   
    lv_img_set_src(lv_flie.lv_image_read,path);                        /* set source */
//    lv_img_set_auto_size(lv_flie.lv_image_read, true);                 /* auto size */
    lv_obj_align_to(lv_flie.lv_image_read,parent,LV_ALIGN_CENTER,0,0); /* centered */
}

/* Build the full path from the current directory and file name. */
char * lv_pash_joint(void)
{
    lv_flie.lv_prev_file[lv_flie.lv_prev_file_flag] = (char *)lv_flie.lv_pash;/* push the path */
    lv_flie.lv_prev_file_flag ++;                              /* flag + 1 */
  
    strcpy((char *)lv_flie.pname, lv_flie.lv_pash);            /* directory */
    strcat((char *)lv_flie.pname, "/");                        /* separator */
    strcat((char *)lv_flie.pname, (char *)lv_flie.lv_pname);   /* file name */
    return lv_flie.pname;
}

/* List entry click callback. */
static void lv_list_btn_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);
  
    if(code == LV_EVENT_CLICKED)
    {
        for (int i = 0; i <= lv_flie.list_flie_nuber ;i++)  /* scan the list */
        {
            if (obj == lv_flie.list_btn[i]) /* the clicked entry */
            {
                lv_flie.lv_pname = mymalloc(SRAMIN, sizeof(lv_flie.lv_pname));         /* file name buffer */
                lv_flie.pname = mymalloc(SRAMIN, sizeof(lv_flie.pname));               /* path buffer */
                lv_flie.lv_pname = (char *)lv_list_get_btn_text(lv_flie.list,lv_flie.list_btn[i]);  /* entry text */
              
                for (int suffix = 0;suffix < LV_SUFFIX(lv_suffix) ;suffix ++)     /* known suffix ? */
                {
                    if (strstr(lv_flie.lv_pname,lv_suffix[suffix]) != NULL)
                    {
                        lv_flie.lv_suffix_flag = 0;
                        break;
                    }
                }

                if (lv_flie.lv_suffix_flag == 1)                                  /* unknown -> directory */
                {   
                    lv_flie.lv_pash = lv_pash_joint();                            /* build the path */
                    lv_del_list(lv_flie.list);                                    /* delete the list */
                    lv_scan_files(lv_flie.pname,lv_scr_act());                    /* rescan */
                }
                else
                {
                    if (strstr(lv_flie.lv_pname,".txt") != NULL)                  /* .txt file */
                    {
                        if (lv_file_read(lv_pash_joint()) == LV_FS_RES_OK)        /* read ok ? */
                        {
                            lv_flie.lv_return_page = lv_create_page(lv_scr_act());/* create page */
                            lv_show_filetxt(lv_flie.lv_return_page);              /* show text */
                        }
                    }
                    else if (strstr(lv_flie.lv_pname,".bin") != NULL)             /* .bin file */
                    {
                        lv_flie.lv_return_page = lv_create_page(lv_scr_act());    /* create page */
                        lv_show_imgbin(lv_flie.lv_return_page,lv_pash_joint());   /* show image */
                    }

                    lv_flie.lv_suffix_flag = 1; /* restore */
                }
            }
        }
    }
}

/* Scan a directory and build the list. */
uint16_t lv_scan_files (const char* path,lv_obj_t *parent)
{
    lv_flie.fr = f_opendir(&lv_flie.lv_dir, path);         /* open the directory */
    memset(lv_flie.list_btn,0,sizeof(lv_flie.list_btn));   /* clear the list */
    lv_flie.list_flie_nuber = 0;                           /* file count = 0 */
    lv_create_list(parent);                                /* create the list */

    if (lv_flie.pname != NULL||lv_flie.lv_pname != NULL)
    {
        myfree(SRAMIN, lv_flie.pname);                    /* free pname */
        myfree(SRAMIN, lv_flie.lv_pname);                 /* free lv_pname */
    }

    if (lv_flie.fr == FR_OK)
    {   /* read entries */
        while(1)
        {   /* loop over the directory */
            lv_flie.fr = f_readdir(&lv_flie.lv_dir, &lv_flie.SD_fno);        /* read entry */

            if (lv_flie.fr != LV_FS_RES_OK||lv_flie.SD_fno.fname[0] == 0) break;  /* done */
            lv_flie.list_flie_nuber ++;     /* count + 1 */
           
          if (lv_flie.SD_fno.fattrib& AM_DIR) /* directory */
            {
               
                /* add the directory entry */
                lv_flie.list_btn[lv_flie.list_flie_nuber] = lv_list_add_btn(lv_flie.list, LV_SYMBOL_DIRECTORY, lv_flie.SD_fno.fname);
                lv_obj_set_style_img_recolor(lv_flie.list_btn[lv_flie.list_flie_nuber],lv_color_hex(0xFFD700),LV_STATE_DEFAULT);  /* icon color */
            }
            else /* file */
            {
               
                if (strstr(lv_flie.SD_fno.fname,".png") != NULL    /* pick the icon by type */
                    ||strstr(lv_flie.SD_fno.fname,".jpeg") != NULL
                    ||strstr(lv_flie.SD_fno.fname,".jpg") != NULL
                    ||strstr(lv_flie.SD_fno.fname,".bmp") != NULL
                    ||strstr(lv_flie.SD_fno.fname,".gif") != NULL
                    ||strstr(lv_flie.SD_fno.fname,".avi") != NULL)
                {
                    lv_flie.image_scr = LV_SYMBOL_IMAGE;
                }
                else
                {
                    lv_flie.image_scr = LV_SYMBOL_FILE;
                }
                
                lv_flie.list_btn[lv_flie.list_flie_nuber] = lv_list_add_btn(lv_flie.list, lv_flie.image_scr, lv_flie.SD_fno.fname);
                lv_obj_set_style_img_recolor(lv_flie.list_btn[lv_flie.list_flie_nuber],lv_color_hex(0x87CEFA),LV_STATE_DEFAULT);   /* icon color */
            }
            
            lv_obj_set_style_pad_left(lv_flie.list_btn[lv_flie.list_flie_nuber],5,LV_STATE_DEFAULT);   /* left padding */
            lv_obj_set_style_pad_right(lv_flie.list_btn[lv_flie.list_flie_nuber],5,LV_STATE_DEFAULT);  /* right padding */
            lv_obj_add_event_cb(lv_flie.list_btn[lv_flie.list_flie_nuber], lv_list_btn_event,LV_EVENT_ALL,NULL); /* callback */

        }
        
        f_closedir(&lv_flie.lv_dir); /* close the directory */
    }
    
    return lv_flie.fr;                   /* result */
}

/* Delete the list. */
void lv_del_list(lv_obj_t *parent)
{
    lv_obj_del(parent);   /* delete */
    lv_flie.list = NULL;  /* clear */
}

/* Slide the list in from the right. */
void lv_animation(lv_obj_t *parent)
{
    lv_anim_t a;                                                /* animation */
    lv_anim_init(&a);                                           /* init */
    lv_anim_set_var(&a, parent);                                /* target */
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);  /* callback */
    int32_t start = lv_obj_get_width(lv_scr_act());
    int32_t end = 0;
    lv_anim_set_values(&a, start, end);                         /* from right to 0 */
    lv_anim_set_time(&a, 300);                                  /* 300 ms */
    lv_anim_start(&a);                                          /* start */
}

/* Create the list. */
void lv_create_list(lv_obj_t *parent)
{
    lv_flie.list = lv_list_create(parent);  /* list widget */
    lv_animation(lv_flie.list);
    lv_obj_set_size(lv_flie.list, lcd_info()->width, lcd_info()->height - lv_font_montserrat_14.line_height*2 - 60);  /* size */
    lv_obj_align_to(lv_flie.list,lv_flie.lv_page_obj,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);            /* align */
    lv_obj_set_style_text_font(lv_flie.list,&lv_font_montserrat_14,LV_STATE_DEFAULT);          /* font */
    lv_obj_set_style_radius(lv_flie.list,0,LV_STATE_DEFAULT);/* radius 0 */
}

/* Back-button callback. */
void lv_file_back_btn_event_handler(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);
  
    if(code == LV_EVENT_PRESSED)
    {
        lv_label_set_text(obj,"#444444 "BACK_BTN_TITLE"#");
        lv_label_set_recolor(obj,true);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        lv_obj_del(lv_general_dev.parent);
        lv_obj_del(lv_flie.list);
        lv_obj_del(lv_flie.lv_back_obj);
        lv_mainstart();/* reopen the launcher */
    }
}

/* Create the page title bar. */
void lv_page_tile(lv_obj_t *parent)
{
    lv_general_dev.lv_general_win_create = NULL;
    lv_general_dev.lv_back_event = NULL;
    lv_general_dev.parent = NULL;
    
    lv_flie.lv_page_obj = lv_obj_create(parent);
    lv_obj_set_size(lv_flie.lv_page_obj,lcd_info()->width,lv_font_montserrat_14.line_height + 50);
    lv_obj_set_style_bg_color(lv_flie.lv_page_obj,lv_palette_main(LV_PALETTE_GREY),LV_STATE_DEFAULT);
    lv_obj_set_style_radius(lv_flie.lv_page_obj,0,LV_STATE_DEFAULT);/* radius 0 */
    lv_obj_align_to(lv_flie.lv_page_obj,parent,LV_ALIGN_TOP_LEFT,0,0);
    
    lv_general_dev.lv_back_event = lv_file_back_btn_event_handler;
    lv_general_dev.parent = lv_flie.lv_page_obj;
    lv_general_dev.lv_general_win_create = lv_general_win_create;
    lv_general_dev.lv_general_win_create();
    
    lv_obj_t *lv_page_label = lv_label_create(lv_flie.lv_page_obj);
    lv_label_set_text(lv_page_label,"File Management System");
    lv_obj_set_style_text_color(lv_page_label,lv_palette_main(LV_PALETTE_RED),LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lv_page_label,&lv_font_montserrat_14,LV_STATE_DEFAULT);
    lv_obj_align_to(lv_page_label,lv_flie.lv_page_obj,LV_ALIGN_CENTER,0,0);
}

/* Menu / Return button callback. */
void lv_back_btn_event_handler(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);
  
    if(code == LV_EVENT_SHORT_CLICKED)
    {
        if (obj == lv_flie.lv_back_btn)
        {
            lv_del_list(lv_flie.list);         /* delete the list */
            list_init(lv_scr_act());           /* back to the menu */
        }
        if (obj == lv_flie.lv_prev_btn)
        { 
            lv_flie.lv_prev_file_flag -- ;     /* pop one path */

            if (lv_flie.lv_prev_file_flag <= 0)
            {
                lv_flie.lv_prev_file_flag = 0; /* clamp at 0 */
            }
          
            lv_del_list(lv_flie.list);         /* delete the list */
            lv_flie.lv_pash = lv_flie.lv_prev_file[lv_flie.lv_prev_file_flag]; /* parent path */
            lv_scan_files(lv_flie.lv_prev_file[lv_flie.lv_prev_file_flag],lv_scr_act());/* rescan */
           
        }
    }
}

/* Create the Menu / Return buttons. */
void lv_fs_win_create(lv_obj_t *parent)
{
    lv_flie.lv_back_btn = lv_label_create(parent);
    lv_obj_set_style_text_font(lv_flie.lv_back_btn,&lv_font_montserrat_14,LV_STATE_DEFAULT); /* font */
    
    lv_label_set_text(lv_flie.lv_back_btn,"Menu");
    lv_obj_align_to(lv_flie.lv_back_btn,parent,LV_ALIGN_RIGHT_MID,-10,0);
    lv_obj_add_flag(lv_flie.lv_back_btn,LV_OBJ_FLAG_CLICKABLE); /* clickable */
    lv_obj_add_event_cb(lv_flie.lv_back_btn,lv_back_btn_event_handler,LV_EVENT_ALL,NULL); /* callback */
    lv_flie.lv_prev_btn = lv_label_create(parent);
    lv_obj_set_style_text_font(lv_flie.lv_prev_btn,&lv_font_montserrat_14,LV_STATE_DEFAULT);
    lv_label_set_text(lv_flie.lv_prev_btn,"Return");
    lv_obj_align_to(lv_flie.lv_prev_btn,parent,LV_ALIGN_LEFT_MID,10,0);
    lv_obj_add_flag(lv_flie.lv_prev_btn,LV_OBJ_FLAG_CLICKABLE); /* clickable */
    lv_obj_add_event_cb(lv_flie.lv_prev_btn,lv_back_btn_event_handler,LV_EVENT_ALL,NULL);
}

/* Bottom bar of the file manager (Menu / Return). */
void lv_page_back(lv_obj_t *parent)
{
    lv_flie.lv_back_obj = lv_obj_create(parent);                                                            /* container */
    lv_obj_set_size(lv_flie.lv_back_obj,lcd_info()->width,lv_font_montserrat_14.line_height+10);                              /* size */
    lv_obj_set_style_bg_color(lv_flie.lv_back_obj,lv_palette_main(LV_PALETTE_GREY),LV_STATE_DEFAULT);       /* grey */
    lv_obj_set_style_radius(lv_flie.lv_back_obj,0,LV_STATE_DEFAULT);                                        /* radius 0 */
    lv_obj_align_to(lv_flie.lv_back_obj,parent,LV_ALIGN_BOTTOM_MID,0,0);                                    /* align */
    lv_fs_win_create(lv_flie.lv_back_obj);                                                                  /* buttons */
}

/* Init the list. */
void list_init(lv_obj_t *parent)
{
    lv_flie.lv_pash = "0:";                                 /* start path */
    lv_flie.lv_prev_file_flag = 0;                          /* path flag */
    lv_flie.lv_prev_file[lv_flie.lv_prev_file_flag] = "0:"; /* initial path */
    lv_flie.list_flie_nuber = 0;                            /* file count */
    lv_flie.lv_suffix_flag = 1;                             /* suffix flag */
    lv_scan_files(lv_flie.lv_pash,parent);                  /* scan */
}

/* Demo entry. */
void lv_file_demo(void)
{
    lv_page_tile(lv_scr_act());         /* title bar */
    lv_page_back(lv_scr_act());         /* bottom bar */
    list_init(lv_scr_act());            /* init the list */
}
