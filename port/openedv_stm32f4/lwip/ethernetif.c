/**
 * @file    ethernetif.c
 * @brief   lwIP netif driver for the STM32F4 ETH MAC (zero-copy, FreeRTOS).
 *
 * Derived from the ST/ALIENTEK LwIP_HTTP_Server example: Rx buffers are lwIP
 * pbufs handed directly to the ETH DMA, Tx pbufs are passed straight to the MAC
 * with checksum/CRC offload.
 */

#include <stddef.h>
#include <string.h>

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/timeouts.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "lwip_comm.h"
#include "ethernetif.h"
#include "eth.h"
#include "eth_phy.h"

#define TIME_WAITING_FOR_INPUT       (portMAX_DELAY)
#define INTERFACE_THREAD_STACK_SIZE  (512)
#define NETIF_IN_TASK_PRIORITY       (2)

#define IFNAME0                      'e'
#define IFNAME1                      'n'

#define ETH_RX_BUFFER_SIZE           1524
#define ETH_RX_BUFFER_CNT            10

typedef enum
{
    RX_ALLOC_OK    = 0,
    RX_ALLOC_ERROR = 1
} RxAllocStatusTypeDef;

typedef struct
{
    struct pbuf_custom pbuf_custom;
    uint8_t buff[(ETH_RX_BUFFER_SIZE + 31) & ~31] __attribute__((aligned(32)));
} RxBuff_t;

LWIP_MEMPOOL_DECLARE(RX_POOL, ETH_RX_BUFFER_CNT, sizeof(RxBuff_t), "Zero-copy RX pool");

static uint8_t RxAllocStatus;

static SemaphoreHandle_t s_tx_semaphore;
static SemaphoreHandle_t s_rx_semaphore;

static ETH_TxPacketConfigTypeDef s_tx_config;
static volatile struct netif *s_netif;

static void   ethernetif_input(void *argument);
static void   rmii_watchdog(void *argument);
static void   pbuf_free_custom(struct pbuf *p);
static struct pbuf *low_level_input(void);

static void low_level_init(struct netif *netif)
{
    uint32_t duplex = ETH_FULLDUPLEX_MODE;
    uint32_t speed  = ETH_SPEED_100M;
    ETH_MACConfigTypeDef macconf = {0};

    eth_init(g_lwipdev.mac);

    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    memcpy(netif->hwaddr, g_lwipdev.mac, 6);
    netif->mtu = 1500;
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    LWIP_MEMPOOL_INIT(RX_POOL);

    memset(&s_tx_config, 0, sizeof(s_tx_config));
    s_tx_config.Attributes   = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
    s_tx_config.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
    s_tx_config.CRCPadCtrl   = ETH_CRC_PAD_INSERT;

    s_rx_semaphore = xSemaphoreCreateBinary();
    s_tx_semaphore = xSemaphoreCreateBinary();

    sys_thread_new("eth_rx", ethernetif_input, netif,
                   INTERFACE_THREAD_STACK_SIZE, NETIF_IN_TASK_PRIORITY);

    if (eth_phy_link_up())
    {
        speed  = (eth_phy_speed() == ETH_PHY_SPEED_100M) ? ETH_SPEED_100M : ETH_SPEED_10M;
        duplex = eth_phy_full_duplex() ? ETH_FULLDUPLEX_MODE : ETH_HALFDUPLEX_MODE;
        g_lwipdev.link_up = 1U;
    }

    HAL_ETH_GetMACConfig(&g_eth_handle, &macconf);
    macconf.DuplexMode = duplex;
    macconf.Speed = speed;
    HAL_ETH_SetMACConfig(&g_eth_handle, &macconf);
    HAL_ETH_Start_IT(&g_eth_handle);

    if (HAL_GetREVID() == 0x1000U)
    {
        sys_thread_new("eth_rmii", rmii_watchdog, NULL,
                       configMINIMAL_STACK_SIZE, NETIF_IN_TASK_PRIORITY + 1);
    }
}

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    uint32_t i = 0;
    struct pbuf *q;
    ETH_BufferTypeDef txbuffer[ETH_TX_DESC_CNT] = {0};

    (void)netif;

    for (q = p; q != NULL; q = q->next)
    {
        if (i >= ETH_TX_DESC_CNT)
        {
            return ERR_IF;
        }

        txbuffer[i].buffer = q->payload;
        txbuffer[i].len    = q->len;

        if (i > 0)
        {
            txbuffer[i - 1].next = &txbuffer[i];
        }

        i++;
    }

    s_tx_config.Length   = p->tot_len;
    s_tx_config.TxBuffer = txbuffer;
    s_tx_config.pData    = p;

    pbuf_ref(p);
    HAL_ETH_Transmit_IT(&g_eth_handle, &s_tx_config);

    while (xSemaphoreTake(s_tx_semaphore, TIME_WAITING_FOR_INPUT) != pdTRUE)
    {
    }

    HAL_ETH_ReleaseTxPacket(&g_eth_handle);

    return ERR_OK;
}

static struct pbuf *low_level_input(void)
{
    struct pbuf *p = NULL;

    if (RxAllocStatus == RX_ALLOC_OK)
    {
        HAL_ETH_ReadData(&g_eth_handle, (void **)&p);
    }

    return p;
}

static void ethernetif_input(void *argument)
{
    struct netif *netif = (struct netif *)argument;
    struct pbuf *p;

    s_netif = netif;

    for (;;)
    {
        if (xSemaphoreTake(s_rx_semaphore, TIME_WAITING_FOR_INPUT) == pdTRUE)
        {
            do
            {
                p = low_level_input();
                if (p != NULL)
                {
                    if (netif->input(p, netif) != ERR_OK)
                    {
                        pbuf_free(p);
                    }
                }
            } while (p != NULL);
        }
    }
}

err_t ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    netif->output     = etharp_output;
    netif->linkoutput = low_level_output;

    low_level_init(netif);

    return ERR_OK;
}

static void pbuf_free_custom(struct pbuf *p)
{
    struct pbuf_custom *custom = (struct pbuf_custom *)p;
    BaseType_t woken = pdFALSE;

    LWIP_MEMPOOL_FREE(RX_POOL, custom);

    if (RxAllocStatus == RX_ALLOC_ERROR)
    {
        RxAllocStatus = RX_ALLOC_OK;
        if (xSemaphoreGiveFromISR(s_rx_semaphore, &woken) == pdTRUE)
        {
            portYIELD_FROM_ISR(woken);
        }
    }
}

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
    BaseType_t woken = pdFALSE;
    (void)heth;

    if (xSemaphoreGiveFromISR(s_rx_semaphore, &woken) == pdTRUE)
    {
        portYIELD_FROM_ISR(woken);
    }
}

void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth)
{
    BaseType_t woken = pdFALSE;
    (void)heth;

    if (xSemaphoreGiveFromISR(s_tx_semaphore, &woken) == pdTRUE)
    {
        portYIELD_FROM_ISR(woken);
    }
}

void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth)
{
    BaseType_t woken = pdFALSE;

    if ((HAL_ETH_GetDMAError(heth) & ETH_DMASR_RBUS) == ETH_DMASR_RBUS)
    {
        if (xSemaphoreGiveFromISR(s_rx_semaphore, &woken) == pdTRUE)
        {
            portYIELD_FROM_ISR(woken);
        }
    }
}

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
    struct pbuf_custom *p = LWIP_MEMPOOL_ALLOC(RX_POOL);

    if (p != NULL)
    {
        *buff = (uint8_t *)p + offsetof(RxBuff_t, buff);
        p->custom_free_function = pbuf_free_custom;
        pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, p, *buff, ETH_RX_BUFFER_SIZE);
    }
    else
    {
        RxAllocStatus = RX_ALLOC_ERROR;
        *buff = NULL;
    }
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
    struct pbuf **ppStart = (struct pbuf **)pStart;
    struct pbuf **ppEnd   = (struct pbuf **)pEnd;
    struct pbuf *p;

    p = (struct pbuf *)(buff - offsetof(RxBuff_t, buff));
    p->next = NULL;
    p->tot_len = 0;
    p->len = Length;

    if (*ppStart == NULL)
    {
        *ppStart = p;
    }
    else
    {
        (*ppEnd)->next = p;
    }
    *ppEnd = p;

    for (p = *ppStart; p != NULL; p = p->next)
    {
        p->tot_len += Length;
    }
}

void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
    pbuf_free((struct pbuf *)buff);
}

static void rmii_watchdog(void *argument)
{
    (void)argument;

    for (;;)
    {
        if (g_eth_handle.Instance->MMCRGUFCR > 0U)
        {
            vTaskDelete(NULL);
        }
        else if (g_eth_handle.Instance->MMCRFCECR > 10U)
        {
            SYSCFG->PMC &= ~SYSCFG_PMC_MII_RMII_SEL;
            SYSCFG->PMC |=  SYSCFG_PMC_MII_RMII_SEL;
            g_eth_handle.Instance->MMCCR |= ETH_MMCCR_CR;
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}
