/**
 * @file    can.h
 * @brief   bxCAN1 driver (PA11 RX / PA12 TX), ported from the vendor example.
 */

#ifndef BSP_CAN_H
#define BSP_CAN_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/** @brief Callback invoked for every received standard data frame. */
typedef void (*can_rx_hook_t)(uint32_t id, const uint8_t *data, uint8_t len);

/**
 * @brief  Initialise and start CAN1.
 * @param  tsjw sync jump width (CAN_SJW_xTQ)
 * @param  tbs2 time segment 2 (CAN_BS2_xTQ)
 * @param  tbs1 time segment 1 (CAN_BS1_xTQ)
 * @param  brp  baud-rate prescaler (1..1024)
 * @param  mode CAN_MODE_NORMAL or CAN_MODE_LOOPBACK
 * @return 0 on success, non-zero on the failing step
 */
uint8_t can_init(uint32_t tsjw, uint32_t tbs2, uint32_t tbs1, uint16_t brp, uint32_t mode);

/** @brief  Queue a standard data frame. @return 0 on success. */
uint8_t can_send(uint32_t id, const uint8_t *msg, uint8_t len);

/**
 * @brief  Poll FIFO0 for a standard data frame with matching @p id.
 * @return received length, or 0 when nothing matched
 */
uint8_t can_receive(uint32_t id, uint8_t *buf);

/** @brief  Register (or clear with 0) the receive hook invoked by can_receive(). */
void can_register_rx_hook(can_rx_hook_t cb);

#endif /* BSP_CAN_H */
