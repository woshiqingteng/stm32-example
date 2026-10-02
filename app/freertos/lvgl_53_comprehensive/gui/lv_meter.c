/**
 * @file    lv_meter.c
 * @brief   LVGL clock demo.
 */

#include "lv_meter.h"
#include "lv_mainstart.h"


LV_IMG_DECLARE(img_hand)

/* clock bitmap paths on the SD card */
char *const METER_IMAGE_GBK_PATH[3] =
{
    "0:/PICTURE/LVGLBIN/lv_bg.bin",
    "0:/PICTURE/LVGLBIN/lv_Greater.bin",
    "0:/PICTURE/LVGLBIN/lv_Rain.bin",
};

static lv_obj_t* meter;
static lv_obj_t* lv_meter_cont;

lv_meter_indicator_t* indic_second;
lv_meter_indicator_t* indic_min;
lv_meter_indicator_t* indic_hour;

lv_timer_t* lv_meter_timer;

static lv_point_t line_points1[] = { {0, 0}, {50, 0}, {50, 85}, {0, 85} };
static lv_point_t line_points2[] = { {50, 85}, {0, 85},{0, 0}, {50, 0} };
typedef struct
{
    char* setting_app_text_English;
}meter_image_info;


static const meter_image_info meter_image[] =
{
    {"Thursday"},
    {"November 5th"},
    {"20°C"},
    {"18°C"},
};

/* number of image paths */
#define meter_image_mun (int)(sizeof(meter_image)/sizeof(meter_image[0]))

int mis_mun = 0;
int min_mun = 0;
int hin_mun = 0;
int lv_hin_mun = 0;
int n = 1;

/* Back-button callback. */
void lv_meter_back_btn_event_handler(lv_event_t* event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t* obj = lv_event_get_target(event);

    if (code == LV_EVENT_PRESSED)
    {
        lv_label_set_text(obj, "#444444 "BACK_BTN_TITLE"#");
        lv_label_set_recolor(obj, true);
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        lv_obj_del(lv_meter_cont);
        lv_timer_del(lv_meter_timer);
        lv_mainstart();/* reopen the launcher */
    }
}

/* Clock tick: advance second/minute/hour indicators. */
void lv_meter_vlaue(lv_timer_t* task)
{
    static char lv_time_buf[32];
    mis_mun++;
    memset(lv_time_buf, 0, sizeof(lv_time_buf));
    lv_meter_timer = task;

    if (mis_mun >= 60)
    {
        mis_mun = 0;
        min_mun++;
    }

    if (n > 5)
    {
        n = 1;
    }

    if (min_mun == 12 * n && (mis_mun == 0)) /* minute hand advances every 5 minutes */
    {
        n++;
        hin_mun++;
    }

    if (hin_mun % 5 == 0 && (min_mun == 60) && hin_mun != 0) /* bump lv_hin_mun every 5 minutes */
    {
        lv_hin_mun++;
    }

    if (min_mun >= 60) /* wrap minutes */
    {
        min_mun = 0;
    }

    if (lv_hin_mun >= 12) /* wrap hour (12h dial) */
    {
        lv_hin_mun = 0;
    }

    if (hin_mun >= 60)  /* wrap hours */
    {
        hin_mun = 0;
    }

    lv_snprintf(lv_time_buf, sizeof(lv_time_buf), "%d:%d:%d", lv_hin_mun, min_mun, mis_mun);

    lv_meter_set_indicator_end_value(meter, indic_second, mis_mun);
    lv_meter_set_indicator_end_value(meter, indic_min, min_mun);
    lv_meter_set_indicator_end_value(meter, indic_hour, hin_mun);
}

/* Demo entry. */
void lv_meter_demo(void)
{
    lv_general_dev.lv_general_win_create = NULL;
    lv_general_dev.lv_back_event = NULL;
    lv_general_dev.parent = NULL;

    lv_meter_cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(lv_meter_cont, lv_color_make(1, 27, 54), LV_STATE_DEFAULT);
    lv_obj_set_size(lv_meter_cont, lv_obj_get_width(lv_scr_act()), lv_obj_get_height(lv_scr_act()));
    lv_obj_set_style_radius(lv_meter_cont, 0, LV_PART_MAIN);
    lv_obj_clear_flag(lv_meter_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lv_mater_bg = lv_img_create(lv_meter_cont);
    lv_img_set_src(lv_mater_bg,METER_IMAGE_GBK_PATH[0]);
    lv_obj_center(lv_mater_bg);

    lv_general_dev.lv_back_event = lv_meter_back_btn_event_handler;
    lv_general_dev.parent = lv_meter_cont;
    lv_general_dev.lv_general_win_create = lv_general_win_create;
    lv_general_dev.lv_general_win_create();

    lv_obj_t * lv_meter_cont_t = lv_obj_create(lv_meter_cont);
    lv_obj_set_size(lv_meter_cont_t, lv_obj_get_width(lv_scr_act()) - 80, lv_obj_get_height(lv_scr_act()) - 80);
    lv_obj_set_style_bg_color(lv_meter_cont_t,lv_color_make(166,66,66),LV_STATE_DEFAULT);
    lv_obj_clear_flag(lv_meter_cont_t, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(lv_meter_cont_t,60,LV_STATE_DEFAULT);
    lv_obj_center(lv_meter_cont_t);

    meter = lv_meter_create(lv_meter_cont_t);
    lv_obj_set_size(meter, 220, 220);
    lv_obj_set_style_border_width(meter,10,LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(meter, lv_color_make(191, 66, 60), LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(meter, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(meter, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(meter,lv_palette_main(LV_PALETTE_GREY),LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(meter, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_y(meter,2, LV_STATE_DEFAULT);
    lv_obj_align(meter,LV_ALIGN_LEFT_MID,0,0);

    lv_obj_t* lv_label1 = lv_label_create(lv_meter_cont_t);
    lv_label_set_text(lv_label1, meter_image[0].setting_app_text_English);
    lv_obj_set_style_text_font(lv_label1,&lv_font_montserrat_14,LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lv_label1,lv_color_make(255,255,255),LV_STATE_DEFAULT);
    lv_obj_align(lv_label1, LV_ALIGN_TOP_RIGHT,-160,30);

    lv_obj_t* lv_label2 = lv_label_create(lv_meter_cont_t);
    lv_label_set_text(lv_label2, meter_image[1].setting_app_text_English);
    lv_obj_set_style_text_font(lv_label2, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lv_label2, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
    lv_obj_align_to(lv_label2, lv_label1, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    lv_obj_t* lv_image = lv_img_create(lv_meter_cont_t);
    lv_img_set_src(lv_image, METER_IMAGE_GBK_PATH[1]);
    lv_obj_align_to(lv_image, meter, LV_ALIGN_OUT_RIGHT_MID, 50, 30);
    lv_obj_set_style_img_recolor_opa(lv_image, 255, LV_PART_MAIN);
    lv_obj_set_style_img_recolor(lv_image, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);

    lv_obj_t * lv_image_lable = lv_label_create(lv_meter_cont_t);
    lv_label_set_text(lv_image_lable, meter_image[2].setting_app_text_English);
    lv_obj_set_style_text_font(lv_image_lable, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lv_image_lable, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
    lv_obj_align_to(lv_image_lable, lv_image, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

    lv_obj_t* lv_image2 = lv_img_create(lv_meter_cont_t);
    lv_img_set_src(lv_image2, METER_IMAGE_GBK_PATH[2]);
    lv_obj_align_to(lv_image2, lv_image, LV_ALIGN_OUT_RIGHT_MID, +50, 0);
    lv_obj_set_style_img_recolor_opa(lv_image2, 255, LV_PART_MAIN);
    lv_obj_set_style_img_recolor(lv_image2, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);

    lv_obj_t* lv_image2_lable = lv_label_create(lv_meter_cont_t);
    lv_label_set_text(lv_image2_lable, meter_image[3].setting_app_text_English);
    lv_obj_set_style_text_font(lv_image2_lable, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lv_image2_lable, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
    lv_obj_align_to(lv_image2_lable, lv_image2, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

    lv_obj_t* line1 = lv_line_create(lv_meter_cont_t);
    lv_obj_set_size(line1, lv_font_montserrat_14.line_height*2 + lv_font_montserrat_14.line_height*3, lv_font_montserrat_14.line_height*2 + lv_font_montserrat_14.line_height*3);
    lv_obj_set_style_line_color(line1, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(line1, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(line1, true, LV_STATE_DEFAULT);
    lv_line_set_points(line1, line_points1, 4);
    lv_obj_align_to(line1, lv_label2, LV_ALIGN_OUT_RIGHT_MID, 0, -20);

    lv_obj_t* line2 = lv_line_create(lv_meter_cont_t);
    lv_obj_set_size(line2, lv_font_montserrat_14.line_height*2 + lv_font_montserrat_14.line_height*3, lv_font_montserrat_14.line_height*2 + lv_font_montserrat_14.line_height*3);
    lv_obj_set_style_line_color(line2, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(line2, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(line2, true, LV_STATE_DEFAULT);
    lv_line_set_points(line2, line_points2, 4);
    lv_obj_align_to(line2, lv_label2, LV_ALIGN_OUT_LEFT_MID, 40, -20);

    /* minute scale: 61 ticks over 360 degrees */
    lv_meter_scale_t* scale_min = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale_min, 61, 1, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_range(meter, scale_min, 0, 60, 360, 270);

    /* hour scale: visual only, major ticks */
    lv_meter_scale_t* scale_hour = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale_hour, 12, 0, 0, lv_palette_main(LV_PALETTE_GREY)); /* 12 ticks */
    lv_meter_set_scale_major_ticks(meter, scale_hour, 1, 2, 20, lv_color_black(), 10);       /* major ticks */
    lv_meter_set_scale_range(meter, scale_hour, 1, 12, 330, 300);                            /* [1..12] */

    /* needle images */
    indic_second = lv_meter_add_needle_img(meter, scale_min, &img_hand, 5, 5);
    indic_min = lv_meter_add_needle_img(meter, scale_min, &img_hand, 5, 5);
    indic_hour = lv_meter_add_needle_img(meter, scale_min, &img_hand, 5, 5);

    lv_timer_create(lv_meter_vlaue, 1000, NULL);
}
