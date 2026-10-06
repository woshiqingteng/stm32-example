/**
 * @file    main.c
 * @brief   54_usb_device_msc (FreeRTOS + CherryUSB): USB Mass Storage backed by
 *          three logical units (SPI NOR, NAND via the FTL and the SD card). The
 *          USB connection and read/write activity are reported on USART1.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bsp.h"
#include "sdio.h"
#include "nor.h"
#include "ftl.h"
#include "nand.h"

#include "usbd_core.h"
#include "usbd_msc.h"

#include "FreeRTOS.h"
#include "task.h"

#define MSC_IN_EP    0x81U
#define MSC_OUT_EP   0x01U

#define USBD_VID       0x0483U
#define USBD_PID       0x5720U
#define USBD_MAX_POWER 500U

#define MSC_TASK_STK_SIZE  512U
#define MSC_TASK_PRIO      2U

/* Logical units. */
#define LUN_NOR   0U
#define LUN_NAND  1U
#define LUN_SD    2U

#define STORAGE_BLK_SIZE 512U

/* ------------------------------------------------------------------ */
/* USB descriptors                                                     */
/* ------------------------------------------------------------------ */

#define USB_CONFIG_SIZE (9U + MSC_DESCRIPTOR_LEN)

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0100, 0x01),
};

static const uint8_t config_descriptor_fs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    MSC_DESCRIPTOR_INIT(0x00, MSC_OUT_EP, MSC_IN_EP, USB_BULK_EP_MPS_FS, 0x00),
};

static const uint8_t device_quality_descriptor[] = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, 0x01),
};

static const uint8_t other_speed_config_descriptor_fs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    MSC_DESCRIPTOR_INIT(0x00, MSC_OUT_EP, MSC_IN_EP, USB_BULK_EP_MPS_FS, 0x00),
};

static const char s_langid[] = { (char)0x09, (char)0x04 };
static char s_serial[25];

static const uint8_t *device_descriptor_callback(uint8_t speed) { (void)speed; return device_descriptor; }
static const uint8_t *config_descriptor_callback(uint8_t speed) { (void)speed; return config_descriptor_fs; }
static const uint8_t *device_quality_descriptor_callback(uint8_t speed) { (void)speed; return device_quality_descriptor; }
static const uint8_t *other_speed_descriptor_callback(uint8_t speed) { (void)speed; return other_speed_config_descriptor_fs; }

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;

    switch (index)
    {
        case 0U:
            return s_langid;
        case 1U:
            return "STMicroelectronics";
        case 2U:
            return "ALIENTEK STM32F4 Mass Storage";
        case 3U:
            (void)snprintf(s_serial, sizeof(s_serial), "%08lX%08lX",
                           (unsigned long)(*(const uint32_t *)0x1FFF7A10U),
                           (unsigned long)(*(const uint32_t *)0x1FFF7A14U));
            return s_serial;
        default:
            return NULL;
    }
}

static const struct usb_descriptor msc_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback = other_speed_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback,
};

/* ------------------------------------------------------------------ */
/* MSC block-device callbacks                                          */
/* ------------------------------------------------------------------ */

typedef enum { ACT_IDLE, ACT_READING, ACT_WRITING } activity_t;

static volatile bool       g_connected;
static volatile activity_t g_activity = ACT_IDLE;
static volatile bool       g_error;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;

    switch (event)
    {
        case USBD_EVENT_RESET:
        case USBD_EVENT_DISCONNECTED:
            g_connected = false;
            break;
        case USBD_EVENT_CONFIGURED:
            g_connected = true;
            break;
        default:
            break;
    }
}

void usbd_msc_get_cap(uint8_t busid, uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
    (void)busid;

    *block_size = STORAGE_BLK_SIZE;

    if (lun == LUN_NOR)
    {
        *block_num = NOR_FATFS_SECTORS;
    }
    else if (lun == LUN_NAND)
    {
        *block_num = (uint32_t)nand_dev.valid_blocknum * nand_dev.block_pagenum *
                     nand_dev.page_mainsize / STORAGE_BLK_SIZE;
    }
    else
    {
        sd_card_info_t info;

        sdio_get_card_info(&info);
        *block_num = info.block_count;
    }
}

int usbd_msc_sector_read(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    (void)busid;

    g_activity = ACT_READING;

    if (lun == LUN_NOR)
    {
        uint32_t off = 0U;

        while (off < length)
        {
            uint16_t chunk = (uint16_t)((length - off) > STORAGE_BLK_SIZE ? STORAGE_BLK_SIZE : (length - off));

            nor_read(buffer + off, (sector * STORAGE_BLK_SIZE) + off, chunk);
            off += chunk;
        }
        return 0;
    }

    if (lun == LUN_NAND)
    {
        if (ftl_read_sectors(buffer, sector, STORAGE_BLK_SIZE, length / STORAGE_BLK_SIZE) != 0U)
        {
            g_error = true;
            return -1;
        }
        return 0;
    }

    if (sd_read_disk(buffer, sector, length / STORAGE_BLK_SIZE) != 0U)
    {
        g_error = true;
        return -1;
    }
    return 0;
}

int usbd_msc_sector_write(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    (void)busid;

    g_activity = ACT_WRITING;

    if (lun == LUN_NOR)
    {
        uint32_t off = 0U;

        while (off < length)
        {
            uint16_t chunk = (uint16_t)((length - off) > STORAGE_BLK_SIZE ? STORAGE_BLK_SIZE : (length - off));

            nor_write(buffer + off, (sector * STORAGE_BLK_SIZE) + off, chunk);
            off += chunk;
        }
        return 0;
    }

    if (lun == LUN_NAND)
    {
        if (ftl_write_sectors(buffer, sector, STORAGE_BLK_SIZE, length / STORAGE_BLK_SIZE) != 0U)
        {
            g_error = true;
            return -1;
        }
        return 0;
    }

    if (sd_write_disk(buffer, sector, length / STORAGE_BLK_SIZE) != 0U)
    {
        g_error = true;
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Monitor task                                                        */
/* ------------------------------------------------------------------ */

static void msc_monitor_task(void *argument)
{
    bool       connected = false;
    activity_t activity  = ACT_IDLE;

    (void)argument;

    for (;;)
    {
        if (g_connected != connected)
        {
            connected = g_connected;
            printf(connected ? "USB Connected\r\n" : "USB DisConnected\r\n");
            connected ? led_on(LED1) : led_off(LED1);
        }

        if (activity != g_activity)
        {
            activity = g_activity;

            if (activity == ACT_WRITING)
            {
                printf("USB Writing...\r\n");
            }
            else if (activity == ACT_READING)
            {
                printf("USB Reading...\r\n");
            }
        }

        if (g_error)
        {
            g_error = false;
            printf("USB storage error\r\n");
        }

        led_toggle(LED0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

static void storage_init(void)
{
    sd_card_info_t info;

    if (sdio_init() == 0U)
    {
        sdio_get_card_info(&info);
        printf("SD Card Size: %lu MB\r\n", (unsigned long)info.total_size_mb);
    }
    else
    {
        printf("SD init failed\r\n");
    }

    /* Bring up every backing medium before USB starts so the MSC enumeration
     * never has to scan/format them. */
    nor_init();
    (void)ftl_init();

    if ((uint32_t)nand_dev.valid_blocknum < ((uint32_t)nand_dev.good_blocknum * 90U / 100U))
    {
        printf("formatting NAND to full capacity...\r\n");
        (void)ftl_format();
    }
}

static struct usbd_interface intf0;

int main(void)
{
    bsp_init();
    /* USB OTG FS needs an exact 48 MHz kernel clock: 336 / 7 = 48 MHz while
     * keeping the core at 168 MHz. Must run before the scheduler starts. */
    (void)sys_clk_reconfig(336U, 25U, 2U, 7U);
    delay_init(168U);
    usart_init(&(usart_cfg_t){ USART_CFG_DEFAULT(USART_ID_1) });

    printf(APP_BANNER "\r\n");

    storage_init();

    usbd_desc_register(0, &msc_descriptor);
    /* usbd_msc_init_intf() registers the bulk endpoints itself. */
    usbd_add_interface(0, usbd_msc_init_intf(0, &intf0, MSC_OUT_EP, MSC_IN_EP));
    (void)usbd_initialize(0, USB_OTG_FS_PERIPH_BASE, usbd_event_handler);

    (void)xTaskCreate(msc_monitor_task, "msc", MSC_TASK_STK_SIZE, NULL, MSC_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
