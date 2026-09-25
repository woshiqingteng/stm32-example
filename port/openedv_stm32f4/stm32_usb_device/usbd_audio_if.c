/**
 * @file    usbd_audio_if.c
 * @brief   USB device Audio class interface. The host streams 16-bit stereo
 *          PCM to the speaker (SAI1 block A -> ES8388 DAC). SAI1 block B is
 *          also set up as a capture path and, while no USB stream is running,
 *          the microphone is looped straight back to the speaker.
 */

#include <string.h>

#include "usbd_audio_if.h"
#include "usbd_audio.h"
#include "es8388.h"
#include "sai.h"

#define AUDIO_MIC_BUF_SIZE   (AUDIO_TOTAL_BUF_SIZE / 2U)

extern USBD_HandleTypeDef USBD_Device;

uint8_t g_audio_volume = 0U;

/* Microphone capture buffers, reused as the local loopback source. */
static uint8_t  g_audio_mic_buf[2][AUDIO_MIC_BUF_SIZE];
static uint8_t  g_audio_streaming = 0U;

static int8_t Audio_Itf_Init(uint32_t audio_freq, uint32_t volume, uint32_t options);
static int8_t Audio_Itf_DeInit(uint32_t options);
static int8_t Audio_Itf_AudioCmd(uint8_t *pbuf, uint32_t size, uint8_t cmd);
static int8_t Audio_Itf_VolumeCtl(uint8_t vol);
static int8_t Audio_Itf_MuteCtl(uint8_t cmd);
static int8_t Audio_Itf_PeriodicTC(uint8_t *pbuf, uint32_t size, uint8_t cmd);
static int8_t Audio_Itf_GetState(void);

static void audio_local_loopback_start(void);
static void audio_sai_tx_callback(void);
static void audio_sai_rx_callback(void);

USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops = {
    Audio_Itf_Init,
    Audio_Itf_DeInit,
    Audio_Itf_AudioCmd,
    Audio_Itf_VolumeCtl,
    Audio_Itf_MuteCtl,
    Audio_Itf_PeriodicTC,
    Audio_Itf_GetState,
};

static int8_t Audio_Itf_Init(uint32_t audio_freq, uint32_t volume, uint32_t options)
{
    (void)options;

    (void)BSP_AUDIO_OUT_Init(0U, (uint8_t)volume, audio_freq);

    g_audio_streaming = 0U;
    audio_local_loopback_start();

    return (int8_t)USBD_OK;
}

static int8_t Audio_Itf_DeInit(uint32_t options)
{
    (void)options;

    g_audio_streaming = 0U;
    audio_local_loopback_start();

    return (int8_t)USBD_OK;
}

static int8_t Audio_Itf_AudioCmd(uint8_t *pbuf, uint32_t size, uint8_t cmd)
{
    switch (cmd)
    {
        case AUDIO_CMD_START:
            g_audio_streaming = 1U;
            (void)BSP_AUDIO_OUT_Play((uint16_t *)pbuf, size);
            break;

        case AUDIO_CMD_PLAY:
            BSP_AUDIO_OUT_ChangeBuffer((uint16_t *)pbuf, (uint16_t)size);
            break;

        case AUDIO_CMD_STOP:
            (void)BSP_AUDIO_OUT_Stop(AUDIO_CMD_STOP);
            g_audio_streaming = 0U;
            audio_local_loopback_start();
            break;

        default:
            break;
    }

    return (int8_t)USBD_OK;
}

static int8_t Audio_Itf_VolumeCtl(uint8_t vol)
{
    (void)BSP_AUDIO_OUT_SetVolume(vol);
    return (int8_t)USBD_OK;
}

static int8_t Audio_Itf_MuteCtl(uint8_t cmd)
{
    (void)BSP_AUDIO_OUT_SetMute(cmd);
    return (int8_t)USBD_OK;
}

static int8_t Audio_Itf_PeriodicTC(uint8_t *pbuf, uint32_t size, uint8_t cmd)
{
    (void)pbuf;
    (void)size;
    (void)cmd;
    return (int8_t)USBD_OK;
}

static int8_t Audio_Itf_GetState(void)
{
    return (int8_t)USBD_OK;
}

static void audio_local_loopback_start(void)
{
    sai1_tx_dma_init(g_audio_mic_buf[0], g_audio_mic_buf[1],
                     AUDIO_MIC_BUF_SIZE / 2U, 1U);
    CLEAR_BIT(g_sai1_tx_dma_handle.Instance->CR, DMA_SxCR_CIRC); /* single transfer */
    CLEAR_BIT(g_sai1_tx_dma_handle.Instance->CR, DMA_SxCR_DBM);  /* single buffer   */
    sai1_play_start();
}

static void audio_sai_tx_callback(void)
{
    if (g_audio_streaming != 0U)
    {
        USBD_AUDIO_Sync(&USBD_Device, AUDIO_OFFSET_FULL);
    }
}

static void audio_sai_rx_callback(void)
{
    /* The capture DMA writes the same buffers the loopback TX reads, so no
     * action is required here. */
}

uint8_t BSP_AUDIO_OUT_Init(uint16_t output_device, uint8_t volume, uint32_t audio_freq)
{
    (void)output_device;

    (void)es8388_init();
    es8388_adda_cfg(1U, 1U);        /* DAC and ADC on      */
    es8388_output_cfg(1U, 1U);      /* both outputs on     */
    es8388_input_cfg(0U);           /* channel 1 (MIC)     */
    es8388_mic_gain(8U);            /* maximum MIC gain    */
    es8388_hpvol_set(25U);
    es8388_spkvol_set(15U);
    es8388_sai_cfg(0U, 3U);         /* Philips I2S, 16-bit */

    sai1_saia_init(SAI_MODEMASTER_TX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_16);
    sai1_saib_init(SAI_MODESLAVE_RX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_16);
    (void)sai1_samplerate_set(audio_freq);

    sai1_rx_dma_init(g_audio_mic_buf[0], g_audio_mic_buf[1],
                     AUDIO_MIC_BUF_SIZE / 2U, 1U);

    sai_tx_callback = audio_sai_tx_callback;
    sai_rx_callback = audio_sai_rx_callback;

    sai1_rec_start();

    (void)BSP_AUDIO_OUT_SetVolume(volume);

    return 0U;
}

uint8_t BSP_AUDIO_OUT_Play(uint16_t *buffer, uint32_t size)
{
    sai1_tx_dma_init((uint8_t *)buffer, 0, (uint16_t)(size / 2U), 1U);
    CLEAR_BIT(g_sai1_tx_dma_handle.Instance->CR, DMA_SxCR_CIRC); /* single transfer */
    CLEAR_BIT(g_sai1_tx_dma_handle.Instance->CR, DMA_SxCR_DBM);  /* single buffer   */
    sai1_play_start();

    return 0U;
}

void BSP_AUDIO_OUT_ChangeBuffer(uint16_t *data, uint16_t size)
{
    (void)data;

    __HAL_DMA_DISABLE(&g_sai1_tx_dma_handle);   /* stop the DMA stream */
    while ((g_sai1_tx_dma_handle.Instance->CR & DMA_SxCR_EN) != 0U)
    {
        /* wait until it can be reprogrammed */
    }

    __HAL_DMA_SET_COUNTER(&g_sai1_tx_dma_handle, size);
    __HAL_DMA_ENABLE(&g_sai1_tx_dma_handle);    /* restart             */
}

uint8_t BSP_AUDIO_OUT_Stop(uint32_t option)
{
    (void)option;
    sai1_play_stop();
    return 0U;
}

uint8_t BSP_AUDIO_OUT_SetVolume(uint8_t volume)
{
    g_audio_volume = volume;
    es8388_hpvol_set((uint8_t)(volume * 0.3f));
    es8388_spkvol_set((uint8_t)(volume * 0.3f));
    return 0U;
}

uint8_t BSP_AUDIO_OUT_SetMute(uint32_t cmd)
{
    if (cmd != 0U)
    {
        es8388_output_cfg(0U, 0U);      /* mute   */
    }
    else
    {
        es8388_output_cfg(1U, 1U);      /* unmute */
    }

    return 0U;
}
