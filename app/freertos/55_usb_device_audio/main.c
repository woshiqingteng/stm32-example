/**
 * @file    main.c
 * @brief   55_usb_device_audio (FreeRTOS + CherryUSB): USB Audio (UAC1) speaker.
 *          Audio streamed from the host is played through the ES8388 codec via
 *          SAI1 block A.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp.h"
#include "codec.h"
#include "sai.h"

#include "usbd_core.h"
#include "usbd_audio.h"

#include "FreeRTOS.h"
#include "task.h"

#define AUDIO_OUT_EP   0x01U

#define USBD_VID       0x0483U
#define USBD_PID       0x5730U
#define USBD_MAX_POWER 500U

#define AUDIO_FREQ       48000U
#define AUDIO_CHANNELS   2U
#define AUDIO_RES_BITS   16U
#define AUDIO_PKT_SIZE   ((AUDIO_FREQ / 1000U) * AUDIO_CHANNELS * (AUDIO_RES_BITS / 8U)) /* 192 */

#define AUDIO_MON_BUF_LEN  192U
#define AUDIO_TASK_STK_SIZE 512U
#define AUDIO_TASK_PRIO     2U

/* Entity IDs used in the descriptor. */
#define AUDIO_ENTITY_IT   1U
#define AUDIO_ENTITY_FU   2U
#define AUDIO_ENTITY_OT   3U

static struct audio_entity_info s_audio_entity_table[] = {
    { .bDescriptorSubtype = AUDIO_CONTROL_FEATURE_UNIT, .bEntityId = AUDIO_ENTITY_FU, .ep = AUDIO_OUT_EP },
    { .bDescriptorSubtype = AUDIO_CONTROL_OUTPUT_TERMINAL, .bEntityId = AUDIO_ENTITY_OT, .ep = AUDIO_OUT_EP },
};

/* ------------------------------------------------------------------ */
/* USB descriptors                                                     */
/* ------------------------------------------------------------------ */

#define AUDIO_AC_WTOTAL (AUDIO_AC_DESCRIPTOR_LEN(1) + 12U + 10U + 9U)
#define AUDIO_AS_TOTAL  AUDIO_AS_DESCRIPTOR_LEN(1)
#define USB_CONFIG_SIZE (9U + AUDIO_AC_WTOTAL + AUDIO_AS_TOTAL)

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
};

static const uint8_t config_descriptor_fs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    AUDIO_AC_DESCRIPTOR_INIT(0x00, 0x02, AUDIO_AC_WTOTAL, 0x00, 0x01),
    AUDIO_AC_INPUT_TERMINAL_DESCRIPTOR_INIT(AUDIO_ENTITY_IT, 0x0101, AUDIO_CHANNELS, 0x0003),
    AUDIO_AC_FEATURE_UNIT_DESCRIPTOR_INIT(AUDIO_ENTITY_FU, AUDIO_ENTITY_IT, 0x01, 0x03, 0x00, 0x00),
    AUDIO_AC_OUTPUT_TERMINAL_DESCRIPTOR_INIT(AUDIO_ENTITY_OT, 0x0301, AUDIO_ENTITY_FU),
    AUDIO_AS_DESCRIPTOR_INIT(0x01, AUDIO_ENTITY_IT, AUDIO_CHANNELS, AUDIO_RES_BITS / 8U, AUDIO_RES_BITS,
                             AUDIO_OUT_EP, 0x09, AUDIO_PKT_SIZE, 0x01,
                             (AUDIO_FREQ & 0xFFU), ((AUDIO_FREQ >> 8) & 0xFFU), ((AUDIO_FREQ >> 16) & 0xFFU)),
};

static const uint8_t device_quality_descriptor[] = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, 0x01),
};

static const uint8_t other_speed_config_descriptor_fs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    AUDIO_AC_DESCRIPTOR_INIT(0x00, 0x02, AUDIO_AC_WTOTAL, 0x00, 0x01),
    AUDIO_AC_INPUT_TERMINAL_DESCRIPTOR_INIT(AUDIO_ENTITY_IT, 0x0101, AUDIO_CHANNELS, 0x0003),
    AUDIO_AC_FEATURE_UNIT_DESCRIPTOR_INIT(AUDIO_ENTITY_FU, AUDIO_ENTITY_IT, 0x01, 0x03, 0x00, 0x00),
    AUDIO_AC_OUTPUT_TERMINAL_DESCRIPTOR_INIT(AUDIO_ENTITY_OT, 0x0301, AUDIO_ENTITY_FU),
    AUDIO_AS_DESCRIPTOR_INIT(0x01, AUDIO_ENTITY_IT, AUDIO_CHANNELS, AUDIO_RES_BITS / 8U, AUDIO_RES_BITS,
                             AUDIO_OUT_EP, 0x09, AUDIO_PKT_SIZE, 0x01,
                             (AUDIO_FREQ & 0xFFU), ((AUDIO_FREQ >> 8) & 0xFFU), ((AUDIO_FREQ >> 16) & 0xFFU)),
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
            return "ALIENTEK STM32F4 USB Audio";
        case 3U:
            (void)snprintf(s_serial, sizeof(s_serial), "%08lX%08lX",
                           (unsigned long)(*(const uint32_t *)0x1FFF7A10U),
                           (unsigned long)(*(const uint32_t *)0x1FFF7A14U));
            return s_serial;
        default:
            return NULL;
    }
}

static const struct usb_descriptor audio_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback = other_speed_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback,
};

/* ------------------------------------------------------------------ */
/* Audio playback                                                      */
/* ------------------------------------------------------------------ */

static volatile bool     g_connected;
static volatile bool     g_streaming;
static volatile uint8_t  g_volume = 70U;

static uint8_t s_usb_buf[AUDIO_PKT_SIZE] __attribute__((aligned(4)));
static uint8_t s_sai_buf[2][AUDIO_MON_BUF_LEN] __attribute__((aligned(4)));

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;

    switch (event)
    {
        case USBD_EVENT_RESET:
        case USBD_EVENT_DISCONNECTED:
            g_connected = false;
            g_streaming = false;
            break;
        case USBD_EVENT_CONFIGURED:
            g_connected = true;
            break;
        default:
            break;
    }
}

static void audio_out_ep_cb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    if (g_streaming && (nbytes > 0U))
    {
        uint32_t n = (nbytes > AUDIO_MON_BUF_LEN) ? AUDIO_MON_BUF_LEN : nbytes;

        (void)memcpy(s_sai_buf[0], s_usb_buf, n);
        sai1_tx_dma_set_inactive_buffer(s_sai_buf[0]);
    }

    (void)usbd_ep_start_read(busid, ep, s_usb_buf, AUDIO_PKT_SIZE);
}

static struct usbd_endpoint audio_out_ep = {
    .ep_addr = AUDIO_OUT_EP,
    .ep_cb = audio_out_ep_cb,
};

/* ---- audio class callbacks (weak overrides) ---- */

void usbd_audio_open(uint8_t busid, uint8_t intf)
{
    (void)busid;
    (void)intf;

    (void)codec_init();
    codec_adda_cfg(1U, 0U);      /* DAC on, ADC off */
    codec_output_cfg(1U, 1U);
    codec_hpvol_set((uint8_t)(g_volume * 0.3f));
    codec_spkvol_set((uint8_t)(g_volume * 0.3f));
    codec_sai_cfg(0U, 3U);       /* Philips I2S, 16-bit */

    sai1_saia_init(SAI_MODEMASTER_TX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_16);
    (void)sai1_samplerate_set(AUDIO_FREQ);
    sai1_tx_dma_init(s_sai_buf[0], s_sai_buf[1], AUDIO_MON_BUF_LEN / 2U, 1U);
    sai1_play_start();

    g_streaming = true;
    (void)usbd_ep_start_read(busid, AUDIO_OUT_EP, s_usb_buf, AUDIO_PKT_SIZE);
}

void usbd_audio_close(uint8_t busid, uint8_t intf)
{
    (void)busid;
    (void)intf;

    g_streaming = false;
    sai1_play_stop();
}

void usbd_audio_set_volume(uint8_t busid, uint8_t ep, uint8_t ch, int volume_db)
{
    (void)busid;
    (void)ep;
    (void)ch;

    /* Map [-100..0] dB to [0..100] %. */
    int32_t v = volume_db + 100;

    if (v < 0)
    {
        v = 0;
    }
    if (v > 100)
    {
        v = 100;
    }
    g_volume = (uint8_t)v;
    codec_hpvol_set((uint8_t)(g_volume * 0.3f));
    codec_spkvol_set((uint8_t)(g_volume * 0.3f));
}

int usbd_audio_get_volume(uint8_t busid, uint8_t ep, uint8_t ch)
{
    (void)busid;
    (void)ep;
    (void)ch;
    return (int)g_volume - 100;
}

void usbd_audio_set_mute(uint8_t busid, uint8_t ep, uint8_t ch, bool mute)
{
    (void)busid;
    (void)ep;
    (void)ch;

    codec_output_cfg(mute ? 0U : 1U, mute ? 0U : 1U);
}

bool usbd_audio_get_mute(uint8_t busid, uint8_t ep, uint8_t ch)
{
    (void)busid;
    (void)ep;
    (void)ch;
    return false;
}

void usbd_audio_set_sampling_freq(uint8_t busid, uint8_t ep, uint32_t sampling_freq)
{
    (void)busid;
    (void)ep;
    (void)sai1_samplerate_set(sampling_freq);
}

uint32_t usbd_audio_get_sampling_freq(uint8_t busid, uint8_t ep)
{
    (void)busid;
    (void)ep;
    return AUDIO_FREQ;
}

static const uint8_t s_sampling_freq_table[] = { 0x01, 0x00,
                                                 (AUDIO_FREQ & 0xFFU), ((AUDIO_FREQ >> 8) & 0xFFU), ((AUDIO_FREQ >> 16) & 0xFFU),
                                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void usbd_audio_get_sampling_freq_table(uint8_t busid, uint8_t ep, uint8_t **sampling_freq_table)
{
    (void)busid;
    (void)ep;
    *sampling_freq_table = (uint8_t *)s_sampling_freq_table;
}

/* ------------------------------------------------------------------ */
/* Monitor task / main                                                 */
/* ------------------------------------------------------------------ */

static struct usbd_interface intf0;
static struct usbd_interface intf1;

static void audio_monitor_task(void *argument)
{
    bool     connected = false;
    uint32_t blink = 0U;

    (void)argument;

    for (;;)
    {
        if (g_connected != connected)
        {
            connected = g_connected;
            printf(connected ? "USB Connected\r\n" : "USB DisConnected\r\n");
            connected ? led_on(LED1) : led_off(LED1);
        }

        if (++blink >= 10U)
        {
            blink = 0U;
            led_toggle(LED0);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

int main(void)
{
    bsp_init();
    /* USB OTG FS needs an exact 48 MHz kernel clock: 336 / 7 = 48 MHz while
     * keeping the core at 168 MHz. Must run before the scheduler starts. */
    (void)sys_clk_reconfig(336U, 25U, 2U, 7U);
    delay_init(168U);
    usart_init(&(usart_cfg_t){ USART_CFG_DEFAULT(USART_ID_1) });

    printf(APP_BANNER "\r\n");

    usbd_desc_register(0, &audio_descriptor);
    usbd_add_interface(0, usbd_audio_init_intf(0, &intf0, 0x0100, s_audio_entity_table,
                                               sizeof(s_audio_entity_table) / sizeof(s_audio_entity_table[0])));
    usbd_add_interface(0, usbd_audio_init_intf(0, &intf1, 0x0100, s_audio_entity_table,
                                               sizeof(s_audio_entity_table) / sizeof(s_audio_entity_table[0])));
    usbd_add_endpoint(0, &audio_out_ep);
    (void)usbd_initialize(0, USB_OTG_FS_PERIPH_BASE, usbd_event_handler);

    (void)xTaskCreate(audio_monitor_task, "audio", AUDIO_TASK_STK_SIZE, NULL, AUDIO_TASK_PRIO, NULL);

    vTaskStartScheduler();

    for (;;)
    {
    }
}
