/**
 * @file    lv_scale.c
 * @brief   Base converter.
 */

#include "lv_scale.h"
#include "lv_mainstart.h"
#include "lvgl.h"
#include "lcd.h"
#include "malloc.h"
#include "stdio.h"
#include "stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
#include "math.h"


/* numeric / hex keyboard */
static const char* btnm1_map[24]={"1","2","3","\n",
                                  "4","5","6","\n",
                                  "7","8","9","\n",
                                  "0","A","B","\n",
                                  "C","D","E","\n",
                                  "F",LV_SYMBOL_CLOSE,LV_SYMBOL_OK,""};

#define LV_ATK_BIT_W                     25 /* bit width */
#define LV_ATK_BIT_H                     35 /* bit height */
#define LV_ATK_BIT_INDEX_H               20 /* bit index height */
#define LV_ATK_BIT_ROW_SPACE             22 /* bit-row spacing */
#define LV_ATK_BIT_COUNT                 33 /* number of bits */
char lv_bit_flag[LV_ATK_BIT_COUNT];         /* 32-bit value */

typedef struct
{
    lv_obj_t *lv_scale_cont;                    /* container */
    lv_obj_t *lv_btnmatrix;                     /* on-screen keyboard */
    lv_obj_t *lv_bit_index[LV_ATK_BIT_COUNT];   /* bit index labels */
    lv_obj_t *lv_bit_text[LV_ATK_BIT_COUNT];    /* bit buttons */
    lv_obj_t *lv_bin;                           /* binary */
    lv_obj_t *lv_dec;                           /* decimal */
    lv_obj_t *lv_hex;                           /* hexadecimal */
    lv_obj_t *lv_oct;                           /* octal */
    lv_obj_t *lv_bin_label;                     /* binary label */
    lv_obj_t *lv_dec_label;                     /* decimal label */
    lv_obj_t *lv_hex_label;                     /* hexadecimal label */
    lv_obj_t *lv_oct_label;                     /* octal label */
    lv_obj_t *lv_middle;                        /* active text area */
}lv_scale_t;

lv_scale_t lv_scale;


/* Reverse a string in place. */
static void lv_atk_bit_str_reverse(char str[])
{
    int n=strlen(str);
    int i;
    char temp;
  
    for (i = 0;i < (n/2); i++)
    {
        temp = str[i];
        str[i] = str[n-i-1];
        str[n-i-1] = temp;
    }
}

/* Binary string -> decimal. */
static long lv_atk_bit_bin_to_dec(const char *pbin)
{
    int ii=0;
    long result=0;

    while (pbin[ii] != 0)
    {
        result = result * 2 + (pbin[ii] - '0');
        ii++;
    }

    return result;
}

/* Find the index of the pressed bit button. */
int lv_atk_bit_to_dec(lv_obj_t *parent)
{
    int  j = 0;

    for (int i = 0 ; i < LV_ATK_BIT_COUNT; i++)
    {

        if (parent == lv_scale.lv_bit_text[i])
        {
            return j;
        }
        
        j ++;
    }
    
    return j;
}

/* Decimal -> octal. */
long lv_atk_dec_to_oct(long dec)
{
    int oct = 0, i = 0;

    i = 1;

    while (dec != 0)
    {
        oct += (dec % 8) * i;
        dec /= 8;
        i *= 10;
    }

    return oct;
}

/* Decimal -> binary. */
long lv_atk_bec_to_bin(long n)
{

    long result=0,k=1,i,temp;
    temp = n;

    while(temp)
    {
        i = temp%2;
        result = k * i + result;
        k = k*10;
        temp = temp/2;
    }
    printf("%ld\n", result);
    
    return result;
}

/* Hexadecimal string -> decimal. */
long lv_atk_hex_to_dex(char*s)
{
    int i,t;
    long sum=0;
  
    for(i=0;s[i];i++)
    { 
        if(s[i]>= '0'  &&s[i]<='9')/* '0'..'9' -> *-'0' */
        {
            t=s[i]-'0';
        }
        
        if(s[i]>='a'&&s[i]<='z')
        {
            t=s[i]-'a'+10;/* 'a'..'z' -> +10 */
        }
        
        if(s[i]>='A'&&s[i]<='Z')
        {
            t=s[i]-'A'+10;/* 'A'..'Z' -> +10 */
        }
        
        sum=sum*16+t; 
    }
    
    return sum;
}

/* Octal -> decimal. */
long lv_atk_oct_to_dex(long n)
{
    int i=0,tmp,sum=0;

    while(n)
    {
        tmp=n%10;
        n=n/10;
        sum+=tmp*pow(8,i);
        i++;
    }
    
    printf("%d",sum);
    
    return sum;
}


/*
 * Bit-button click handler: toggles the bit and updates all base fields.
 */
static void lv_event_bit_map_handler(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);
  
    if(code == LV_EVENT_CLICKED)
    {
        static char lv_buf[32];
        int lv_bit_post = 0;
        char lv_str_buf[32];
        char lv_oct_buf[32];
        char lv_bit_flag_buf[LV_ATK_BIT_COUNT - 1] = "00000000000000000000000000000000";
      
        if (lv_scale.lv_btnmatrix != NULL)
        {
            lv_obj_del(lv_scale.lv_btnmatrix); /* delete the keyboard */
            lv_textarea_set_cursor_click_pos(lv_scale.lv_middle, true);  /* re-enable cursor */
            lv_scale.lv_middle = NULL;
            lv_scale.lv_btnmatrix = NULL;
        }
        
        lv_textarea_set_text(lv_scale.lv_bin, "");
        lv_textarea_set_text(lv_scale.lv_dec, "");
        lv_textarea_set_text(lv_scale.lv_hex, "");
        lv_textarea_set_text(lv_scale.lv_oct, "");
        lv_textarea_set_placeholder_text(lv_scale.lv_bin, "");
        lv_textarea_set_placeholder_text(lv_scale.lv_dec, "");
        lv_textarea_set_placeholder_text(lv_scale.lv_hex, "");
        lv_textarea_set_placeholder_text(lv_scale.lv_oct, "");
        lv_obj_t * lv_obj_child = lv_obj_get_child(obj, 0);
        lv_bit_post = lv_atk_bit_to_dec(obj);

        lv_snprintf(lv_buf, sizeof(lv_buf), "%s",lv_label_get_text(lv_obj_child));

        if (strcmp(lv_buf, "1") == 0)
        {
            lv_label_set_text(lv_obj_child, "0");
            lv_bit_flag[lv_bit_post] = '0';
        }
        else if (strcmp(lv_buf, "0") == 0)
        {
            lv_label_set_text(lv_obj_child, "1");
            lv_bit_flag[lv_bit_post] = '1';
        }

        strcpy(lv_bit_flag_buf, lv_bit_flag);
        
        lv_atk_bit_str_reverse(lv_bit_flag_buf);
        
        lv_textarea_set_placeholder_text(lv_scale.lv_bin, lv_bit_flag_buf);
        const char * lv_show_lv_atk_str_buffer = lv_textarea_get_placeholder_text(lv_scale.lv_bin);                    /* read binary */
        lv_snprintf(lv_str_buf, sizeof(lv_str_buf), "%d", lv_atk_bit_bin_to_dec(lv_show_lv_atk_str_buffer));  /* binary -> decimal */
        lv_textarea_set_placeholder_text(lv_scale.lv_dec, lv_str_buf);
        lv_snprintf(lv_oct_buf, sizeof(lv_oct_buf), "%d", lv_atk_dec_to_oct(atof((const char *)lv_str_buf)));
        lv_textarea_set_placeholder_text(lv_scale.lv_oct, lv_oct_buf);                                                 /* decimal -> octal */
        lv_snprintf(lv_str_buf, sizeof(lv_str_buf),"0x%X", lv_atk_bit_bin_to_dec(lv_show_lv_atk_str_buffer)); /* binary -> hex */
        lv_textarea_set_placeholder_text(lv_scale.lv_hex, lv_str_buf);
    }
}


/* btnmatrix key handler. */
static void lv_btnmatrix_event_handler(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);

    if(code == LV_EVENT_VALUE_CHANGED)
    {
        const char * txt = lv_btnmatrix_get_btn_text(lv_scale.lv_btnmatrix,lv_btnmatrix_get_selected_btn(obj));
      
        if (txt == btnm1_map[22]||txt == btnm1_map[21])
        {
            lv_obj_del(lv_scale.lv_btnmatrix); /* delete the keyboard */
            lv_scale.lv_btnmatrix = NULL;
            lv_scale.lv_middle = NULL;
        }
        
        for (int i = 0 ;i < 20 ;i ++)
        {
            if (txt == btnm1_map[i])
            {
                lv_textarea_add_text(lv_scale.lv_middle,(const char *)txt);
                break;
            }
        }

    }
}

/* Text-area handler: open the keyboard and convert between bases. */
static void lv_ta_cb_event_handler(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * ta = lv_event_get_target(event);
  
    if((code == LV_EVENT_CLICKED) && (lv_scale.lv_btnmatrix == NULL))
    {
        lv_scale.lv_middle = NULL;
        lv_textarea_set_cursor_click_pos(ta, false);               /* disable cursor */
        lv_scale.lv_btnmatrix = lv_btnmatrix_create(lv_scr_act()); /* create the keyboard */
        lv_btnmatrix_set_map(lv_scale.lv_btnmatrix, btnm1_map);          /* set the map */
        lv_btnmatrix_set_btn_width(lv_scale.lv_btnmatrix, 12, 1);        /* button width */
        lv_obj_set_size(lv_scale.lv_btnmatrix,lcd_info()->width,lcd_info()->height/2);    /* size */
        lv_obj_align_to(lv_scale.lv_btnmatrix, NULL, LV_ALIGN_BOTTOM_MID, 0, 0);/* align */
        lv_obj_add_event_cb(lv_scale.lv_btnmatrix, lv_btnmatrix_event_handler,LV_EVENT_ALL,NULL); /* callback */

      
        /* clear the text areas */
        lv_textarea_set_text(lv_scale.lv_bin, "");
        lv_textarea_set_text(lv_scale.lv_dec, "");
        lv_textarea_set_text(lv_scale.lv_hex, "");
        lv_textarea_set_text(lv_scale.lv_oct, "");
        lv_textarea_set_placeholder_text(lv_scale.lv_bin, "");
        lv_textarea_set_placeholder_text(lv_scale.lv_dec, "");
        lv_textarea_set_placeholder_text(lv_scale.lv_hex, "");
        lv_textarea_set_placeholder_text(lv_scale.lv_oct, "");
        /* ------- */
      
        if (ta == lv_scale.lv_bin) /* binary input */
        {
            for (int i = 0 ; i < 18 ; i++)
            {
              if (i == 0 || i == 9 || i == 16 || i == 17) /* keep 0, 9, 16, 17 */
                {
                    continue;
                }
                else /* disable the others */
                {
                    lv_btnmatrix_set_btn_ctrl(lv_scale.lv_btnmatrix, i, LV_BTNMATRIX_CTRL_DISABLED);
                }
            }
            lv_scale.lv_middle = lv_scale.lv_bin; /* active input */
            lv_textarea_set_cursor_click_pos(lv_scale.lv_dec, true);  /* re-enable cursor */
            lv_textarea_set_cursor_click_pos(lv_scale.lv_hex, true);
            lv_textarea_set_cursor_click_pos(lv_scale.lv_oct, true);
        }
        else if (ta == lv_scale.lv_dec)
        {
            for (int i = 0 ; i < 18 ; i++)
            {
                if (i == 10 || i == 11 || i == 12 || i == 13 || i == 14 || i == 15)
                {
                    lv_btnmatrix_set_btn_ctrl(lv_scale.lv_btnmatrix, i, LV_BTNMATRIX_CTRL_DISABLED);
                }
            }
            lv_scale.lv_middle = lv_scale.lv_dec;
            lv_textarea_set_cursor_click_pos(lv_scale.lv_bin, true);  /* re-enable cursor */
            lv_textarea_set_cursor_click_pos(lv_scale.lv_hex, true);
            lv_textarea_set_cursor_click_pos(lv_scale.lv_oct, true);
        }
        else if (ta == lv_scale.lv_hex)
        {
            for (int i = 0 ; i < 19 ; i++)
            {
                lv_btnmatrix_set_btn_ctrl(lv_scale.lv_btnmatrix, i, LV_BTNMATRIX_CTRL_CHECKABLE);
            }
            lv_scale.lv_middle = lv_scale.lv_hex;
            lv_textarea_set_cursor_click_pos(lv_scale.lv_bin, true);  /* re-enable cursor */
            lv_textarea_set_cursor_click_pos(lv_scale.lv_dec, true);
            lv_textarea_set_cursor_click_pos(lv_scale.lv_oct, true);
        }
        else if (ta == lv_scale.lv_oct)
        {
            for (int i = 0 ; i < 18 ; i++)
            {
                if (i > 6 && i < 16)
                {
                    lv_btnmatrix_set_btn_ctrl(lv_scale.lv_btnmatrix, i, LV_BTNMATRIX_CTRL_DISABLED);
                }
            }
            lv_scale.lv_middle = lv_scale.lv_oct;
            lv_textarea_set_cursor_click_pos(lv_scale.lv_bin, true);  /* re-enable cursor */
            lv_textarea_set_cursor_click_pos(lv_scale.lv_dec, true);
            lv_textarea_set_cursor_click_pos(lv_scale.lv_hex, true);
        }
    }
    else if(code == LV_EVENT_VALUE_CHANGED) /* text changed: convert */
    {
        /* ---- base conversion ---- */
        char str_tmp[32];        /* scratch */
        char lv_oct_buf[32];     /* scratch */
        char lv_dex_oct_buf[32]; /* scratch */
        static char regbit_flag_buf[32];
        const char * lv_atk_str_buffer = lv_textarea_get_text(ta); /* current text */
      
        if (ta == lv_scale.lv_bin) /* binary */
        {
            if (lv_atk_str_buffer != NULL)
            {
                memset(lv_oct_buf,0,sizeof(lv_oct_buf));
                lv_snprintf(str_tmp, sizeof(str_tmp),"%d", lv_atk_bit_bin_to_dec(lv_atk_str_buffer));               /* binary -> decimal */
                lv_textarea_set_placeholder_text(lv_scale.lv_dec, str_tmp);
                lv_snprintf(lv_oct_buf, sizeof(lv_oct_buf), "%d", lv_atk_dec_to_oct(atof((const char *)str_tmp)));
                lv_textarea_set_placeholder_text(lv_scale.lv_oct, lv_oct_buf);                                      /* decimal -> octal */
                lv_snprintf(str_tmp, sizeof(str_tmp), "0x%X", lv_atk_bit_bin_to_dec(lv_atk_str_buffer));            /* binary -> hex */
                lv_textarea_set_placeholder_text(lv_scale.lv_hex, str_tmp);

            }
            else /* empty */
            {
                lv_textarea_set_placeholder_text(lv_scale.lv_dec, "");
                lv_textarea_set_placeholder_text(lv_scale.lv_hex, "");
                lv_textarea_set_placeholder_text(lv_scale.lv_oct, "");
            }
        }
        else if (ta == lv_scale.lv_dec) /* decimal */
        {
            if (lv_atk_str_buffer != NULL)
            {
                /* decimal -> binary */
                memset(regbit_flag_buf,0,sizeof(regbit_flag_buf));
                memset(lv_oct_buf,0,sizeof(lv_oct_buf));
                sprintf(regbit_flag_buf,"%ld",lv_atk_bec_to_bin(atof((const char *)lv_atk_str_buffer)));
                lv_textarea_set_placeholder_text(lv_scale.lv_bin, regbit_flag_buf);
                lv_snprintf(lv_oct_buf, sizeof(lv_oct_buf), "%d", lv_atk_dec_to_oct(atof((const char *)lv_atk_str_buffer)));  /* decimal -> octal */
                lv_textarea_set_placeholder_text(lv_scale.lv_oct, lv_oct_buf);
                lv_snprintf(str_tmp, sizeof(str_tmp),"0x%X", lv_atk_bit_bin_to_dec(regbit_flag_buf));                         /* binary -> hex */
                lv_textarea_set_placeholder_text(lv_scale.lv_hex, str_tmp);
            }
            else
            {
                lv_textarea_set_placeholder_text(lv_scale.lv_bin, "");
                lv_textarea_set_placeholder_text(lv_scale.lv_hex, "");
                lv_textarea_set_placeholder_text(lv_scale.lv_oct, "");
            }

        }
        else if (ta == lv_scale.lv_hex) /* hexadecimal */
        {
            if (lv_atk_str_buffer != NULL)
            {
                memset(lv_oct_buf,0,sizeof(lv_oct_buf));
                memset(regbit_flag_buf,0,sizeof(regbit_flag_buf));
                memset(lv_dex_oct_buf,0,sizeof(lv_dex_oct_buf));
                lv_snprintf(lv_oct_buf, sizeof(lv_oct_buf), "%ld", lv_atk_hex_to_dex((char *)lv_atk_str_buffer));              /* hex -> decimal */
                lv_textarea_set_placeholder_text(lv_scale.lv_dec, lv_oct_buf);
              
                sprintf(regbit_flag_buf,"%ld",lv_atk_bec_to_bin(atof((const char *)lv_oct_buf)));
                lv_textarea_set_placeholder_text(lv_scale.lv_bin, regbit_flag_buf);

                lv_snprintf(lv_dex_oct_buf, sizeof(lv_dex_oct_buf), "%d", lv_atk_dec_to_oct(atof((const char *)lv_oct_buf)));  /* decimal -> octal */
                lv_textarea_set_placeholder_text(lv_scale.lv_oct, lv_dex_oct_buf);
                lv_snprintf(str_tmp, sizeof(str_tmp),"0x%X", lv_atk_bit_bin_to_dec(regbit_flag_buf));                          /* binary -> hex */
                lv_textarea_set_placeholder_text(lv_scale.lv_hex, str_tmp);
            }
            else
            {
                lv_textarea_set_placeholder_text(lv_scale.lv_dec, "");
                lv_textarea_set_placeholder_text(lv_scale.lv_bin, "");
                lv_textarea_set_placeholder_text(lv_scale.lv_oct, "");
            }
        }
        else if (ta == lv_scale.lv_oct) /* octal */
        {
            if (lv_atk_str_buffer != NULL)
            {
                memset(lv_oct_buf,0,sizeof(lv_oct_buf));
                memset(regbit_flag_buf,0,sizeof(regbit_flag_buf));
                lv_snprintf(lv_oct_buf, sizeof(lv_oct_buf), "%ld", lv_atk_oct_to_dex(atof((const char *)lv_atk_str_buffer)));  /* octal -> decimal */
                lv_textarea_set_placeholder_text(lv_scale.lv_dec, lv_oct_buf);
              
                sprintf(regbit_flag_buf,"%ld",lv_atk_bec_to_bin(atof((const char *)lv_oct_buf)));                              /* decimal -> binary */
                lv_textarea_set_placeholder_text(lv_scale.lv_bin, regbit_flag_buf);

                lv_snprintf(str_tmp, sizeof(str_tmp),"0x%X", lv_atk_bit_bin_to_dec(regbit_flag_buf));                          /* binary -> hex */
                lv_textarea_set_placeholder_text(lv_scale.lv_hex, str_tmp);
            }
            else
            {
                lv_textarea_set_placeholder_text(lv_scale.lv_dec, "");
                lv_textarea_set_placeholder_text(lv_scale.lv_bin, "");
                lv_textarea_set_placeholder_text(lv_scale.lv_hex, "");
            }
        }
    }
    
}

/* Build the bit grid and the four base text areas. */
void lv_atk_scale_bit_init(lv_obj_t *parent)
{
    int lv_scale_col_1 = 0;   /* column */
    lv_obj_t *lv_obj_text;

    for (int i = 0 ; i < LV_ATK_BIT_COUNT; i++)
    {
        /* bit index */
        lv_scale.lv_bit_index[i] = lv_obj_create(parent);                                                  /* index object */
        lv_obj_set_style_bg_color(lv_scale.lv_bit_index[i], lv_color_hex3(0x00), LV_STATE_DEFAULT);        /* background */
        lv_obj_set_style_border_side(lv_scale.lv_bit_index[i], 0, LV_STATE_DEFAULT);                       /* no border */
        lv_obj_set_style_text_color(lv_scale.lv_bit_index[i], lv_color_hex3(0xffffff), LV_STATE_DEFAULT);  /* text color */
        lv_obj_set_style_radius(lv_scale.lv_bit_index[i], 2, LV_STATE_DEFAULT);                            /* radius */
        lv_obj_set_size(lv_scale.lv_bit_index[i], LV_ATK_BIT_W, LV_ATK_BIT_INDEX_H);                       /* size */

        /* bit button */
        lv_scale.lv_bit_text[i] = lv_btn_create(parent);                                                       /* bit button */
        lv_obj_set_style_radius(lv_scale.lv_bit_text[i], 5, LV_STATE_DEFAULT);                                 /* radius */
        lv_obj_set_style_bg_color(lv_scale.lv_bit_text[i], lv_palette_main(LV_PALETTE_RED), LV_STATE_DEFAULT); /* background */
        lv_obj_set_style_border_side(lv_scale.lv_bit_text[i], 0, LV_STATE_DEFAULT);                            /* no border */
        lv_obj_set_style_text_color(lv_scale.lv_bit_text[i], lv_color_hex3(0xffffff), LV_STATE_DEFAULT);       /* text color */
        lv_obj_set_size(lv_scale.lv_bit_text[i], LV_ATK_BIT_W, LV_ATK_BIT_H);                                  /* size */
        lv_obj_add_event_cb(lv_scale.lv_bit_text[i], lv_event_bit_map_handler,LV_EVENT_ALL,NULL);              /* callback */
        /* bit label */
        lv_obj_text  = lv_label_create(lv_scale.lv_bit_text[i]);    /* label */
        lv_label_set_text(lv_obj_text, "0");
        lv_obj_align_to(lv_obj_text, NULL, LV_ALIGN_CENTER, 0, 0);  /* center */
        lv_obj_align_to(lv_scale.lv_bit_index[i], parent, LV_ALIGN_TOP_RIGHT, -(LV_ATK_BIT_ROW_SPACE*(lv_scale_col_1 ++ )+3), 50); /* layout */
        lv_obj_align_to(lv_scale.lv_bit_text[i], lv_scale.lv_bit_index[i], LV_ALIGN_OUT_BOTTOM_MID, 0, 0);                         /* layout */
        
        if (i == 32)
        {
            lv_obj_set_style_bg_color(lv_scale.lv_bit_text[32], lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);  /* background */
            lv_obj_set_style_bg_color(lv_scale.lv_bit_index[32], lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT); /* background */
            lv_label_set_text(lv_obj_get_child(lv_scale.lv_bit_text[32], 0), "");
        }

    }
    
    lv_scale.lv_bin_label = lv_label_create(parent);
    lv_label_set_text(lv_scale.lv_bin_label,"BIN");
    lv_obj_align_to(lv_scale.lv_bin_label,lv_scale.lv_bit_text[LV_ATK_BIT_COUNT - 1],LV_ALIGN_OUT_BOTTOM_MID,0,20);
    lv_obj_set_style_text_color(lv_scale.lv_bin_label,lv_color_make(255,255,255),LV_STATE_DEFAULT);
    /* binary */
    lv_scale.lv_bin = lv_textarea_create(parent);
    lv_textarea_set_accepted_chars(lv_scale.lv_bin, "01");                                   /* only '0'/'1' */
    lv_obj_set_style_text_font(lv_scale.lv_bin, &lv_font_montserrat_14, LV_STATE_DEFAULT);   /* font */
    lv_obj_set_style_radius(lv_scale.lv_bin, 0, LV_STATE_DEFAULT);                           /* radius */
    lv_obj_set_size(lv_scale.lv_bin,lcd_info()->width/2,LV_ATK_BIT_H);
    lv_textarea_set_text(lv_scale.lv_bin, "");
    lv_textarea_set_one_line(lv_scale.lv_bin, true);
    lv_textarea_set_cursor_click_pos(lv_scale.lv_bin, true);
    lv_obj_add_event_cb(lv_scale.lv_bin, lv_ta_cb_event_handler,LV_EVENT_ALL,NULL);
    lv_obj_align_to(lv_scale.lv_bin, lv_scale.lv_bin_label, LV_ALIGN_OUT_RIGHT_MID, 12, 0);  /* layout */
    
    
    lv_scale.lv_dec_label = lv_label_create(parent);
    lv_label_set_text(lv_scale.lv_dec_label,"DEC");
    lv_obj_align_to(lv_scale.lv_dec_label,lv_scale.lv_bin_label,LV_ALIGN_OUT_BOTTOM_MID,0,20);
    lv_obj_set_style_text_color(lv_scale.lv_dec_label,lv_color_make(255,255,255),LV_STATE_DEFAULT);
    /* decimal */
    lv_scale.lv_dec = lv_textarea_create(parent);
    lv_textarea_set_accepted_chars(lv_scale.lv_dec, "0123456789");                          /* digits only */
    lv_obj_set_style_text_font(lv_scale.lv_dec, &lv_font_montserrat_14, LV_STATE_DEFAULT);  /* font */
    lv_obj_set_style_radius(lv_scale.lv_dec, 0, LV_STATE_DEFAULT);                          /* radius */
    lv_obj_set_size(lv_scale.lv_dec,lcd_info()->width/2,LV_ATK_BIT_H);
    lv_textarea_set_text(lv_scale.lv_dec, "");
    lv_textarea_set_one_line(lv_scale.lv_dec,lv_scale.lv_dec);
    lv_textarea_set_cursor_click_pos(lv_scale.lv_dec, true);
    lv_obj_add_event_cb(lv_scale.lv_dec, lv_ta_cb_event_handler,LV_EVENT_ALL,NULL);
    lv_obj_align_to(lv_scale.lv_dec, lv_scale.lv_dec_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);  /* layout */
    
    lv_scale.lv_hex_label = lv_label_create(parent);
    lv_label_set_text(lv_scale.lv_hex_label,"HEX");
    lv_obj_align_to(lv_scale.lv_hex_label,lv_scale.lv_dec_label,LV_ALIGN_OUT_BOTTOM_MID,0,20);
    lv_obj_set_style_text_color(lv_scale.lv_hex_label,lv_color_make(255,255,255),LV_STATE_DEFAULT);
    /* hexadecimal */
    lv_scale.lv_hex = lv_textarea_create(parent);
    lv_obj_set_style_text_color(lv_scale.lv_hex,lv_color_make(255,255,255),LV_STATE_DEFAULT);
    lv_textarea_set_accepted_chars(lv_scale.lv_hex, "0123456789ABCDEFabcdef");              /* hex chars only */
    lv_obj_set_style_text_font(lv_scale.lv_hex, &lv_font_montserrat_14, LV_STATE_DEFAULT);  /* font */
    lv_obj_set_style_radius(lv_scale.lv_hex, 0, LV_STATE_DEFAULT);        /*  radius */
    lv_obj_set_size(lv_scale.lv_hex,lcd_info()->width/2,LV_ATK_BIT_H);
    lv_textarea_set_text(lv_scale.lv_hex, "");
    lv_textarea_set_one_line(lv_scale.lv_hex, true);
    lv_textarea_set_cursor_click_pos(lv_scale.lv_hex, true);
    lv_obj_add_event_cb(lv_scale.lv_hex, lv_ta_cb_event_handler,LV_EVENT_ALL,NULL);
    lv_obj_align_to(lv_scale.lv_hex, lv_scale.lv_hex_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);  /* layout */
    
    lv_scale.lv_oct_label = lv_label_create(parent);
    lv_label_set_text(lv_scale.lv_oct_label,"OCT");
    lv_obj_align_to(lv_scale.lv_oct_label,lv_scale.lv_hex_label,LV_ALIGN_OUT_BOTTOM_MID,0,20);
    lv_obj_set_style_text_color(lv_scale.lv_oct_label,lv_color_make(255,255,255),LV_STATE_DEFAULT);
    /* octal */
    lv_scale.lv_oct = lv_textarea_create(parent);
    lv_textarea_set_accepted_chars(lv_scale.lv_oct, "01234567");                            /* octal chars only */
    lv_obj_set_style_text_font(lv_scale.lv_oct, &lv_font_montserrat_14, LV_STATE_DEFAULT);  /* font */
    lv_obj_set_style_radius(lv_scale.lv_oct, 0, LV_STATE_DEFAULT);                          /* radius */
    lv_obj_set_size(lv_scale.lv_oct,lcd_info()->width/2,LV_ATK_BIT_H);
    lv_textarea_set_text(lv_scale.lv_oct, "");
    lv_textarea_set_one_line(lv_scale.lv_oct, true);
    lv_textarea_set_cursor_click_pos(lv_scale.lv_oct, true);
    lv_obj_add_event_cb(lv_scale.lv_oct, lv_ta_cb_event_handler,LV_EVENT_ALL,NULL);
    lv_obj_align_to(lv_scale.lv_oct, lv_scale.lv_oct_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);  /*  layout */
}

/* Back-button callback. */
void lv_3d_back_btn_event_handler(lv_event_t *event)
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
        lv_obj_del(lv_scale.lv_scale_cont);
        lv_mainstart();/* reopen the launcher */
    }
}

/* Demo entry. */
void lv_scale_demo(void)
{
    lv_snprintf(lv_bit_flag, sizeof(lv_bit_flag), "000000000000000000000000000000000"); /* bit flags */

    lv_general_dev.lv_general_win_create = NULL;
    lv_general_dev.lv_back_event = NULL;
    lv_general_dev.parent = NULL;

    lv_scale.lv_scale_cont = lv_obj_create(lv_scr_act());                    /* container */
    lv_obj_set_style_bg_color(lv_scale.lv_scale_cont, lv_color_make(1, 27, 54), LV_STATE_DEFAULT);
    lv_obj_set_size(lv_scale.lv_scale_cont, lv_obj_get_width(lv_scr_act()), lv_obj_get_height(lv_scr_act()));
    lv_obj_set_style_radius(lv_scale.lv_scale_cont, 0, LV_PART_MAIN);
    lv_obj_add_flag(lv_scale.lv_scale_cont,LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_flag(lv_scale.lv_scale_cont,LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    
    lv_general_dev.lv_back_event = lv_3d_back_btn_event_handler;
    lv_general_dev.parent = lv_scale.lv_scale_cont;
    lv_general_dev.lv_general_win_create = lv_general_win_create;
    lv_general_dev.lv_general_win_create();
    lv_obj_set_pos(lv_scale.lv_scale_cont,0,0);                              /* position */
    lv_scale.lv_btnmatrix = NULL;                                            /* keyboard = none */
    lv_atk_scale_bit_init(lv_scale.lv_scale_cont);                           /* init the bits */
}
