/**
 * @file    lv_mainstart.h
 * @brief   LVGL comprehensive demo launcher.
 */

#ifndef LV_MAINSTART_H
#define LV_MAINSTART_H

#include "lvgl.h"
#include "exfuns.h"

/* sub-window control block */
typedef struct
{
    void (*lv_general_win_create)(void); /* create the sub-window */
    lv_obj_t* parent;
    lv_event_cb_t lv_back_event;
} lv_m_general;

#define BACK_BTN_TITLE    LV_SYMBOL_LEFT" Back\n"   /* back button text */

extern lv_m_general lv_general_dev;     /* sub-window control */

void lv_general_win_create(void);
int lv_clz(unsigned int  app_readly_list[]);
void lv_mainstart(void);

#endif
