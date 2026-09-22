/**
 * @file    touch.h
 * @brief   GT9147 capacitive touch panel driver over a dedicated software IIC
 *          bus (CT_SCL = PH6, CT_SDA = PI3, RST = PI8, INT = PH7).
 */

#ifndef BSP_TOUCH_H
#define BSP_TOUCH_H

#include <stdint.h>

/** @brief  Maximum simultaneous touches reported by the driver. */
#define TOUCH_MAX_POINTS    5U

/** @brief  touch_dev_t.type bit set when the panel is capacitive. */
#define TOUCH_TYPE_CAPACITIVE 0x80U

/** @brief  Touch controller state. */
typedef struct
{
    uint8_t  type;                     /*!< panel capability flags */
    uint8_t  pressed;                  /*!< non-zero while a finger is down */
    uint16_t x[TOUCH_MAX_POINTS];      /*!< logical X coordinates */
    uint16_t y[TOUCH_MAX_POINTS];      /*!< logical Y coordinates */
} touch_dev_t;

extern touch_dev_t g_touch;

/** @brief  Reset and probe the GT9147.
 *  @return 0 on success, 1 if the product id is not recognised. */
uint8_t touch_init(void);

/**
 * @brief  Poll the controller.
 * @param  mode 0: return logical coordinates; non-zero: keep raw coordinates.
 * @return 1 while the panel is touched, 0 otherwise.
 */
uint8_t touch_scan(uint8_t mode);

/** @brief  Copy the first touch point into @p x and @p y. */
void touch_read_xy(uint16_t *x, uint16_t *y);

/** @brief  True while the last scan reported a touch. */
uint8_t touch_pressed(void);

#endif /* BSP_TOUCH_H */
