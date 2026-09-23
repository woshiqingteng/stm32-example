/**
 * @file    audioplay.h
 * @brief   46_music WAV playlist/navigation (application layer).
 */

#ifndef APP_AUDIOPLAY_H
#define APP_AUDIOPLAY_H

#include <stdint.h>

#define AUDIO_MUSIC_DIR "0:/MUSIC"

/** @brief  Count the playable WAV files in a directory. */
uint16_t audioplay_get_tnum(const char *path);

/** @brief  Render the current track index (e.g. "3/12"). */
void audioplay_index_show(uint16_t index, uint16_t total);

/** @brief  Scan 0:/MUSIC and play all WAV tracks with KEY navigation. */
void audioplay_play(void);

#endif /* APP_AUDIOPLAY_H */
