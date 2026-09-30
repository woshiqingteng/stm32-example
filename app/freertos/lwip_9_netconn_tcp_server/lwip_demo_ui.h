/**
 * @file    lwip_demo_ui.h
 * @brief   Per-app lwIP demo UI scaffolding (KEY0 send, RX display, LED).
 *
 * Mirrors the ALIENTEK freertos_demo.c: a key task sets the send flag, a
 * display task drains a queue onto the LCD, and an LED task blinks.
 */

#ifndef LWIP_DEMO_UI_H
#define LWIP_DEMO_UI_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"

#define LWIP_SEND_DATA      0x80U
#define LWIP_DEMO_RX_X      30U
#define LWIP_DEMO_RX_Y      230U

extern QueueHandle_t    g_display_queue;
extern uint8_t          g_lwip_send_flag;
extern volatile uint8_t g_lwip_font_ok;   /* 1 when the GBK font store is valid */

/** @brief  Draw the fixed UI header and start the key/display/LED tasks. */
void lwip_demo_ui_start(const char *title);

/** @brief  Show the local IP (line 130). */
void lwip_demo_ui_ip(const char *ip);

/** @brief  Show the Ethernet speed (line 150). */
void lwip_demo_ui_speed(const char *speed);

/** @brief  Show the "Init failed / Retrying" state. */
void lwip_demo_ui_retry(void);

/** @brief  Set the connection state line (line 90). */
void lwip_demo_ui_state(const char *text, uint32_t color);

/** @brief  Draw one ASCII string (EN) at (x, y). */
void lwip_demo_ui_show(uint16_t x, uint16_t y, uint8_t size, const char *en, uint32_t color);

/** @brief  Draw one GBK string (CN) at (x, y); no-op without the font store. */
void lwip_demo_ui_show_cn(uint16_t x, uint16_t y, uint8_t size, const char *gbk, uint32_t color);

#endif /* LWIP_DEMO_UI_H */
