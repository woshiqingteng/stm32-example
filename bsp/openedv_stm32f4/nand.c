/**
 * @file    nand.c
 * @brief   NAND FLASH driver over FMC bank3 (8-bit bus), ported from the vendor
 *          NAND example. The FMC hardware ECC is used on demand while reading
 *          (PCR3) and page copy support is provided for the FTL. MSP content is
 *          inlined into nand_init().
 */

#include <stdio.h>
#include "nand.h"
#include "delay.h"

NAND_HandleTypeDef g_nand_handle;
nand_attriute      nand_dev;

void nand_delay(volatile uint32_t i)
{
    while (i > 0U)
    {
        i--;
    }
}

uint8_t nand_waitrb(volatile uint8_t rb)
{
    volatile uint32_t time = 0U;

    while (time < 0x1FFFFFFU)
    {
        time++;

        if (NAND_RB == rb)
        {
            return 0U;
        }
    }

    return 1U;
}

uint8_t nand_readstatus(void)
{
    volatile uint8_t data;

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_READSTA;
    nand_delay(NAND_TWHR_DELAY);
    data = *(volatile uint8_t *)NAND_ADDRESS;

    return data;
}

uint8_t nand_wait_for_ready(void)
{
    uint8_t status;
    volatile uint32_t time = 0U;

    for (;;)
    {
        status = nand_readstatus();

        if ((status & NSTA_READY) != 0U)
        {
            break;
        }

        time++;

        if (time >= 0x1FFFFFFFU)
        {
            return NSTA_TIMEOUT;
        }
    }

    return NSTA_READY;
}

uint8_t nand_reset(void)
{
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_RESET;

    return (nand_wait_for_ready() == NSTA_READY) ? 0U : 1U;
}

uint32_t nand_readid(void)
{
    uint8_t  deviceid[5];
    uint32_t id;

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = nand_readID;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = 0x00;

    deviceid[0] = *(volatile uint8_t *)NAND_ADDRESS;
    deviceid[1] = *(volatile uint8_t *)NAND_ADDRESS;
    deviceid[2] = *(volatile uint8_t *)NAND_ADDRESS;
    deviceid[3] = *(volatile uint8_t *)NAND_ADDRESS;
    deviceid[4] = *(volatile uint8_t *)NAND_ADDRESS;

    /* The vendor drops the first ID byte and keeps the remaining four. */
    id = ((uint32_t)deviceid[1] << 24) | ((uint32_t)deviceid[2] << 16) |
         ((uint32_t)deviceid[3] << 8)  | (uint32_t)deviceid[4];

    return id;
}

uint8_t nand_modeset(uint8_t mode)
{
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_FEATURE;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = 0x01;
    *(volatile uint8_t *)NAND_ADDRESS = mode;
    *(volatile uint8_t *)NAND_ADDRESS = 0;
    *(volatile uint8_t *)NAND_ADDRESS = 0;
    *(volatile uint8_t *)NAND_ADDRESS = 0;

    return (nand_wait_for_ready() == NSTA_READY) ? 0U : 1U;
}

uint8_t nand_init(void)
{
    GPIO_InitTypeDef          gpio_init = {0};
    FMC_NAND_PCC_TimingTypeDef com_timing = {0};
    FMC_NAND_PCC_TimingTypeDef att_timing = {0};

    /* ---- MSP begin: FMC + PD/PE/PG NAND pins ---- */
    __HAL_RCC_FMC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    gpio_init.Pin   = NAND_RB_GPIO_PIN;
    gpio_init.Mode  = GPIO_MODE_INPUT;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(NAND_RB_GPIO_PORT, &gpio_init);

    gpio_init.Pin       = GPIO_PIN_9;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOG, &gpio_init);

    gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 |
                    GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &gpio_init);

    gpio_init.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOE, &gpio_init);
    /* ---- MSP end ---- */

    g_nand_handle.Instance = FMC_NAND_DEVICE;
    g_nand_handle.Init.NandBank          = FMC_NAND_BANK3;
    g_nand_handle.Init.Waitfeature       = FMC_NAND_PCC_WAIT_FEATURE_DISABLE;
    g_nand_handle.Init.MemoryDataWidth   = FMC_NAND_PCC_MEM_BUS_WIDTH_8;
    g_nand_handle.Init.EccComputation    = FMC_NAND_ECC_DISABLE;
    g_nand_handle.Init.ECCPageSize       = FMC_NAND_ECC_PAGE_SIZE_2048BYTE;
    g_nand_handle.Init.TCLRSetupTime     = 8;
    g_nand_handle.Init.TARSetupTime      = 8;

    com_timing.SetupTime     = 4;
    com_timing.WaitSetupTime = 6;
    com_timing.HoldSetupTime = 2;
    com_timing.HiZSetupTime  = 3;

    att_timing.SetupTime     = 4;
    att_timing.WaitSetupTime = 6;
    att_timing.HoldSetupTime = 2;
    att_timing.HiZSetupTime  = 3;

    (void)HAL_NAND_Init(&g_nand_handle, &com_timing, &att_timing);

    (void)nand_reset();
    delay_ms(100U);
    nand_dev.id = nand_readid();
    (void)nand_modeset(4);

    if (nand_dev.id == MT29F16G08ABABA)
    {
        nand_dev.page_totalsize = 4320;
        nand_dev.page_mainsize  = 4096;
        nand_dev.page_sparesize = 224;
        nand_dev.block_pagenum  = 128;
        nand_dev.plane_blocknum = 2048;
        nand_dev.block_totalnum = 4096;
    }
    else if (nand_dev.id == MT29F4G08ABADA)
    {
        nand_dev.page_totalsize = 2112;
        nand_dev.page_mainsize  = 2048;
        nand_dev.page_sparesize = 64;
        nand_dev.block_pagenum  = 64;
        nand_dev.plane_blocknum = 2048;
        nand_dev.block_totalnum = 4096;
    }
    else if (nand_dev.id == FSNS8B004G)
    {
        nand_dev.page_totalsize = 4160;
        nand_dev.page_mainsize  = 4096;
        nand_dev.page_sparesize = 64;
        nand_dev.block_pagenum  = 64;
        nand_dev.plane_blocknum = 1024;
        nand_dev.block_totalnum = 2048;
    }
    else
    {
        return 1U;
    }

    return 0U;
}

void nand_get_info(nand_info_t *info)
{
    if (info == 0)
    {
        return;
    }

    info->id              = nand_dev.id;
    info->page_mainsize   = nand_dev.page_mainsize;
    info->block_pagenum   = nand_dev.block_pagenum;
    info->block_totalnum  = nand_dev.block_totalnum;
    info->size_mb         = ((uint32_t)nand_dev.block_totalnum / 1024U) *
                            ((uint32_t)nand_dev.page_mainsize / 1024U) *
                            (uint32_t)nand_dev.block_pagenum;
}

uint8_t nand_readpage(uint32_t pagenum, uint16_t colnum, uint8_t *pbuffer, uint16_t numbyte_to_read)
{
    volatile uint16_t i;
    uint8_t  sect;
    uint8_t  eccnum;
    uint8_t  eccstart;
    uint8_t  errsta = 0U;
    uint8_t *p;

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_AREA_A;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)colnum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(colnum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)pagenum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 16);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_AREA_TRUE1;

    if (nand_waitrb(0) != 0U)
    {
        return NSTA_TIMEOUT;
    }

    if (nand_waitrb(1) != 0U)
    {
        return NSTA_TIMEOUT;
    }

    if ((numbyte_to_read % NAND_ECC_SECTOR_SIZE) != 0U)
    {
        /* Not a whole number of ECC sectors: raw read, no ECC. */
        for (i = 0U; i < numbyte_to_read; i++)
        {
            *pbuffer++ = *(volatile uint8_t *)NAND_ADDRESS;
        }
    }
    else
    {
        eccnum   = (uint8_t)(numbyte_to_read / NAND_ECC_SECTOR_SIZE);
        eccstart = (uint8_t)(colnum / NAND_ECC_SECTOR_SIZE);
        p        = pbuffer;

        for (sect = 0U; sect < eccnum; sect++)
        {
            FMC_Bank2_3->PCR3 |= 1U << 6;                       /* enable ECC */

            for (i = 0U; i < NAND_ECC_SECTOR_SIZE; i++)
            {
                *pbuffer++ = *(volatile uint8_t *)NAND_ADDRESS;
            }

            while ((FMC_Bank2_3->SR3 & (1U << 6)) == 0U)         /* wait FIFO ready */
            {
            }

            nand_dev.ecc_hdbuf[sect + eccstart] = FMC_Bank2_3->ECCR3;
            FMC_Bank2_3->PCR3 &= ~(1U << 6);                    /* disable ECC */
        }

        i = (uint16_t)(nand_dev.page_mainsize + 0x10U + eccstart * 4U);
        nand_delay(NAND_TRHW_DELAY);
        *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD)  = 0x05; /* random data output */
        *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)i;
        *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(i >> 8);
        *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD)  = 0xE0;

        nand_delay(NAND_TWHR_DELAY);
        pbuffer = (uint8_t *)&nand_dev.ecc_rdbuf[eccstart];

        for (i = 0U; i < (4U * eccnum); i++)                    /* read stored ECC */
        {
            *pbuffer++ = *(volatile uint8_t *)NAND_ADDRESS;
        }

        for (i = 0U; i < eccnum; i++)
        {
            if (nand_dev.ecc_rdbuf[i + eccstart] != nand_dev.ecc_hdbuf[i + eccstart])
            {
                printf("err hd,rd:0x%x,0x%x\r\n", nand_dev.ecc_hdbuf[i + eccstart], nand_dev.ecc_rdbuf[i + eccstart]);
                printf("eccnum,eccstart:%d,%d\r\n", eccnum, eccstart);
                printf("PageNum,ColNum:%d,%d\r\n", (int)pagenum, (int)colnum);

                if (nand_ecc_correction(p + NAND_ECC_SECTOR_SIZE * i,
                                        nand_dev.ecc_rdbuf[i + eccstart],
                                        nand_dev.ecc_hdbuf[i + eccstart]) != 0U)
                {
                    errsta = NSTA_ECC2BITERR;
                }
                else
                {
                    errsta = NSTA_ECC1BITERR;
                }
            }
        }
    }

    if (nand_wait_for_ready() != NSTA_READY)
    {
        errsta = NSTA_ERROR;
    }

    return errsta;
}

uint8_t nand_readpagecomp(uint32_t pagenum, uint16_t colnum, uint32_t cmpval,
                          uint16_t numbyte_to_read, uint16_t *numbyte_equal)
{
    uint16_t i;
    uint8_t  res;

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_AREA_A;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)colnum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(colnum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)pagenum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 16);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_AREA_TRUE1;

    res = nand_waitrb(0);
    if (res != 0U)
    {
        return NSTA_TIMEOUT;
    }

    res = nand_waitrb(1);
    if (res != 0U)
    {
        return NSTA_TIMEOUT;
    }

    for (i = 0U; i < numbyte_to_read; i++)
    {
        if (*(volatile uint32_t *)NAND_ADDRESS != cmpval)
        {
            break;
        }
    }

    *numbyte_equal = i;

    if (nand_wait_for_ready() != NSTA_READY)
    {
        return NSTA_ERROR;
    }

    return 0U;
}

uint8_t nand_writepage(uint32_t pagenum, uint16_t colnum, uint8_t *pbuffer, uint16_t numbyte_to_write)
{
    volatile uint16_t i;

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_WRITE0;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)colnum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(colnum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)pagenum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 16);
    nand_delay(NAND_TADL_DELAY);

    for (i = 0U; i < numbyte_to_write; i++)
    {
        *(volatile uint8_t *)NAND_ADDRESS = pbuffer[i];
    }

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_WRITE_TURE1;
    delay_us(NAND_TPROG_DELAY);

    if (nand_wait_for_ready() != NSTA_READY)
    {
        return NSTA_ERROR;
    }

    return 0U;
}

uint8_t nand_write_pageconst(uint32_t pagenum, uint16_t colnum, uint32_t cval, uint16_t numbyte_to_write)
{
    uint16_t i;

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_WRITE0;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)colnum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(colnum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)pagenum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(pagenum >> 16);
    nand_delay(NAND_TADL_DELAY);

    for (i = 0U; i < numbyte_to_write; i++)
    {
        *(volatile uint32_t *)NAND_ADDRESS = cval;
    }

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_WRITE_TURE1;
    delay_us(NAND_TPROG_DELAY);

    if (nand_wait_for_ready() != NSTA_READY)
    {
        return NSTA_ERROR;
    }

    return 0U;
}

uint8_t nand_readspare(uint32_t pagenum, uint16_t colnum, uint8_t *pbuffer, uint16_t numbyte_to_read)
{
    uint8_t remainbyte = (uint8_t)(nand_dev.page_sparesize - colnum);

    if (numbyte_to_read > remainbyte)
    {
        numbyte_to_read = remainbyte;
    }

    return nand_readpage(pagenum, (uint16_t)(colnum + nand_dev.page_mainsize),
                         pbuffer, numbyte_to_read);
}

uint8_t nand_writespare(uint32_t pagenum, uint16_t colnum, uint8_t *pbuffer, uint16_t numbyte_to_write)
{
    uint8_t remainbyte = (uint8_t)(nand_dev.page_sparesize - colnum);

    if (numbyte_to_write > remainbyte)
    {
        numbyte_to_write = remainbyte;
    }

    return nand_writepage(pagenum, (uint16_t)(colnum + nand_dev.page_mainsize),
                          pbuffer, numbyte_to_write);
}

uint8_t nand_eraseblock(uint32_t blocknum)
{
    if (nand_dev.id == MT29F16G08ABABA)
    {
        blocknum <<= 7;
    }
    else if (nand_dev.id == MT29F4G08ABADA)
    {
        blocknum <<= 6;
    }
    else if (nand_dev.id == FSNS8B004G)
    {
        blocknum <<= 6;
    }

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_ERASE0;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)blocknum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(blocknum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(blocknum >> 16);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_ERASE1;

    delay_ms(NAND_TBERS_DELAY);

    if (nand_wait_for_ready() != NSTA_READY)
    {
        return NSTA_ERROR;
    }

    return 0U;
}

void nand_erasechip(void)
{
    uint16_t i;

    for (i = 0U; i < nand_dev.block_totalnum; i++)
    {
        (void)nand_eraseblock(i);
    }
}

uint8_t nand_copypage_withoutwrite(uint32_t source_pagenum, uint32_t dest_pagenum)
{
    uint32_t source_block = source_pagenum / nand_dev.block_pagenum;
    uint32_t dest_block   = dest_pagenum / nand_dev.block_pagenum;

    if ((source_block % 2U) != (dest_block % 2U))
    {
        return NSTA_ERROR;                                  /* must be in the same plane */
    }

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_MOVEDATA_CMD0;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = 0x00;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = 0x00;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)source_pagenum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(source_pagenum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(source_pagenum >> 16);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_MOVEDATA_CMD1;

    if (nand_waitrb(0) != 0U)
    {
        return NSTA_TIMEOUT;
    }

    if (nand_waitrb(1) != 0U)
    {
        return NSTA_TIMEOUT;
    }

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_MOVEDATA_CMD2;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = 0x00;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = 0x00;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)dest_pagenum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(dest_pagenum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(dest_pagenum >> 16);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_MOVEDATA_CMD3;
    delay_us(NAND_TPROG_DELAY);

    if (nand_wait_for_ready() != NSTA_READY)
    {
        return NSTA_ERROR;
    }

    return 0U;
}

uint8_t nand_copypage_withwrite(uint32_t source_pagenum, uint32_t dest_pagenum, uint16_t colnum,
                                uint8_t *pbuffer, uint16_t numbyte_to_write)
{
    volatile uint16_t i;
    uint8_t  sect;
    uint8_t  eccnum;
    uint8_t  eccstart;
    uint32_t source_block = source_pagenum / nand_dev.block_pagenum;
    uint32_t dest_block   = dest_pagenum / nand_dev.block_pagenum;

    if ((source_block % 2U) != (dest_block % 2U))
    {
        return NSTA_ERROR;                                  /* must be in the same plane */
    }

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD)  = NAND_MOVEDATA_CMD0;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = 0x00;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = 0x00;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)source_pagenum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(source_pagenum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(source_pagenum >> 16);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD)  = NAND_MOVEDATA_CMD1;

    if (nand_waitrb(0) != 0U)
    {
        return NSTA_TIMEOUT;
    }

    if (nand_waitrb(1) != 0U)
    {
        return NSTA_TIMEOUT;
    }

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD)  = NAND_MOVEDATA_CMD2;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)colnum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(colnum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)dest_pagenum;
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(dest_pagenum >> 8);
    *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(dest_pagenum >> 16);
    nand_delay(NAND_TADL_DELAY);

    if ((numbyte_to_write % NAND_ECC_SECTOR_SIZE) != 0U)
    {
        for (i = 0U; i < numbyte_to_write; i++)
        {
            *(volatile uint8_t *)NAND_ADDRESS = *pbuffer++;
        }
    }
    else
    {
        eccnum   = (uint8_t)(numbyte_to_write / NAND_ECC_SECTOR_SIZE);
        eccstart = (uint8_t)(colnum / NAND_ECC_SECTOR_SIZE);

        for (sect = 0U; sect < eccnum; sect++)
        {
            FMC_Bank2_3->PCR3 |= 1U << 6;

            for (i = 0U; i < NAND_ECC_SECTOR_SIZE; i++)
            {
                *(volatile uint8_t *)NAND_ADDRESS = *pbuffer++;
            }

            while ((FMC_Bank2_3->SR3 & (1U << 6)) == 0U)
            {
            }

            nand_dev.ecc_hdbuf[sect + eccstart] = FMC_Bank2_3->ECCR3;
            FMC_Bank2_3->PCR3 &= ~(1U << 6);
        }

        i = (uint16_t)(nand_dev.page_mainsize + 0x10U + eccstart * 4U);
        nand_delay(NAND_TADL_DELAY);
        *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD)  = 0x85;
        *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)i;
        *(volatile uint8_t *)(NAND_ADDRESS | NAND_ADDR) = (uint8_t)(i >> 8);
        nand_delay(NAND_TADL_DELAY);

        pbuffer = (uint8_t *)&nand_dev.ecc_hdbuf[eccstart];

        for (i = 0U; i < eccnum; i++)
        {
            *(volatile uint8_t *)NAND_ADDRESS = *pbuffer++;
            *(volatile uint8_t *)NAND_ADDRESS = *pbuffer++;
            *(volatile uint8_t *)NAND_ADDRESS = *pbuffer++;
            *(volatile uint8_t *)NAND_ADDRESS = *pbuffer++;
        }
    }

    *(volatile uint8_t *)(NAND_ADDRESS | NAND_CMD) = NAND_MOVEDATA_CMD3;
    delay_us(NAND_TPROG_DELAY);

    if (nand_wait_for_ready() != NSTA_READY)
    {
        return NSTA_ERROR;
    }

    return 0U;
}

uint16_t nand_ecc_get_oe(uint8_t oe, uint32_t eccval)
{
    uint8_t  i;
    uint16_t ecctemp = 0U;

    for (i = 0U; i < 24U; i++)
    {
        if ((i % 2U) == oe)
        {
            if (((eccval >> i) & 0x01U) != 0U)
            {
                ecctemp = (uint16_t)(ecctemp + (1U << (i >> 1)));
            }
        }
    }

    return ecctemp;
}

uint8_t nand_ecc_correction(uint8_t *data_buf, uint32_t eccrd, uint32_t ecccl)
{
    uint16_t eccrdo;
    uint16_t eccrde;
    uint16_t eccclo;
    uint16_t ecccle;
    uint16_t errorpos;
    uint32_t bytepos;

    eccrdo = nand_ecc_get_oe(1U, eccrd);
    eccrde = nand_ecc_get_oe(0U, eccrd);
    eccclo = nand_ecc_get_oe(1U, ecccl);
    ecccle = nand_ecc_get_oe(0U, ecccl);

    if ((uint16_t)(eccrdo ^ eccrde ^ eccclo ^ ecccle) == 0xFFFFU)
    {
        /* single-bit error: locate and flip it */
        errorpos = (uint16_t)(eccrdo ^ eccclo);
        printf("errorpos:%d\r\n", errorpos);
        bytepos  = errorpos / 8U;
        data_buf[bytepos] ^= (uint8_t)(1U << (errorpos % 8U));
    }
    else
    {
        /* two or more bit errors cannot be recovered */
        printf("2bit ecc error or more\r\n");
        return 1U;
    }

    return 0U;
}
