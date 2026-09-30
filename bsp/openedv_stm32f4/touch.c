/**
 * @file    touch.c
 * @brief   Capacitive touch panel: device-independent point/state layer.
 *
 * Coordinates come from the controller layer (touch_gt9xxx.h) in raw panel
 * space and are mapped here to the logical display orientation.
 */

#include "lcd.h"
#include "touch.h"
#include "touch_gt9xxx.h"

touch_dev_t g_touch;

static void touch_map_raw(uint8_t idx, uint16_t raw_x, uint16_t raw_y)
{
    if (lcd_info()->dir == LCD_DIR_LANDSCAPE)
    {
        g_touch.x[idx] = raw_x;
        g_touch.y[idx] = raw_y;
    }
    else
    {
        g_touch.x[idx] = (uint16_t)(lcd_info()->width - raw_y);
        g_touch.y[idx] = raw_x;
    }
}

uint8_t touch_init(void)
{
    uint8_t ret = touch_gt9xxx_init();

    g_touch.type = TOUCH_TYPE_CAPACITIVE;
    g_touch.pressed = false;
    g_touch.count = 0U;

    return ret;
}

bool touch_scan(bool mode)
{
    uint16_t raw_x[TOUCH_MAX_POINT_COUNT];
    uint16_t raw_y[TOUCH_MAX_POINT_COUNT];
    uint8_t  count = 0U;
    uint8_t  i;

    if (touch_gt9xxx_read(raw_x, raw_y, TOUCH_MAX_POINT_COUNT, &count))
    {
        for (i = 0U; i < count; i++)
        {
            if (mode)
            {
                g_touch.x[i] = raw_x[i];
                g_touch.y[i] = raw_y[i];
            }
            else
            {
                touch_map_raw(i, raw_x[i], raw_y[i]);
            }
        }

        g_touch.pressed = true;
        g_touch.count = count;
        return true;
    }

    g_touch.pressed = false;
    g_touch.count = 0U;
    return false;
}

void touch_read_xy(uint16_t *x, uint16_t *y)
{
    if (x != 0)
    {
        *x = g_touch.x[0];
    }

    if (y != 0)
    {
        *y = g_touch.y[0];
    }
}

bool touch_pressed(void)
{
    return g_touch.pressed;
}
