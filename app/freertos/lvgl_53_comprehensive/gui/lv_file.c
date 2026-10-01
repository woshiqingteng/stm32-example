/**
 ****************************************************************************************************
 * @file        lv_mainstart.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-03-23
 * @brief       �ļ�����ϵͳ
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� ������ F429������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 ****************************************************************************************************
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
/* �ļ��ĺ�׺���������������������δ֪�ĺ�׺ */
char *lv_suffix [] ={".txt",".avi",".png","jpeg",".jpg",".bin",".gif",".bmp",".FON",".dat",".sif",".BIN",".xbf",".ttf"};
#define LV_SUFFIX(x)    (int)(sizeof(x)/sizeof(x[0])) /* ����lv_suffix����Ĵ�С */

uint16_t lv_scan_files (const char* path,lv_obj_t *parent);
void lv_del_list(lv_obj_t *parent);
void lv_create_list(lv_obj_t *parent);
void lv_mainstart(void);
void list_init(lv_obj_t *parent);
lv_obj_t * lv_create_page(lv_obj_t *parent);
char * lv_pash_joint(void);

/**
 * @brief  �����ļ���λ��
 * @param  fd���ļ�ָ��
 * @return ����λ��
 */
long lv_tell(lv_fs_file_t *fd)
{
    uint32_t pos = 0;
    lv_fs_tell(fd, &pos);
    printf("\nlv_tcur pos is: %d\n", pos);
    return pos;
}

/**
 * @brief  ��ȡ�ļ�����
 * @param  path���ļ�·��
 * @return LV_FS_RES_OK����ȡ�ɹ�
 */
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

/**
  * @brief  ҳ�淵�ذ����ص�����
  * @param  obj  : ����
  * @param  event: �¼�
  * @retval ��
  */
void lv_btn_close_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);

    if (code == LV_EVENT_RELEASED)
    {
        if (lv_flie.lv_image_read != NULL)     /* �ж�ͼƬ·���ͷ�Ϊ�� */
        {
            lv_obj_del(lv_flie.lv_image_read); /* ɾ��ͼƬ���� */
            lv_flie.lv_image_read = NULL;      /* ����ͼƬ����Ϊ�� */
        }
        
        lv_flie.lv_prev_file_flag -- ;         /* �ļ����ر�־λ��һ */
        
        lv_obj_del(lv_flie.lv_page_cont);      /* �����·���ʱ��ɾ��ҳ�� */
    }
}

/**
  * @brief  ����pageҳ��
  * @param  parent:����
  * @retval ��
  */
lv_obj_t * lv_create_page(lv_obj_t *parent)
{
    lv_flie.lv_page_cont = lv_obj_create(parent);    /* �������� */
    lv_obj_set_size(lv_flie.lv_page_cont, lcd_info()->width, lcd_info()->height);
    lv_obj_set_style_radius(lv_flie.lv_page_cont,0,LV_STATE_DEFAULT);/* ����Բ�뾶Ϊ0 */
    lv_obj_clear_flag(lv_flie.lv_page_cont,LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_clear_flag(lv_flie.lv_page_cont,LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_align_to(lv_flie.lv_page_cont,parent,LV_ALIGN_CENTER,0,0);
  
    lv_obj_t *lv_page_obj = lv_obj_create(lv_flie.lv_page_cont); /* �������ذ��������� */
    lv_obj_set_style_bg_color(lv_page_obj,lv_palette_main(LV_PALETTE_BLUE),LV_STATE_DEFAULT);
    lv_obj_align(lv_page_obj,LV_ALIGN_BOTTOM_MID,0,10);
    lv_obj_set_size(lv_page_obj, lcd_info()->width, lv_font_montserrat_14.line_height);

    lv_obj_t * lv_page_back_btn = lv_label_create(lv_page_obj);  /* ����lable��Ϊ���صĶ��� */
    lv_obj_set_style_text_font(lv_page_back_btn,&lv_font_montserrat_14,LV_STATE_DEFAULT);
    lv_label_set_text(lv_page_back_btn,"Return");
    lv_obj_align_to(lv_page_back_btn,NULL,LV_ALIGN_CENTER,0,0);
    lv_obj_add_flag(lv_page_back_btn,LV_OBJ_FLAG_CLICKABLE); /* ���ñ�ǩ�ɵ�� */
    lv_obj_add_event_cb(lv_page_back_btn,lv_btn_close_event,LV_EVENT_ALL,NULL); /* ���ûص����� */

    return lv_flie.lv_page_cont;
}

/**
  * @brief  ��ʾ.txt�ļ�
  * @param  parent:����
  * @retval ��
  */
void lv_show_filetxt(lv_obj_t *parent)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(label,&lv_font_montserrat_14,LV_STATE_DEFAULT);
    lv_obj_set_width(label, lv_obj_get_width(parent));
    lv_label_set_text(label, (char *)lv_flie.rbuf); /* ��ʾ��ȡ������ */
    memset(lv_flie.rbuf,0,sizeof(lv_flie.rbuf));
}

/**
  * @brief  ��ʾ.binͼƬ
  * @param  parent:����
  * @param  path:·��
  * @retval ��
  */
void lv_show_imgbin(lv_obj_t *parent,const char *path)
{
    lv_flie.lv_image_read = lv_img_create(parent);                     /* ���� image �ؼ� */   
    lv_img_set_src(lv_flie.lv_image_read,path);                        /* Ϊ�ؼ�����ͼƬ */
//    lv_img_set_auto_size(lv_flie.lv_image_read, true);                 /* �Զ����ô�С */
    lv_obj_align_to(lv_flie.lv_image_read,parent,LV_ALIGN_CENTER,0,0); /* ���ÿؼ��Ķ��뷽ʽ,������� */
}

/**
  * @brief  �ļ�·��ƴ��
  * @param  ��
  * @retval ��
  */
char * lv_pash_joint(void)
{
    lv_flie.lv_prev_file[lv_flie.lv_prev_file_flag] = (char *)lv_flie.lv_pash;/* ����һ��·�����浽��������� */
    lv_flie.lv_prev_file_flag ++;                              /* ǰһ��·��������־λ��һ */
  
    strcpy((char *)lv_flie.pname, lv_flie.lv_pash);            /* ����·��(Ŀ¼) */ 
    strcat((char *)lv_flie.pname, "/");                        /* ����·��(Ŀ¼) */ 
    strcat((char *)lv_flie.pname, (char *)lv_flie.lv_pname);   /* ���ļ������ں��� */
    return lv_flie.pname;
}

/**
  * @brief  �б������ص�����
  * @param  obj  : ����
  * @param  event: �¼�
  * @retval ��
  */
static void lv_list_btn_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);
  
    if(code == LV_EVENT_CLICKED)
    {
        for (int i = 0; i <= lv_flie.list_flie_nuber ;i++)  /* ��ѯ�б��� */
        {
            if (obj == lv_flie.list_btn[i]) /* �ж��б�����Ǹ��������� */
            {
                lv_flie.lv_pname = mymalloc(SRAMIN, sizeof(lv_flie.lv_pname));         /* ��ȡ�ļ��������ڴ� */
                lv_flie.pname = mymalloc(SRAMIN, sizeof(lv_flie.pname));               /* Ϊ��·�����ļ��������ڴ� */
                lv_flie.lv_pname = (char *)lv_list_get_btn_text(lv_flie.list,lv_flie.list_btn[i]);  /* ��ȡ�б����ֵ */
              
                for (int suffix = 0;suffix < LV_SUFFIX(lv_suffix) ;suffix ++)     /* ��ѯ�ļ���׺ */
                {
                    if (strstr(lv_flie.lv_pname,lv_suffix[suffix]) != NULL)       /* ��������ļ��� */
                    {
                        lv_flie.lv_suffix_flag = 0;                               /* ���ú�׺�ı�־λΪ0 */
                        break;
                    }
                }

                if (lv_flie.lv_suffix_flag == 1)                                  /* �ñ�־λ��Ϊ0�����ļ��в��� */
                {   
                    lv_flie.lv_pash = lv_pash_joint();                            /* ���ļ�·��ת�ݸ�lv_pash���� */
                    lv_del_list(lv_flie.list);                                    /* ɾ���б� */
                    lv_scan_files(lv_flie.pname,lv_scr_act());                    /* ���´����ļ��б� */
                }
                else
                {
                    if (strstr(lv_flie.lv_pname,".txt") != NULL)                  /* ��������ļ�������txt�ļ� */
                    {
                        if (lv_file_read(lv_pash_joint()) == LV_FS_RES_OK)        /* �ж϶�ȡtxt�ļ��Ƿ�ɹ� */
                        {
                            lv_flie.lv_return_page = lv_create_page(lv_scr_act());/* ����ҳ�� */
                            lv_show_filetxt(lv_flie.lv_return_page);              /* ��ҳ����ʾtxt�ļ����� */
                        }
                    }
                    else if (strstr(lv_flie.lv_pname,".bin") != NULL)             /* ��������ļ�������bin�ļ� */
                    {
                        lv_flie.lv_return_page = lv_create_page(lv_scr_act());    /* ����ҳ�� */
                        lv_show_imgbin(lv_flie.lv_return_page,lv_pash_joint());   /* ��ʾbinͼƬ */
                    }

                    lv_flie.lv_suffix_flag = 1; /* �ָ��ļ��е�� */
                }
            }
        }
    }
}

/**
  * @brief  ��ȡ�ļ���
  * @param  char* path: Ҫɨ����ļ�·��
  * @retval FR_OK���ɹ�������ʧ��
  */
uint16_t lv_scan_files (const char* path,lv_obj_t *parent)
{
    lv_flie.fr = f_opendir(&lv_flie.lv_dir, path);         /* ���ļ�Ŀ¼* */
    memset(lv_flie.list_btn,0,sizeof(lv_flie.list_btn));   /* ����б�������� */
    lv_flie.list_flie_nuber = 0;                           /* �����ļ�����Ϊ0 */
    lv_create_list(parent);                                /* �����б� */

    if (lv_flie.pname != NULL||lv_flie.lv_pname != NULL)
    {
        myfree(SRAMIN, lv_flie.pname);                    /* �ͷ�pname�ڴ� */
        myfree(SRAMIN, lv_flie.lv_pname);                 /* �ͷ�lv_pname_shift�ڴ� */
    }

    if (lv_flie.fr == FR_OK)
    {   /* ����򿪳ɹ�ѭ�������ļ����ֵ�buff�� */
        while(1)
        {   /* ѭ�������ļ����֣�ѭ����������SD����Ŀ¼�µ��ļ���Ŀ */
            lv_flie.fr = f_readdir(&lv_flie.lv_dir, &lv_flie.SD_fno);        /* ��ȡ�ļ���* */

            if (lv_flie.fr != LV_FS_RES_OK||lv_flie.SD_fno.fname[0] == 0) break;  /* ��ȡ������߶��������ļ�����������ѭ�� */
            lv_flie.list_flie_nuber ++;     /* �ļ�������1 */
           
          if (lv_flie.SD_fno.fattrib& AM_DIR) /* ��ȡ�����ļ������� */
            {
               
                /* �����ļ����ֵ����沢��ӡ�ļ��� */
                lv_flie.list_btn[lv_flie.list_flie_nuber] = lv_list_add_btn(lv_flie.list, LV_SYMBOL_DIRECTORY, lv_flie.SD_fno.fname);      /* �����б���(�ļ���) */
                lv_obj_set_style_img_recolor(lv_flie.list_btn[lv_flie.list_flie_nuber],lv_color_hex(0xFFD700),LV_STATE_DEFAULT);  /* �����ļ���������ɫ */
            }
            else /* ��ȡ�����ļ����� */
            {
              
                if (strstr(lv_flie.SD_fno.fname,".png") != NULL    /* �ж��ļ����� */
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
                
                lv_flie.list_btn[lv_flie.list_flie_nuber] = lv_list_add_btn(lv_flie.list, lv_flie.image_scr, lv_flie.SD_fno.fname);         /* �����б���(�ļ�) */ 
                lv_obj_set_style_img_recolor(lv_flie.list_btn[lv_flie.list_flie_nuber],lv_color_hex(0x87CEFA),LV_STATE_DEFAULT);   /* �����ļ�������ɫ */
            }
            
            lv_obj_set_style_pad_left(lv_flie.list_btn[lv_flie.list_flie_nuber],5,LV_STATE_DEFAULT);   /* �����б������б���ߵľ��� */
            lv_obj_set_style_pad_right(lv_flie.list_btn[lv_flie.list_flie_nuber],5,LV_STATE_DEFAULT);  /* �����б������б��ұߵľ��� */
            lv_obj_add_event_cb(lv_flie.list_btn[lv_flie.list_flie_nuber], lv_list_btn_event,LV_EVENT_ALL,NULL); /* �����б��ص����� */

        }
        
        f_closedir(&lv_flie.lv_dir); /* �ر��ļ�Ŀ¼ */
    }
    
    return lv_flie.fr;                   /* ���� */
}

/**
  * @brief  ɾ���б�
  * @param  parent: ����
  * @retval ��
  */
void lv_del_list(lv_obj_t *parent)
{
    lv_obj_del(parent);   /* ɾ������ */
    lv_flie.list = NULL;  /* �����б�Ϊ�� */
}

/**
  * @brief  �����б�����
  * @param  parent: ����
  * @retval ��
  */
void lv_animation(lv_obj_t *parent)
{
    lv_anim_t a;                                                /* ��һ��������һ������ */
    lv_anim_init(&a);                                           /* �ڶ�����ʼ������ */
    lv_anim_set_var(&a, parent);                                /* ������������ʵʩ����Ч���Ŀؼ� */
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);  /* ���ö������� var */
    int32_t start = lv_obj_get_width(lv_scr_act());
    int32_t end = 0;
    lv_anim_set_values(&a, start, end);                         /* ���ÿ�ʼֵ�ͽ���ֵ������0,150 */
    lv_anim_set_time(&a, 300);                                  /* �����ĳ���[ms]����300mS */
    lv_anim_start(&a);                                          /* ���Ĳ����������� */
}

/**
  * @brief  �����б�
  * @param  parent: ����
  * @retval ��
  */
void lv_create_list(lv_obj_t *parent)
{
    lv_flie.list = lv_list_create(parent);  /* �����б� */
    lv_animation(lv_flie.list);
    lv_obj_set_size(lv_flie.list, lcd_info()->width, lcd_info()->height - lv_font_montserrat_14.line_height*2 - 60);  /* �����б��Ĵ�С */
    lv_obj_align_to(lv_flie.list,lv_flie.lv_page_obj,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);            /* �����б��Ķ���ģʽ */
    lv_obj_set_style_text_font(lv_flie.list,&lv_font_montserrat_14,LV_STATE_DEFAULT);          /* �������� */
    lv_obj_set_style_radius(lv_flie.list,0,LV_STATE_DEFAULT);/* ����Բ�뾶Ϊ0 */
}

/**
  * @brief  ���ذ����ص�����
  * @param  event:�¼�
  * @retval ��
  */
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
        lv_mainstart();/* �򿪵�¼���� */
    }
}

/**
  * @brief  ����ҳ�����
  * @param  parent: ����
  * @retval ��
  */
void lv_page_tile(lv_obj_t *parent)
{
    lv_general_dev.lv_general_win_create = NULL;
    lv_general_dev.lv_back_event = NULL;
    lv_general_dev.parent = NULL;
    
    lv_flie.lv_page_obj = lv_obj_create(parent);
    lv_obj_set_size(lv_flie.lv_page_obj,lcd_info()->width,lv_font_montserrat_14.line_height + 50);
    lv_obj_set_style_bg_color(lv_flie.lv_page_obj,lv_palette_main(LV_PALETTE_GREY),LV_STATE_DEFAULT);
    lv_obj_set_style_radius(lv_flie.lv_page_obj,0,LV_STATE_DEFAULT);/* ����Բ�뾶Ϊ0 */
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

/**
  * @brief  ���ذ����ص�����
  * @param  obj:����
  * @param  event:�¼�
  * @retval ��
  */
void lv_back_btn_event_handler(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);
  
    if(code == LV_EVENT_SHORT_CLICKED)
    {
        if (obj == lv_flie.lv_back_btn)
        {
            lv_del_list(lv_flie.list);         /* ɾ���б� */
            list_init(lv_scr_act());           /* ���ز˵� */
        }
        if (obj == lv_flie.lv_prev_btn)
        { 
            lv_flie.lv_prev_file_flag -- ;     /* �ļ����ر�־λ��һ */

            if (lv_flie.lv_prev_file_flag <= 0)
            {
                lv_flie.lv_prev_file_flag = 0; /* ���С�ڵ���0����ǿ������Ϊ0 */
            }
          
            lv_del_list(lv_flie.list);         /* ����ɾ���б��ؼ� */
            lv_flie.lv_pash = lv_flie.lv_prev_file[lv_flie.lv_prev_file_flag]; /* �ϼ�·�����Ƶ�lv_pash���� */
            lv_scan_files(lv_flie.lv_prev_file[lv_flie.lv_prev_file_flag],lv_scr_act());/* ��ȡ�ļ�·�� */
           
        }
    }
}

/**
  * @brief  ���ذ���
  * @param  parent:����
  * @retval ��
  */
void lv_fs_win_create(lv_obj_t *parent)
{
    lv_flie.lv_back_btn = lv_label_create(parent);
    lv_obj_set_style_text_font(lv_flie.lv_back_btn,&lv_font_montserrat_14,LV_STATE_DEFAULT); /* �������� */
    
    lv_label_set_text(lv_flie.lv_back_btn,"Menu");
    lv_obj_align_to(lv_flie.lv_back_btn,parent,LV_ALIGN_RIGHT_MID,-10,0);
    lv_obj_add_flag(lv_flie.lv_back_btn,LV_OBJ_FLAG_CLICKABLE); /* ���ñ�ǩ�ɵ�� */
    lv_obj_add_event_cb(lv_flie.lv_back_btn,lv_back_btn_event_handler,LV_EVENT_ALL,NULL); /* ���ûص����� */
    lv_flie.lv_prev_btn = lv_label_create(parent);
    lv_obj_set_style_text_font(lv_flie.lv_prev_btn,&lv_font_montserrat_14,LV_STATE_DEFAULT);
    lv_label_set_text(lv_flie.lv_prev_btn,"Return");
    lv_obj_align_to(lv_flie.lv_prev_btn,parent,LV_ALIGN_LEFT_MID,10,0);
    lv_obj_add_flag(lv_flie.lv_prev_btn,LV_OBJ_FLAG_CLICKABLE); /* ���ñ�ǩ�ɵ�� */
    lv_obj_add_event_cb(lv_flie.lv_prev_btn,lv_back_btn_event_handler,LV_EVENT_ALL,NULL);
}

/**
  * @brief  �ļ�ϵͳ����/�˵�����������
  * @param  parent: ����
  * @retval ��
  */
void lv_page_back(lv_obj_t *parent)
{
    lv_flie.lv_back_obj = lv_obj_create(parent);                                                            /* �����ļ����ض������� */
    lv_obj_set_size(lv_flie.lv_back_obj,lcd_info()->width,lv_font_montserrat_14.line_height+10);                              /* ���ø�����Ĵ�С */
    lv_obj_set_style_bg_color(lv_flie.lv_back_obj,lv_palette_main(LV_PALETTE_GREY),LV_STATE_DEFAULT);       /* ���ø��������ɫΪ��ɫ */
    lv_obj_set_style_radius(lv_flie.lv_back_obj,0,LV_STATE_DEFAULT);                                        /* ����Բ�뾶Ϊ0 */
    lv_obj_align_to(lv_flie.lv_back_obj,parent,LV_ALIGN_BOTTOM_MID,0,0);                                    /* ���ö���ģʽ */
    lv_fs_win_create(lv_flie.lv_back_obj);                                                                  /* �������غͲ˵����� */
}

/**
  * @brief  �б���ʼ��
  * @param  parent: ����
  * @retval ��
  */
void list_init(lv_obj_t *parent)
{
    lv_flie.lv_pash = "0:";                                 /* ��ʼ·�� */
    lv_flie.lv_prev_file_flag = 0;                          /* ��һ���ļ�·����־λ��0 */
    lv_flie.lv_prev_file[lv_flie.lv_prev_file_flag] = "0:"; /* ��ʼ�ϸ��ļ���·�� */
    lv_flie.list_flie_nuber = 0;                            /* ��ʼ�ļ����� */
    lv_flie.lv_suffix_flag = 1;                             /* �ļ���׺��־λ */
    lv_scan_files(lv_flie.lv_pash,parent);                  /* ��ȡ�ļ�·�� */
}

/**
  * @brief  �ؼ����Ժ���
  * @param  ��
  * @retval ��
  */
void lv_file_demo(void)
{
    lv_page_tile(lv_scr_act());         /* ����ҳ����� */
    lv_page_back(lv_scr_act());         /* ����ҳ�淵�� */
    list_init(lv_scr_act());            /* �б���ʼ�� */
}
