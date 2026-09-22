/**
 * @file    sai.h
 * @brief   SAI1 audio interface: block A for playback (TX) and block B for
 *          capture (RX), both driven by DMA2 in double-buffer mode.
 *
 * The ES8388 codec is clocked from the SAI1 master clock (PE2) and exchanges
 * I2S data on PE6 (SD-A, playback) and PE3 (SD-B, capture).
 */

#ifndef BSP_SAI_H
#define BSP_SAI_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/* SAI1 pin mapping. */
#define SAI1_CLK_GPIO_PORT              GPIOE
#define SAI1_CLK_GPIO_PIN               GPIO_PIN_2
#define SAI1_CLK_GPIO_CLK_ENABLE()      do { __HAL_RCC_GPIOE_CLK_ENABLE(); } while (0)

#define SAI1_SCK_GPIO_PORT              GPIOE
#define SAI1_SCK_GPIO_PIN               GPIO_PIN_5
#define SAI1_SCK_GPIO_CLK_ENABLE()      do { __HAL_RCC_GPIOE_CLK_ENABLE(); } while (0)

#define SAI1_FSA_GPIO_PORT              GPIOE
#define SAI1_FSA_GPIO_PIN               GPIO_PIN_4
#define SAI1_FSA_GPIO_CLK_ENABLE()      do { __HAL_RCC_GPIOE_CLK_ENABLE(); } while (0)

#define SAI1_SDA_GPIO_PORT              GPIOE
#define SAI1_SDA_GPIO_PIN               GPIO_PIN_6
#define SAI1_SDA_GPIO_CLK_ENABLE()      do { __HAL_RCC_GPIOE_CLK_ENABLE(); } while (0)

#define SAI1_SDB_GPIO_PORT              GPIOE
#define SAI1_SDB_GPIO_PIN               GPIO_PIN_3
#define SAI1_SDB_GPIO_CLK_ENABLE()      do { __HAL_RCC_GPIOE_CLK_ENABLE(); } while (0)

#define SAI1_SAI_CLK_ENABLE()           do { __HAL_RCC_SAI1_CLK_ENABLE(); } while (0)

/* Playback DMA: DMA2 stream 3, channel 0. */
#define SAI1_TX_DMASx                   DMA2_Stream3
#define SAI1_TX_DMASx_Channel           DMA_CHANNEL_0
#define SAI1_TX_DMASx_IRQHandler        DMA2_Stream3_IRQHandler
#define SAI1_TX_DMASx_IRQ               DMA2_Stream3_IRQn
#define SAI1_TX_DMASx_FLAG              DMA_FLAG_TCIF3_7
#define SAI1_TX_DMA_CLK_ENABLE()        do { __HAL_RCC_DMA2_CLK_ENABLE(); } while (0)

/* Capture DMA: DMA2 stream 5, channel 0. */
#define SAI1_RX_DMASx                   DMA2_Stream5
#define SAI1_RX_DMASx_Channel           DMA_CHANNEL_0
#define SAI1_RX_DMASx_IRQHandler        DMA2_Stream5_IRQHandler
#define SAI1_RX_DMASx_IRQ               DMA2_Stream5_IRQn
#define SAI1_RX_DMASx_FLAG              DMA_FLAG_TCIF1_5
#define SAI1_RX_DMA_CLK_ENABLE()        do { __HAL_RCC_DMA2_CLK_ENABLE(); } while (0)

extern SAI_HandleTypeDef g_sai1_a_handle;     /* SAI1 block A (playback) */
extern SAI_HandleTypeDef g_sai1_b_handle;     /* SAI1 block B (capture)  */
extern DMA_HandleTypeDef g_sai1_tx_dma_handle; /* playback DMA */
extern DMA_HandleTypeDef g_sai1_rx_dma_handle; /* capture DMA  */

extern void (*sai_tx_callback)(void);         /* playback DMA transfer callback */
extern void (*sai_rx_callback)(void);         /* capture DMA transfer callback  */

/** @brief  SAI1 TX DMA double-buffer target: non-zero when M1AR is active,
 *          zero when M0AR is active (DMA_SxCR_CT). */
uint8_t sai1_tx_dma_target(void);

/** @brief  SAI1 RX DMA double-buffer target (DMA_SxCR_CT). */
uint8_t sai1_rx_dma_target(void);

/** @brief  Programme the inactive SAI1 TX double-buffer area with @p buf. */
void    sai1_tx_dma_set_inactive_buffer(uint8_t *buf);

/** @brief  Disable the SAI1 TX DMA transfer-complete interrupt. Safe to call
 *          before sai1_tx_dma_init() (targets DMA2_Stream3 directly). */
void    sai1_tx_dma_irq_disable(void);

void    sai1_saia_init(uint8_t mode, uint8_t cpol, uint8_t datalen);
void    sai1_saib_init(uint8_t mode, uint8_t cpol, uint8_t datalen);
uint8_t sai1_samplerate_set(uint32_t samplerate);
void    sai1_tx_dma_init(uint8_t *buf0, uint8_t *buf1, uint16_t num, uint8_t width);
void    sai1_rx_dma_init(uint8_t *buf0, uint8_t *buf1, uint16_t num, uint8_t width);
void    sai1_saia_dma_enable(void);
void    sai1_saib_dma_enable(void);
void    sai1_play_start(void);
void    sai1_play_stop(void);
void    sai1_rec_start(void);
void    sai1_rec_stop(void);

#endif /* BSP_SAI_H */
