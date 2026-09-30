/**
 * @file    lwip_demo_ui.c
 * @brief   Per-app lwIP demo UI scaffolding.
 */

#include "lwip_demo_ui.h"

#include "bsp.h"
#include "lcd.h"
#include "text.h"

#define UI_KEY_PRIO     5
#define UI_DISP_PRIO    5
#define UI_LED_PRIO     4

#define UI_KEY_STK      256
#define UI_DISP_STK     512
#define UI_LED_STK      128

#define UI_Q_LEN        20
#define UI_Q_ITEM       200

QueueHandle_t    g_display_queue;
uint8_t          g_lwip_send_flag;
volatile uint8_t g_lwip_font_ok;

static void ui_key_task(void *arg)
{
    (void)arg;

    for (;;)
    {
        if (key_scan(false) == KEY0)
        {
            g_lwip_send_flag |= LWIP_SEND_DATA;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void ui_display_task(void *arg)
{
    char line[UI_Q_ITEM];

    (void)arg;

    for (;;)
    {
        if (xQueueReceive(g_display_queue, line, portMAX_DELAY) == pdTRUE)
        {
            lcd_fill(LWIP_DEMO_RX_X, LWIP_DEMO_RX_Y, LCD_WIDTH_PX - 5, LCD_HEIGHT_PX - 5, WHITE);
            lcd_show_string(LWIP_DEMO_RX_X, LWIP_DEMO_RX_Y, LCD_WIDTH_PX - LWIP_DEMO_RX_X - 5,
                            16, LCD_FONT_SIZE_16, line, RED);
        }
    }
}

static void ui_led_task(void *arg)
{
    (void)arg;

    for (;;)
    {
        led_toggle(LED0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void lwip_demo_ui_start(const char *title)
{
    g_display_queue = xQueueCreate(UI_Q_LEN, UI_Q_ITEM);

    lcd_show_string(6, 10, 200, 32, LCD_FONT_SIZE_32, "STM32", DARKBLUE);
    lcd_show_string(6, 40, LCD_WIDTH_PX, 24, LCD_FONT_SIZE_24, title, DARKBLUE);
    lcd_show_string(6, 70, 200, 16, LCD_FONT_SIZE_16, "ATOM@ALIENTEK", DARKBLUE);
    lcd_show_string(5, 110, 200, 16, LCD_FONT_SIZE_16, "lwIP Init Successed", MAGENTA);
    lcd_show_string(5, 170, 200, 16, LCD_FONT_SIZE_16, "KEY0:Send data", MAGENTA);
    lcd_show_string(5, 190, LCD_WIDTH_PX - 30, LCD_HEIGHT_PX - 190,
                    LCD_FONT_SIZE_16, "Receive Data:", BLUE);

    xTaskCreate(ui_key_task, "key", UI_KEY_STK, NULL, UI_KEY_PRIO, NULL);
    xTaskCreate(ui_display_task, "disp", UI_DISP_STK, NULL, UI_DISP_PRIO, NULL);
    xTaskCreate(ui_led_task, "led", UI_LED_STK, NULL, UI_LED_PRIO, NULL);
}

void lwip_demo_ui_ip(const char *ip)
{
    lcd_show_string(5, 130, 200, 16, LCD_FONT_SIZE_16, ip, MAGENTA);
}

void lwip_demo_ui_speed(const char *speed)
{
    lcd_show_string(5, 150, 200, 16, LCD_FONT_SIZE_16, speed, MAGENTA);
}

void lwip_demo_ui_retry(void)
{
    lcd_show_string(30, 110, 200, 16, LCD_FONT_SIZE_16, "lwIP Init failed!!", RED);
}

void lwip_demo_ui_state(const char *text, uint32_t color)
{
    lcd_fill(5, 90, LCD_WIDTH_PX - 5, 106, WHITE);
    lcd_show_string(5, 90, LCD_WIDTH_PX - 10, 16, LCD_FONT_SIZE_16, text, color);
}

void lwip_demo_ui_show(uint16_t x, uint16_t y, uint8_t size, const char *en, uint32_t color)
{
    lcd_show_string(x, y, LCD_WIDTH_PX - x - 5, size, (lcd_font_size_t)size, en, color);
}

void lwip_demo_ui_show_cn(uint16_t x, uint16_t y, uint8_t size, const char *gbk, uint32_t color)
{
    if (g_lwip_font_ok)
    {
        text_show_string(x, y, LCD_WIDTH_PX - x - 5, size, (char *)gbk, size, 1U, color);
    }
}
