/**
 * @file    can.c
 * @brief   bxCAN1 driver on PA11 (RX) / PA12 (TX), AF9. MSP content is inlined
 *          into can_init(). Reception is polled; the optional hook is invoked
 *          from can_receive() for each accepted frame.
 */

#include "stm32f4xx_hal.h"
#include "can.h"

#define CAN_RX_GPIO_PORT    GPIOA
#define CAN_RX_GPIO_PIN     GPIO_PIN_11
#define CAN_TX_GPIO_PORT    GPIOA
#define CAN_TX_GPIO_PIN     GPIO_PIN_12
#define CAN_GPIO_AF         GPIO_AF9_CAN1

#define CAN_FILTER_BANK     0U
#define CAN_SLAVE_START_BANK 14U

static CAN_HandleTypeDef    g_can_handle;
static CAN_TxHeaderTypeDef  g_can_tx_header;
static CAN_RxHeaderTypeDef  g_can_rx_header;
static can_rx_hook_t        g_can_rx_hook;

uint8_t can_init(uint32_t tsjw, uint32_t tbs2, uint32_t tbs1, uint16_t brp, uint32_t mode)
{
    GPIO_InitTypeDef  gpio_init = {0};
    CAN_FilterTypeDef filter    = {0};

    /* ---- MSP begin: CAN1 clock + PA11/PA12 ---- */
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_init.Pin       = CAN_RX_GPIO_PIN | CAN_TX_GPIO_PIN;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = CAN_GPIO_AF;
    HAL_GPIO_Init(CAN_RX_GPIO_PORT, &gpio_init);
    /* ---- MSP end ---- */

    g_can_handle.Instance                  = CAN1;
    g_can_handle.Init.Prescaler            = brp;
    g_can_handle.Init.Mode                 = mode;
    g_can_handle.Init.SyncJumpWidth        = tsjw;
    g_can_handle.Init.TimeSeg1             = tbs1;
    g_can_handle.Init.TimeSeg2             = tbs2;
    g_can_handle.Init.TimeTriggeredMode    = DISABLE;
    g_can_handle.Init.AutoBusOff           = DISABLE;
    g_can_handle.Init.AutoWakeUp           = DISABLE;
    g_can_handle.Init.AutoRetransmission   = ENABLE;
    g_can_handle.Init.ReceiveFifoLocked    = DISABLE;
    g_can_handle.Init.TransmitFifoPriority = DISABLE;

    if (HAL_CAN_Init(&g_can_handle) != HAL_OK)
    {
        return 1U;
    }

    filter.FilterIdHigh         = 0x0000U;
    filter.FilterIdLow          = 0x0000U;
    filter.FilterMaskIdHigh     = 0x0000U;
    filter.FilterMaskIdLow      = 0x0000U;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterBank           = CAN_FILTER_BANK;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterActivation     = CAN_FILTER_ENABLE;
    filter.SlaveStartFilterBank = CAN_SLAVE_START_BANK;

    if (HAL_CAN_ConfigFilter(&g_can_handle, &filter) != HAL_OK)
    {
        return 2U;
    }

    if (HAL_CAN_Start(&g_can_handle) != HAL_OK)
    {
        return 3U;
    }

    return 0U;
}

uint8_t can_send(uint32_t id, const uint8_t *msg, uint8_t len)
{
    uint32_t mailbox = CAN_TX_MAILBOX0;

    g_can_tx_header.StdId = id;
    g_can_tx_header.ExtId = 0U;
    g_can_tx_header.IDE   = CAN_ID_STD;
    g_can_tx_header.RTR   = CAN_RTR_DATA;
    g_can_tx_header.DLC   = len;

    if (HAL_CAN_AddTxMessage(&g_can_handle, &g_can_tx_header, (uint8_t *)msg, &mailbox) != HAL_OK)
    {
        return 1U;
    }

    return 0U;
}

uint8_t can_receive(uint32_t id, uint8_t *buf)
{
    if (HAL_CAN_GetRxFifoFillLevel(&g_can_handle, CAN_RX_FIFO0) == 0U)
    {
        return 0U;
    }

    if (HAL_CAN_GetRxMessage(&g_can_handle, CAN_RX_FIFO0, &g_can_rx_header, buf) != HAL_OK)
    {
        return 0U;
    }

    if ((g_can_rx_header.StdId != id) || (g_can_rx_header.IDE != CAN_ID_STD) ||
        (g_can_rx_header.RTR != CAN_RTR_DATA))
    {
        return 0U;
    }

    if (g_can_rx_hook != 0)
    {
        g_can_rx_hook(g_can_rx_header.StdId, buf, (uint8_t)g_can_rx_header.DLC);
    }

    return (uint8_t)g_can_rx_header.DLC;
}

void can_register_rx_hook(can_rx_hook_t cb)
{
    g_can_rx_hook = cb;
}
