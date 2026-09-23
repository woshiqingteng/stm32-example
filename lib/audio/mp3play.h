/**
 * @file    mp3play.h
 * @brief   MP3 playback using the Helix fixed-point decoder over ES8388 + SAI.
 */

#ifndef LIB_AUDIO_MP3PLAY_H
#define LIB_AUDIO_MP3PLAY_H

#include <stdint.h>
#include "audio.h"

/** @brief  Play one MP3 file. @return AUDIO_* navigation code. */
audio_nav_t mp3_play_song(char *fname);

#endif /* LIB_AUDIO_MP3PLAY_H */
