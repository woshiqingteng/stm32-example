/**
 * @file    recorder.h
 * @brief   WAV recorder state machine: captures the codec ADC through SAI RX
 *          into a small FIFO and writes it to 0:/RECORDER/RECxxxxx.wav.
 */

#ifndef LIB_AUDIO_RECORDER_H
#define LIB_AUDIO_RECORDER_H

#include <stdint.h>
#include "wavplay.h"    /* __WaveHeader */

#define REC_SAI_RX_DMA_BUF_SIZE     4096U   /* capture DMA half-buffer size (bytes) */
#define REC_SAI_RX_FIFO_SIZE        10U     /* capture FIFO depth (buffers) */
#define REC_SAMPLERATE              44100U  /* recording sample rate */

/** @brief  Read one buffer from the capture FIFO. @return 0 when empty. */
uint8_t recoder_sai_fifo_read(uint8_t **buf);

/** @brief  Push one buffer into the capture FIFO. @return 0 on success. */
uint8_t recoder_sai_fifo_write(uint8_t *buf);

/** @brief  DMA RX half-transfer callback (capture FIFO producer). */
void recoder_sai_dma_rx_callback(void);

/** @brief  Switch the codec + SAI to PCM record mode. */
void recoder_enter_rec_mode(void);

/** @brief  Switch the codec + SAI to playback mode. */
void recoder_enter_play_mode(void);

/** @brief  Fill a WAV header for the current record settings. */
void recoder_wav_init(__WaveHeader *wavhead);

/** @brief  Render the recorded time and bit rate. */
void recoder_msg_show(uint32_t tsec, uint32_t kbps);

/** @brief  Render the key hint for record (0) or playback (1) mode. */
void recoder_remindmsg_show(uint8_t mode);

/** @brief  Build the next free "0:RECORDER/RECxxxxx.wav" path. */
void recoder_new_pathname(char *pname);

/** @brief  Run the recorder (record / pause / save / play). */
void wav_recorder(void);

#endif /* LIB_AUDIO_RECORDER_H */
