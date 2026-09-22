/**
 * @file    iap.c
 * @brief   In-application programming over USART1 (STM32F4 internal flash).
 */

#include "stm32f4xx_hal.h"
#include "iap.h"
#include "usart.h"
#include "sys.h"

#define IAP_FLASH_SECTOR_SIZE  (128U * 1024U) /* 0x08010000..0x080FFFFF: 128K sectors */
#define IAP_FRAME_HEAD0        0x5AU
#define IAP_FRAME_HEAD1        0xA5U
#define IAP_RX_TIMEOUT_MS      5000U
#define IAP_MAX_IMAGE_SIZE     (960U * 1024U)

static uint8_t g_iap_buf[2048];

static int iap_rx_byte(uint8_t *byte)
{
    return (HAL_UART_Receive(&g_uart1_handle, byte, 1U, IAP_RX_TIMEOUT_MS) == HAL_OK) ? 0 : -1;
}

iap_status_t iap_write_appbin(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t               sector_error = 0U;
    uint32_t               end = addr + len;
    uint32_t               i;

    if ((addr < IAP_APP_ADDR) || (end > (IAP_APP_ADDR + IAP_MAX_IMAGE_SIZE)))
    {
        return IAP_ERR_PARAM;
    }

    HAL_FLASH_Unlock();

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = (end - addr + IAP_FLASH_SECTOR_SIZE - 1U) / IAP_FLASH_SECTOR_SIZE;
    erase.Sector = (addr - FLASH_BASE) / IAP_FLASH_SECTOR_SIZE;

    if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return IAP_ERR_ERASE;
    }

    for (i = 0U; i < len; i += 2U)
    {
        uint16_t half = (uint16_t)buf[i];
        if ((i + 1U) < len)
        {
            half |= (uint16_t)((uint16_t)buf[i + 1U] << 8);
        }
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, half) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return IAP_ERR_WRITE;
        }
    }

    HAL_FLASH_Lock();
    return IAP_OK;
}

void iap_jump(uint32_t addr)
{
    uint32_t jump_addr;
    void (*app_reset)(void);

    if (((*(volatile uint32_t *)addr) & 0xFF000000U) != 0x08000000U)
    {
        return;
    }

    sys_intx_disable();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    sys_set_vector_table(addr);
    __set_MSP(*(volatile uint32_t *)addr);

    jump_addr = *(volatile uint32_t *)(addr + 4U);
    app_reset = (void (*)(void))jump_addr;
    sys_intx_enable();
    app_reset();
}

iap_status_t iap_receive_usart(void)
{
    uint8_t  byte;
    uint8_t  head0 = 0U;
    uint8_t  head1 = 0U;
    uint16_t len;
    uint32_t i;
    uint8_t  sum = 0U;

    (void)HAL_UART_AbortReceive(&g_uart1_handle);

    if (iap_rx_byte(&head0) != 0) { return IAP_ERR_TIMEOUT; }
    if (head0 != IAP_FRAME_HEAD0) { return IAP_ERR_FRAME; }
    if (iap_rx_byte(&head1) != 0) { return IAP_ERR_TIMEOUT; }
    if (head1 != IAP_FRAME_HEAD1) { return IAP_ERR_FRAME; }

    if ((iap_rx_byte(&byte) != 0)) { return IAP_ERR_TIMEOUT; }
    len = byte;
    if ((iap_rx_byte(&byte) != 0)) { return IAP_ERR_TIMEOUT; }
    len |= (uint16_t)((uint16_t)byte << 8);

    if ((len == 0U) || (len > IAP_MAX_IMAGE_SIZE)) { return IAP_ERR_PARAM; }

    for (i = 0U; i < len; i += sizeof(g_iap_buf))
    {
        uint32_t chunk = len - i;
        uint32_t k;

        if (chunk > sizeof(g_iap_buf))
        {
            chunk = sizeof(g_iap_buf);
        }
        for (k = 0U; k < chunk; k++)
        {
            if (iap_rx_byte(&g_iap_buf[k]) != 0) { return IAP_ERR_TIMEOUT; }
            sum = (uint8_t)(sum + g_iap_buf[k]);
        }
        if (iap_write_appbin(IAP_APP_ADDR + i, g_iap_buf, chunk) != IAP_OK)
        {
            return IAP_ERR_WRITE;
        }
    }

    if (iap_rx_byte(&byte) != 0) { return IAP_ERR_TIMEOUT; }
    if (byte != sum) { return IAP_ERR_FRAME; }

    return IAP_OK;
}
