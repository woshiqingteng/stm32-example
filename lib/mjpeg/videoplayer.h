/**
 * @file    videoplayer.h
 * @brief   MJPEG / AVI video player with synchronised ES8388 + SAI audio.
 */

#ifndef LIB_MJPEG_VIDEOPLAYER_H
#define LIB_MJPEG_VIDEOPLAYER_H

#include <stdint.h>
#include "ff.h"
#include "avi.h"

#define AVI_AUDIO_BUF_SIZE  (1024 * 5)      /* audio half-buffer size (bytes) */
#define AVI_AUDIO_BUF_NUM   4               /* audio ring depth */
#define AVI_VIDEO_BUF_SIZE  (1024 * 60)     /* video chunk read buffer (bytes) */

/** @brief  Count the playable AVI files in a directory. */
uint16_t video_get_tnum(char *path);

/** @brief  Render the elapsed / total playback time. */
void video_time_show(FIL *favi, AVI_INFO *aviinfo);

/** @brief  Render the video stream information. */
void video_info_show(AVI_INFO *aviinfo);

/** @brief  Render the file name and index. */
void video_bmsg_show(char *name, uint16_t index, uint16_t total);

/** @brief  Scan 0:/VIDEO and play every AVI file with KEY navigation. */
void video_play(void);

/** @brief  Play one AVI file. @return AUDIO_* navigation code. */
uint8_t video_play_mjpeg(char *pname);

/** @brief  Seek within the current AVI file. */
uint8_t video_seek(FIL *favi, AVI_INFO *aviinfo, uint8_t *mbuf);

#endif /* LIB_MJPEG_VIDEOPLAYER_H */
