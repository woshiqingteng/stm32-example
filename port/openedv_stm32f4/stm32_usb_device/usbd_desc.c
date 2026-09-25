/**
 * @file    usbd_desc.c
 * @brief   USB device descriptor sets for the MSC, CDC (VCP) and AUDIO device
 *          apps. Every set has its own device descriptor (the class bytes and
 *          the product ID differ) and shares the language ID, manufacturer and
 *          serial number callbacks.
 */

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_conf.h"

#define USBD_VID                  0x0483U
#define USBD_MSC_PID              0x5720U
#define USBD_VCP_PID              0x5740U
#define USBD_AUDIO_PID            0x5730U
#define USBD_LANGID_STRING        0x0409U
#define USBD_MANUFACTURER_STRING  "STMicroelectronics"

__ALIGN_BEGIN static uint8_t usbd_langid_desc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
    USB_LEN_LANGID_STR_DESC, USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID_STRING), HIBYTE(USBD_LANGID_STRING)
};

__ALIGN_BEGIN static uint8_t usbd_string_serial[USB_SIZ_STRING_SERIAL] __ALIGN_END = {
    USB_SIZ_STRING_SERIAL, USB_DESC_TYPE_STRING
};

__ALIGN_BEGIN static uint8_t usbd_str_desc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

static void usbd_int_to_unicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
    uint8_t idx;

    for (idx = 0U; idx < len; idx++)
    {
        if ((value >> 28) < 0xAU)
        {
            pbuf[2U * idx] = (uint8_t)((value >> 28) + '0');
        }
        else
        {
            pbuf[2U * idx] = (uint8_t)((value >> 28) + 'A' - 10U);
        }

        value <<= 4;
        pbuf[2U * idx + 1U] = 0U;
    }
}

static void usbd_get_serial_num(void)
{
    uint32_t serial0 = *(uint32_t *)DEVICE_ID1;
    uint32_t serial1 = *(uint32_t *)DEVICE_ID2;
    uint32_t serial2 = *(uint32_t *)DEVICE_ID3;

    serial0 += serial2;

    if (serial0 != 0U)
    {
        usbd_int_to_unicode(serial0, (uint8_t *)&usbd_string_serial[2], 8U);
        usbd_int_to_unicode(serial1, (uint8_t *)&usbd_string_serial[18], 4U);
    }
}

static uint8_t *usbd_get_string_desc(const char *str, uint16_t *length)
{
    USBD_GetString((uint8_t *)str, usbd_str_desc, length);
    return usbd_str_desc;
}

static uint8_t *usbd_langid_str_desc(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = USB_LEN_LANGID_STR_DESC;
    return usbd_langid_desc;
}

static uint8_t *usbd_manufacturer_str_desc(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    return usbd_get_string_desc(USBD_MANUFACTURER_STRING, length);
}

static uint8_t *usbd_serial_str_desc(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = USB_SIZ_STRING_SERIAL;
    usbd_get_serial_num();
    return usbd_string_serial;
}

/* Emit the four class specific callbacks plus the device descriptor buffer. */
#define DEFINE_USBD_PROFILE(prefix, pid, dev_class, dev_subclass, product_str, config_str, iface_str) \
    __ALIGN_BEGIN static uint8_t prefix##_device_desc_buf[USB_LEN_DEV_DESC] __ALIGN_END = {           \
        0x12, USB_DESC_TYPE_DEVICE, 0x00, 0x02,                                                       \
        (dev_class), (dev_subclass), 0x00, USB_MAX_EP0_SIZE,                                          \
        LOBYTE(USBD_VID), HIBYTE(USBD_VID),                                                           \
        LOBYTE(pid), HIBYTE(pid),                                                                     \
        0x00, 0x02,                                                                                   \
        USBD_IDX_MFC_STR, USBD_IDX_PRODUCT_STR, USBD_IDX_SERIAL_STR,                                  \
        USBD_MAX_NUM_CONFIGURATION                                                                    \
    };                                                                                                \
    static uint8_t *prefix##_device_desc(USBD_SpeedTypeDef speed, uint16_t *length)                   \
    {                                                                                                 \
        (void)speed;                                                                                  \
        *length = sizeof(prefix##_device_desc_buf);                                                   \
        return prefix##_device_desc_buf;                                                              \
    }                                                                                                 \
    static uint8_t *prefix##_product_desc(USBD_SpeedTypeDef speed, uint16_t *length)                  \
    {                                                                                                 \
        (void)speed;                                                                                  \
        return usbd_get_string_desc(product_str, length);                                             \
    }                                                                                                 \
    static uint8_t *prefix##_config_desc(USBD_SpeedTypeDef speed, uint16_t *length)                   \
    {                                                                                                 \
        (void)speed;                                                                                  \
        return usbd_get_string_desc(config_str, length);                                              \
    }                                                                                                 \
    static uint8_t *prefix##_interface_desc(USBD_SpeedTypeDef speed, uint16_t *length)                \
    {                                                                                                 \
        (void)speed;                                                                                  \
        return usbd_get_string_desc(iface_str, length);                                               \
    }

DEFINE_USBD_PROFILE(msc, USBD_MSC_PID, 0x00U, 0x00U,
                    "ALIENTEK STM32F4 Mass Storage", "MSC Config", "MSC Interface")

DEFINE_USBD_PROFILE(vcp, USBD_VCP_PID, 0x02U, 0x02U,
                    "ALIENTEK STM32F4 Virtual COM", "VCP Config", "VCP Interface")

DEFINE_USBD_PROFILE(audio, USBD_AUDIO_PID, 0x00U, 0x00U,
                    "ALIENTEK STM32F4 USB Audio", "AUDIO Config", "AUDIO Interface")

USBD_DescriptorsTypeDef MSC_Desc = {
    msc_device_desc, usbd_langid_str_desc, usbd_manufacturer_str_desc,
    msc_product_desc, usbd_serial_str_desc, msc_config_desc, msc_interface_desc
};

USBD_DescriptorsTypeDef VCP_Desc = {
    vcp_device_desc, usbd_langid_str_desc, usbd_manufacturer_str_desc,
    vcp_product_desc, usbd_serial_str_desc, vcp_config_desc, vcp_interface_desc
};

USBD_DescriptorsTypeDef AUDIO_Desc = {
    audio_device_desc, usbd_langid_str_desc, usbd_manufacturer_str_desc,
    audio_product_desc, usbd_serial_str_desc, audio_config_desc, audio_interface_desc
};
