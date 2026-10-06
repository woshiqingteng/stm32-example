/*
 * CherryUSB configuration for the ALIENTEK STM32F429 board (device only).
 *
 * Selected by the board port (this directory is added to the cherryusb target
 * include path). Derived from cherryusb_config_template.h, trimmed to the
 * device role + the MSC / CDC-ACM / audio classes actually built.
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include <stdio.h>

/* ================ USB common Configuration ================ */

#define CONFIG_USB_PRINTF(...) printf(__VA_ARGS__)

#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO
#endif

/* No D-Cache on this MCU: keep the "nocache" buffers in normal RAM. */
#define USB_NOCACHE_RAM_SECTION

#define CONFIG_USB_ALIGN_SIZE 4

/* ================= USB Device Stack Configuration ================ */

#ifndef CONFIG_USBDEV_REQUEST_BUFFER_LEN
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 512
#endif

#define CONFIG_USBDEV_ADVANCE_DESC

/* Storage class: non-blocking with the SCSI processing moved to a thread so
 * the SD/NOR/NAND I/O never runs inside the USB ISR. */
#define CONFIG_USBDEV_MSC_THREAD

#ifndef CONFIG_USBDEV_MSC_MAX_LUN
#define CONFIG_USBDEV_MSC_MAX_LUN 1
#endif

#ifndef CONFIG_USBDEV_MSC_MAX_BUFSIZE
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE 512
#endif

#ifndef CONFIG_USBDEV_MSC_PRIO
#define CONFIG_USBDEV_MSC_PRIO 4
#endif

#ifndef CONFIG_USBDEV_MSC_STACKSIZE
#define CONFIG_USBDEV_MSC_STACKSIZE 2048
#endif

/* SCSI INQUIRY vendor / product / revision strings (fixed-width fields). */
#ifndef CONFIG_USBDEV_MSC_MANUFACTURER_STRING
#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING "ALIENTEK"
#endif
#ifndef CONFIG_USBDEV_MSC_PRODUCT_STRING
#define CONFIG_USBDEV_MSC_PRODUCT_STRING "STM32F4 MSC"
#endif
#ifndef CONFIG_USBDEV_MSC_VERSION_STRING
#define CONFIG_USBDEV_MSC_VERSION_STRING "1.00"
#endif

/* ================ USB Device Port Configuration ================ */

#ifndef CONFIG_USBDEV_MAX_BUS
#define CONFIG_USBDEV_MAX_BUS 1
#endif

/* DWC2 device: PA11/PA12 (OTG_FS) is slave-only, so no DMA-mode descriptor. */
// #define CONFIG_USB_DWC2_DMA_ENABLE

/* Host build is not used, but the shared DWC2 glue (usb_glue_st.c) includes
 * usbh_core.h, which needs the host geometry macros at parse time. */
#define CONFIG_USBHOST_MAX_RHPORTS          1
#define CONFIG_USBHOST_MAX_EXTHUBS          1
#define CONFIG_USBHOST_MAX_EHPORTS          4
#define CONFIG_USBHOST_MAX_INTERFACES       8
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 2
#define CONFIG_USBHOST_MAX_ENDPOINTS        4
#define CONFIG_USBHOST_DEV_NAMELEN          16

#ifndef CONFIG_USBHOST_REQUEST_BUFFER_LEN
#define CONFIG_USBHOST_REQUEST_BUFFER_LEN 512
#endif
#ifndef CONFIG_USBHOST_MAX_BUS
#define CONFIG_USBHOST_MAX_BUS 1
#endif

#ifndef usb_phyaddr2ramaddr
#define usb_phyaddr2ramaddr(addr) (addr)
#endif

#ifndef usb_ramaddr2phyaddr
#define usb_ramaddr2phyaddr(addr) (addr)
#endif

#endif /* CHERRYUSB_CONFIG_H */
