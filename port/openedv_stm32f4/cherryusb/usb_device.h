/**
 * @file    usb_device.h
 * @brief   Shared CherryUSB device helpers for the board: the vendor id, the
 *          standard string descriptors and the connect/disconnect bookkeeping
 *          used by the FreeRTOS applications.
 */

#ifndef BSP_CHERRYUSB_USB_DEVICE_H
#define BSP_CHERRYUSB_USB_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

/** @brief  Board USB vendor id (STMicroelectronics). */
#define USBD_VID 0x0483U

/**
 * @brief  Shared string descriptor callback: index 0 (language id), 1
 *         (manufacturer) and 3 (serial number from the 96-bit unique id).
 *         Returns NULL for every other index - the application supplies the
 *         product string (index 2) itself.
 * @param  speed  the descriptor speed (unused)
 * @param  index  the string index
 * @return the string, or NULL when this helper does not own the index
 */
const char *usb_device_string_desc(uint8_t speed, uint8_t index);

/** @brief  Update the shared connect state from a device event handler. */
void usb_device_event(uint8_t event);

/** @brief  true after USBD_EVENT_CONFIGURED, until RESET / DISCONNECTED. */
bool usb_device_connected(void);

#endif /* BSP_CHERRYUSB_USB_DEVICE_H */
