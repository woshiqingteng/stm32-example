/**
 * @file    norflash.c
 * @brief   W25Qxx SPI NOR flash driver, ported from the vendor NORFLASH example
 *          and re-based onto the shared spi driver (SPI_BUS_NORFLASH, CS = PF6).
 */

#include "stm32f4xx_hal.h"
#include "spi.h"
#include "norflash.h"
#include "delay.h"

#define NORFLASH_CS_HIGH()  HAL_GPIO_WritePin(NORFLASH_CS_GPIO_PORT, NORFLASH_CS_GPIO_PIN, GPIO_PIN_SET)
#define NORFLASH_CS_LOW()   HAL_GPIO_WritePin(NORFLASH_CS_GPIO_PORT, NORFLASH_CS_GPIO_PIN, GPIO_PIN_RESET)

/* Command set. */
#define FLASH_WriteEnable       0x06U
#define FLASH_ReadStatusReg1    0x05U
#define FLASH_ReadStatusReg2    0x35U
#define FLASH_ReadStatusReg3    0x15U
#define FLASH_WriteStatusReg3   0x11U
#define FLASH_ReadData          0x03U
#define FLASH_PageProgram       0x02U
#define FLASH_SectorErase       0x20U
#define FLASH_ManufactDeviceID  0x90U
#define FLASH_Enable4ByteAddr   0xB7U

#define FLASH_SR3_ADP_BIT       0x02U
#define FLASH_SR1_BUSY_BIT      0x01U

uint16_t g_norflash_type = BY25Q256;

static uint8_t g_norflash_buf[NORFLASH_SECTOR_SIZE];

static uint8_t norflash_spi_rw(uint8_t data)
{
    return spi_read_write_byte(SPI_BUS_NORFLASH, data);
}

/** @brief  Read one of the three status registers (1..3). */
static uint8_t norflash_read_sr(uint8_t regno)
{
    uint8_t command;
    uint8_t byte;

    switch (regno)
    {
        case 2:
            command = FLASH_ReadStatusReg2;
            break;
        case 3:
            command = FLASH_ReadStatusReg3;
            break;
        case 1:
        default:
            command = FLASH_ReadStatusReg1;
            break;
    }

    NORFLASH_CS_LOW();
    (void)norflash_spi_rw(command);
    byte = norflash_spi_rw(0xFFU);
    NORFLASH_CS_HIGH();

    return byte;
}

static void norflash_wait_busy(void)
{
    while ((norflash_read_sr(1) & FLASH_SR1_BUSY_BIT) == FLASH_SR1_BUSY_BIT)
    {
    }
}

static void norflash_write_enable(void)
{
    NORFLASH_CS_LOW();
    (void)norflash_spi_rw(FLASH_WriteEnable);
    NORFLASH_CS_HIGH();
}

static void norflash_send_address(uint32_t address)
{
    if ((g_norflash_type == W25Q256) || (g_norflash_type == BY25Q256))
    {
        (void)norflash_spi_rw((uint8_t)(address >> 24));
    }

    (void)norflash_spi_rw((uint8_t)(address >> 16));
    (void)norflash_spi_rw((uint8_t)(address >> 8));
    (void)norflash_spi_rw((uint8_t)address);
}

static void norflash_write_status_reg3(uint8_t sr)
{
    NORFLASH_CS_LOW();
    (void)norflash_spi_rw(FLASH_WriteStatusReg3);
    (void)norflash_spi_rw(sr);
    NORFLASH_CS_HIGH();
}

void norflash_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* ---- MSP begin: CS clock + pin ---- */
    __HAL_RCC_GPIOF_CLK_ENABLE();

    gpio_init.Pin   = NORFLASH_CS_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(NORFLASH_CS_GPIO_PORT, &gpio_init);
    /* ---- MSP end ---- */

    NORFLASH_CS_HIGH();

    spi_init(SPI_BUS_NORFLASH);
    spi_set_speed(SPI_BUS_NORFLASH, SPI_BAUDRATEPRESCALER_2);

    g_norflash_type = norflash_read_id();

    if ((g_norflash_type == W25Q256) || (g_norflash_type == BY25Q256))
    {
        uint8_t sr3 = norflash_read_sr(3);

        if ((sr3 & 0x01U) == 0U) /* not yet in 4-byte address mode */
        {
            norflash_write_enable();
            sr3 |= FLASH_SR3_ADP_BIT;
            norflash_write_status_reg3(sr3);
            delay_ms(20U);

            NORFLASH_CS_LOW();
            (void)norflash_spi_rw(FLASH_Enable4ByteAddr);
            NORFLASH_CS_HIGH();
        }
    }
}

uint16_t norflash_read_id(void)
{
    uint16_t deviceid;

    NORFLASH_CS_LOW();
    (void)norflash_spi_rw(FLASH_ManufactDeviceID);
    (void)norflash_spi_rw(0x00U);
    (void)norflash_spi_rw(0x00U);
    (void)norflash_spi_rw(0x00U);
    deviceid  = (uint16_t)(norflash_spi_rw(0xFFU) << 8);
    deviceid |= norflash_spi_rw(0xFFU);
    NORFLASH_CS_HIGH();

    return deviceid;
}

void norflash_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t i;

    NORFLASH_CS_LOW();
    (void)norflash_spi_rw(FLASH_ReadData);
    norflash_send_address(addr);

    for (i = 0U; i < datalen; i++)
    {
        pbuf[i] = norflash_spi_rw(0xFFU);
    }

    NORFLASH_CS_HIGH();
}

static void norflash_write_page(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t i;

    norflash_write_enable();

    NORFLASH_CS_LOW();
    (void)norflash_spi_rw(FLASH_PageProgram);
    norflash_send_address(addr);

    for (i = 0U; i < datalen; i++)
    {
        (void)norflash_spi_rw(pbuf[i]);
    }

    NORFLASH_CS_HIGH();
    norflash_wait_busy();
}

static void norflash_write_nocheck(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t pageremain = (uint16_t)(NORFLASH_PAGE_SIZE - (addr % NORFLASH_PAGE_SIZE));

    if (datalen <= pageremain)
    {
        pageremain = datalen;
    }

    for (;;)
    {
        norflash_write_page(pbuf, addr, pageremain);

        if (datalen == pageremain)
        {
            break;
        }

        pbuf      += pageremain;
        addr      += pageremain;
        datalen   -= pageremain;
        pageremain = (datalen > NORFLASH_PAGE_SIZE) ? NORFLASH_PAGE_SIZE : datalen;
    }
}

void norflash_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint32_t secpos;
    uint16_t secoff;
    uint16_t secremain;
    uint16_t i;

    secpos    = addr / NORFLASH_SECTOR_SIZE;
    secoff    = (uint16_t)(addr % NORFLASH_SECTOR_SIZE);
    secremain = (uint16_t)(NORFLASH_SECTOR_SIZE - secoff);

    if (datalen <= secremain)
    {
        secremain = datalen;
    }

    for (;;)
    {
        norflash_read(g_norflash_buf, secpos * NORFLASH_SECTOR_SIZE, NORFLASH_SECTOR_SIZE);

        for (i = 0U; i < secremain; i++)
        {
            if (g_norflash_buf[secoff + i] != 0xFFU)
            {
                break;
            }
        }

        if (i < secremain) /* sector is not blank: erase and merge */
        {
            norflash_erase_sector(secpos);

            for (i = 0U; i < secremain; i++)
            {
                g_norflash_buf[i + secoff] = pbuf[i];
            }

            norflash_write_nocheck(g_norflash_buf, secpos * NORFLASH_SECTOR_SIZE, NORFLASH_SECTOR_SIZE);
        }
        else
        {
            norflash_write_nocheck(pbuf, addr, secremain);
        }

        if (datalen == secremain)
        {
            break;
        }

        secpos++;
        secoff = 0U;
        pbuf   += secremain;
        addr   += secremain;
        datalen -= secremain;
        secremain = (datalen > NORFLASH_SECTOR_SIZE) ? NORFLASH_SECTOR_SIZE : datalen;
    }
}

void norflash_erase_sector(uint32_t saddr)
{
    saddr *= NORFLASH_SECTOR_SIZE;

    norflash_write_enable();
    norflash_wait_busy();

    NORFLASH_CS_LOW();
    (void)norflash_spi_rw(FLASH_SectorErase);
    norflash_send_address(saddr);
    NORFLASH_CS_HIGH();
    norflash_wait_busy();
}
