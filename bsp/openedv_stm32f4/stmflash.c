/**
 * @file    stmflash.c
 * @brief   Internal STM32F429 flash access. Uses HAL_FLASH_* with the sector
 *          layout of the 1 MB STM32F429IG device (sectors 0-3 16 KB, 4 64 KB,
 *          5-11 128 KB, 12-15 16 KB, 16 64 KB, 17-23 128 KB).
 */

#include "stm32f4xx_hal.h"
#include "stmflash.h"

#define STMFLASH_TIMEOUT      50000U
#define STMFLASH_WORD_SIZE    4U

#define STMFLASH_SECTOR_COUNT 24U

static const uint32_t g_stmflash_sector_addr[STMFLASH_SECTOR_COUNT] =
{
    0x08000000U, 0x08004000U, 0x08008000U, 0x0800C000U,
    0x08010000U, 0x08020000U, 0x08040000U, 0x08060000U,
    0x08080000U, 0x080A0000U, 0x080C0000U, 0x080E0000U,
    0x08100000U, 0x08104000U, 0x08108000U, 0x0810C000U,
    0x08110000U, 0x08120000U, 0x08140000U, 0x08160000U,
    0x08180000U, 0x081A0000U, 0x081C0000U, 0x081E0000U,
};

static uint32_t stmflash_sector_of(uint32_t addr)
{
    uint32_t sector;

    for (sector = (STMFLASH_SECTOR_COUNT - 1U); sector > 0U; sector--)
    {
        if (addr >= g_stmflash_sector_addr[sector])
        {
            return sector;
        }
    }

    return 0U;
}

static bool stmflash_addr_valid(uint32_t addr)
{
    return (addr >= STMFLASH_BASE) && (addr < (STMFLASH_BASE + STMFLASH_SIZE));
}

uint32_t stmflash_read_word(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

bool stmflash_erase_sector(uint32_t addr)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t               sector_error = 0U;
    HAL_StatusTypeDef      status;

    if (!stmflash_addr_valid(addr))
    {
        return false;
    }

    HAL_FLASH_Unlock();

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.Sector       = stmflash_sector_of(addr);
    erase.NbSectors    = 1U;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    status             = HAL_FLASHEx_Erase(&erase, &sector_error);

    HAL_FLASH_Lock();

    return status == HAL_OK;
}

void stmflash_write_word(uint32_t addr, uint32_t data)
{
    if (!stmflash_addr_valid(addr) || ((addr % STMFLASH_WORD_SIZE) != 0U))
    {
        return;
    }

    HAL_FLASH_Unlock();
    (void)FLASH_WaitForLastOperation(STMFLASH_TIMEOUT);
    (void)HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, (uint64_t)data);
    HAL_FLASH_Lock();
}

void stmflash_write_halfword(uint32_t addr, uint16_t data)
{
    if (!stmflash_addr_valid(addr))
    {
        return;
    }

    HAL_FLASH_Unlock();
    (void)FLASH_WaitForLastOperation(STMFLASH_TIMEOUT);
    (void)HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, (uint64_t)data);
    HAL_FLASH_Lock();
}

void stmflash_read(uint32_t addr, uint32_t *buf, uint32_t words)
{
    uint32_t i;

    for (i = 0U; i < words; i++)
    {
        buf[i] = stmflash_read_word(addr + (i * STMFLASH_WORD_SIZE));
    }
}

void stmflash_write(uint32_t addr, const uint32_t *buf, uint32_t words)
{
    uint32_t i;

    if (words == 0U)
    {
        return;
    }

    if (!stmflash_addr_valid(addr) || ((addr % STMFLASH_WORD_SIZE) != 0U))
    {
        return;
    }

    if (!stmflash_addr_valid(addr + ((words - 1U) * STMFLASH_WORD_SIZE)))
    {
        return;
    }

    /* Erase the target sector once if it still holds programmed data. */
    for (i = 0U; i < words; i++)
    {
        if (stmflash_read_word(addr + (i * STMFLASH_WORD_SIZE)) != 0xFFFFFFFFU)
        {
            (void)stmflash_erase_sector(addr);
            break;
        }
    }

    HAL_FLASH_Unlock();

    for (i = 0U; i < words; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + (i * STMFLASH_WORD_SIZE),
                              (uint64_t)buf[i]) != HAL_OK)
        {
            break;
        }
    }

    HAL_FLASH_Lock();
}
