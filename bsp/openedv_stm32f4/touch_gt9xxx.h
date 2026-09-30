/**
 * @file    touch_gt9xxx.h
 * @brief   GT9xxx capacitive touch controller over a dedicated bit-bang IIC
 *          bus (CT_SCL = PH6, CT_SDA = PI3, RST = PI8, INT = PH7).
 */

#ifndef BSP_TOUCH_GT9XXX_H
#define BSP_TOUCH_GT9XXX_H

#include <stdbool.h>
#include <stdint.h>

/** @brief  Maximum simultaneous points the controller reports. */
#define TOUCH_GT9XXX_MAX_POINT_COUNT 5U

/** @brief  Reset, probe and configure the controller.
 *  @return 0 on success, 1 if the product id is not recognised. */
uint8_t touch_gt9xxx_init(void);

/**
 * @brief  Read raw (unmapped) touch points.
 * @param  x,y        Output arrays with room for @p max_points entries.
 * @param  max_points Capacity of @p x / @p y.
 * @param  count      Receives the number of valid points (<= max_points).
 * @return true while the panel is touched.
 */
bool touch_gt9xxx_read(uint16_t *x, uint16_t *y, uint8_t max_points, uint8_t *count);

#endif /* BSP_TOUCH_GT9XXX_H */
