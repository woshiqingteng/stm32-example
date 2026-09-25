/**
 * @file    usbd_audio_if.h
 * @brief   USB device Audio class interface: speaker playback through the
 *          ES8388 codec / SAI1 plus a local microphone monitor loopback.
 */

#ifndef PORT_USBD_AUDIO_IF_H
#define PORT_USBD_AUDIO_IF_H

#include <stdint.h>
#include "usbd_audio.h"

extern USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops;

/** @brief  Current speaker volume (0..100). */
extern uint8_t g_audio_volume;

uint8_t BSP_AUDIO_OUT_Init(uint16_t output_device, uint8_t volume, uint32_t audio_freq);
uint8_t BSP_AUDIO_OUT_Play(uint16_t *buffer, uint32_t size);
void    BSP_AUDIO_OUT_ChangeBuffer(uint16_t *data, uint16_t size);
uint8_t BSP_AUDIO_OUT_Stop(uint32_t option);
uint8_t BSP_AUDIO_OUT_SetVolume(uint8_t volume);
uint8_t BSP_AUDIO_OUT_SetMute(uint32_t cmd);

#endif /* PORT_USBD_AUDIO_IF_H */
