/**
 * @file    iap.c
 * @brief   In-application programming over USART1 (STM32F4 internal flash).
 */

#include "stm32f4xx_hal.h"
#include "iap.h"
#include "usart.h"
#include "sys.h"

#define IAP_FRAME_HEAD0        0x5AU
#define IAP_FRAME_HEAD1        0xA5U
#define IAP_RX_TIMEOUT_MS      5000U
#define IAP_MAX_IMAGE_SIZE_BYTE     (960U * 1024U)

/* STM32F429IG sector layout (1 MB): 0-3 = 16K, 4 = 64K, 5-11 = 128K. */
#define IAP_SECTOR_SIZE_16K_BYTE         0x4000U
#define IAP_SECTOR_SIZE_64K_BYTE         0x10000U
#define IAP_SECTOR_SIZE_128K_BYTE        0x20000U

static uint8_t g_iap_buf[2048];

static uint32_t iap_sector_of(uint32_t addr)
{
    uint32_t off = addr - FLASH_BASE;

    if (off < (4U * IAP_SECTOR_SIZE_16K_BYTE))
    {
        return off / IAP_SECTOR_SIZE_16K_BYTE;
    }
    if (off < (4U * IAP_SECTOR_SIZE_16K_BYTE) + IAP_SECTOR_SIZE_64K_BYTE)
    {
        return 4U;
    }
    return 5U + ((off - (4U * IAP_SECTOR_SIZE_16K_BYTE) - IAP_SECTOR_SIZE_64K_BYTE) / IAP_SECTOR_SIZE_128K_BYTE);
}

static int iap_rx_byte(uint8_t *byte)
{
    return (usart_read(USART_ID_1, byte, 1U, IAP_RX_TIMEOUT_MS) == 1U) ? 0 : -1;
}

iap_status_t iap_erase_app(uint32_t addr, uint32_t len)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t               sector_error = 0U;
    uint32_t               end = addr + len;

    if ((addr < IAP_APP_ADDR) || (end > (IAP_APP_ADDR + IAP_MAX_IMAGE_SIZE_BYTE)))
    {
        return IAP_ERR_PARAM;
    }

    HAL_FLASH_Unlock();

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.Sector = iap_sector_of(addr);
    erase.NbSectors = iap_sector_of(end - 1U) - erase.Sector + 1U;

    if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return IAP_ERR_ERASE;
    }

    HAL_FLASH_Lock();
    return IAP_OK;
}

iap_status_t iap_write_appbin(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t end = addr + len;
    uint32_t i;

    if ((addr < IAP_APP_ADDR) || (end > (IAP_APP_ADDR + IAP_MAX_IMAGE_SIZE_BYTE)))
    {
        return IAP_ERR_PARAM;
    }

    HAL_FLASH_Unlock();

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

static void iap_stop_systick(void)
{
    /* Stop the kernel tick before handing control to the application. */
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;
}

void iap_jump(uint32_t addr)
{
    uint32_t stack_top;
    uint32_t reset_vec;
    void (*app_reset)(void);

    stack_top = *(volatile uint32_t *)addr;
    reset_vec = *(volatile uint32_t *)(addr + 4U);

    /* Valid image: stack top inside SRAM, reset vector inside flash. */
    if ((stack_top < 0x20000000U) || (stack_top > 0x20030000U) ||
        ((reset_vec & 0xFF000000U) != 0x08000000U))
    {
        return;
    }

    sys_intx_disable();
    iap_stop_systick();
    sys_set_vector_table(addr);
    __set_MSP(stack_top);

    app_reset = (void (*)(void))reset_vec;
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
    uint8_t  drop;

    /* Drop any stale bytes so the frame starts from a clean input. */
    while (usart_read(USART_ID_1, &drop, 1U, 0U) == 1U)
    {
    }

    if (iap_rx_byte(&head0) != 0) { return IAP_ERR_TIMEOUT; }
    if (head0 != IAP_FRAME_HEAD0) { return IAP_ERR_FRAME; }
    if (iap_rx_byte(&head1) != 0) { return IAP_ERR_TIMEOUT; }
    if (head1 != IAP_FRAME_HEAD1) { return IAP_ERR_FRAME; }

    if ((iap_rx_byte(&byte) != 0)) { return IAP_ERR_TIMEOUT; }
    len = byte;
    if ((iap_rx_byte(&byte) != 0)) { return IAP_ERR_TIMEOUT; }
    len |= (uint16_t)((uint16_t)byte << 8);

    if ((len == 0U) || (len > IAP_MAX_IMAGE_SIZE_BYTE)) { return IAP_ERR_PARAM; }

    if (iap_erase_app(IAP_APP_ADDR, len) != IAP_OK) { return IAP_ERR_ERASE; }

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
