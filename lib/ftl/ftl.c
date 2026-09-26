/**
 * @file    ftl.c
 * @brief   NAND flash FTL (flash translation layer): logical-to-physical
 *          mapping, bad-block handling and sector read/write over the raw NAND.
 */

#include <string.h>
#include <stdio.h>
#include "ftl.h"
#include "nand.h"

#define FTL_MAX_BLOCKS 4096U
static uint16_t g_ftl_lut[FTL_MAX_BLOCKS];
static uint8_t  g_ftl_blktbl[FTL_MAX_BLOCKS];

uint8_t ftl_init(void)
{
    uint8_t temp;

    if (nand_init())
    {
        return 1;
    }

    nand_dev.lut = g_ftl_lut;
    memset(nand_dev.lut, 0, nand_dev.block_totalnum * 2);

    temp = ftl_create_lut(1);

    if (temp)
    {
        printf("format nand flash...\r\n");
        temp = ftl_format();

        if (temp)
        {
            printf("format failed!\r\n");
            return 2;
        }
    }
    else
    {
        printf("total block num: %d\r\n", nand_dev.block_totalnum);
        printf("good block num: %d\r\n", nand_dev.good_blocknum);
        printf("valid block num: %d\r\n", nand_dev.valid_blocknum);
    }

    return 0;
}

void ftl_badblock_mark(uint32_t blocknum)
{
    uint32_t temp = 0XAAAAAAAA;
    nand_writespare(blocknum * nand_dev.block_pagenum, 0, (uint8_t *)&temp, 4);
    nand_writespare(blocknum * nand_dev.block_pagenum + 1, 0, (uint8_t *)&temp, 4);
}

uint8_t ftl_check_badblock(uint32_t blocknum)
{
    uint8_t flag = 0;

    nand_readspare(blocknum * nand_dev.block_pagenum, 0, &flag, 1);

    if (flag == 0XFF)
    {
        nand_readspare(blocknum * nand_dev.block_pagenum + 1, 0, &flag, 1);

        if (flag == 0XFF)
        {
            return 0;
        }

        else return 1;
    }

    return 2;
}

uint8_t ftl_used_blockmark(uint32_t blocknum)
{
    uint8_t buf[4];

    uint8_t temp = 0;
    nand_readspare(blocknum * nand_dev.block_pagenum, 0, buf, 4);

    buf[1] = 0XCC;
    temp = nand_writespare(blocknum * nand_dev.block_pagenum, 0, buf, 4);

    return temp;
}

uint32_t ftl_find_unused_block(uint32_t sblock, uint8_t flag)
{
    uint32_t temp = 0;
    uint32_t blocknum = 0;

    for (blocknum = sblock + 1; blocknum > 0; blocknum--)
    {
        if (((blocknum - 1) % 2) == flag)
        {
            nand_readspare((blocknum - 1) * nand_dev.block_pagenum, 0, (uint8_t *)&temp, 4);

            if (temp == 0XFFFFFFFF)
            {
                return (blocknum - 1);
            }
        }
    }

    return 0XFFFFFFFF;
}

uint32_t ftl_find_same_plane_unused_block(uint32_t sblock)
{
    static uint32_t curblock = 0XFFFFFFFF;
    uint32_t unusedblock = 0;

    if (curblock > (nand_dev.block_totalnum - 1))
    {
        curblock = nand_dev.block_totalnum - 1;
    }

    unusedblock = ftl_find_unused_block(curblock, sblock % 2);

    if (unusedblock == 0XFFFFFFFF && curblock < (nand_dev.block_totalnum - 1))
    {
        curblock = nand_dev.block_totalnum - 1;
        unusedblock = ftl_find_unused_block(curblock, sblock % 2);
    }

    if (unusedblock == 0XFFFFFFFF)
    {
        return 0XFFFFFFFF;
    }

    curblock = unusedblock;

    return unusedblock;
}

uint8_t ftl_copy_and_write_to_block(uint32_t source_pagenum, uint16_t colnum, uint8_t *pbuffer, uint32_t numbyte_to_write)
{
    uint16_t i = 0, temp = 0, wrlen;
    uint32_t source_block = 0, pageoffset = 0;
    uint32_t unusedblock = 0;
    source_block = source_pagenum / nand_dev.block_pagenum;
    pageoffset = source_pagenum % nand_dev.block_pagenum;

retry:
    unusedblock = ftl_find_same_plane_unused_block(source_block);

    if (unusedblock > nand_dev.block_totalnum)
    {
        return 1;
    }

    for (i = 0; i < nand_dev.block_pagenum; i++)
    {
        if (i >= pageoffset && numbyte_to_write)
        {
            if (numbyte_to_write > (nand_dev.page_mainsize - colnum))
            {
                wrlen = nand_dev.page_mainsize - colnum;
            }
            else
            {
                wrlen = numbyte_to_write;
            }

            temp = nand_copypage_withwrite(source_block * nand_dev.block_pagenum + i, unusedblock * nand_dev.block_pagenum + i, colnum, pbuffer, wrlen);
            colnum = 0;
            pbuffer += wrlen;
            numbyte_to_write -= wrlen;
        }

        else
        {
            temp = nand_copypage_withoutwrite(source_block * nand_dev.block_pagenum + i, unusedblock * nand_dev.block_pagenum + i);
        }

        if (temp)
        {
            ftl_badblock_mark(unusedblock);
            ftl_create_lut(1);
            goto retry;
        }
    }

    if (i == nand_dev.block_pagenum)
    {
        ftl_used_blockmark(unusedblock);
        nand_eraseblock(source_block);

        for (i = 0; i < nand_dev.block_totalnum; i++)
        {
            if (nand_dev.lut[i] == source_block)
            {
                nand_dev.lut[i] = unusedblock;
                break;
            }
        }
    }

    return 0;
}

uint16_t ftl_lbn_to_pbn(uint32_t lbnnum)
{
    uint16_t pbnno = 0;

    if (lbnnum > nand_dev.valid_blocknum)
    {
        return 0XFFFF;
    }

    pbnno = nand_dev.lut[lbnnum];

    return pbnno;
}

uint8_t ftl_write_sectors(uint8_t *pbuffer, uint32_t sectorno, uint16_t sectorsize, uint32_t sectorcount)
{
    uint8_t flag = 0;
    uint16_t temp;
    uint32_t i = 0;
    uint16_t wsecs;
    uint32_t wlen;
    uint32_t lbnno;
    uint32_t pbnno;
    uint32_t phypageno;
    uint32_t pageoffset;
    uint32_t blockoffset;
    uint32_t markdpbn = 0XFFFFFFFF;

    for (i = 0; i < sectorcount; i++)
    {
        lbnno = (sectorno + i) / (nand_dev.block_pagenum * (nand_dev.page_mainsize / sectorsize));
        pbnno = ftl_lbn_to_pbn(lbnno);

        if (pbnno >= nand_dev.block_totalnum)
        {
            return 1;
        }

        blockoffset = ((sectorno + i) % (nand_dev.block_pagenum * (nand_dev.page_mainsize / sectorsize))) * sectorsize;
        phypageno = pbnno * nand_dev.block_pagenum + blockoffset / nand_dev.page_mainsize;
        pageoffset = blockoffset % nand_dev.page_mainsize;
        temp = nand_dev.page_mainsize - pageoffset;
        temp /= sectorsize;
        wsecs = sectorcount - i;

        if (wsecs >= temp)
        {
            wsecs = temp;
        }

        wlen = wsecs * sectorsize;

        flag = nand_readpagecomp(phypageno, pageoffset, 0XFFFFFFFF, wlen / 4, &temp);

        if (flag)
        {
            return 2;
        }

        if (temp == (wlen / 4))
        {
            flag = nand_writepage(phypageno, pageoffset, pbuffer, wlen);
        }
        else
        {
            flag = 1;
        }

        if (flag == 0 && (markdpbn != pbnno))
        {
            flag = ftl_used_blockmark(pbnno);
            markdpbn = pbnno;
        }

        if (flag)
        {
            temp = ((uint32_t)nand_dev.block_pagenum * nand_dev.page_mainsize - blockoffset) / sectorsize;
            wsecs = sectorcount - i;

            if (wsecs >= temp)
            {
                wsecs = temp;
            }

            wlen = wsecs * sectorsize;
            flag = ftl_copy_and_write_to_block(phypageno, pageoffset, pbuffer, wlen);

            if (flag)
            {
                return 3;
            }
        }

        i += wsecs - 1;
        pbuffer += wlen;
    }

    return 0;
}

uint8_t ftl_read_sectors(uint8_t *pbuffer, uint32_t sectorno, uint16_t sectorsize, uint32_t sectorcount)
{
    uint8_t flag = 0;
    uint16_t rsecs;
    uint32_t i = 0;
    uint32_t lbnno;
    uint32_t pbnno;
    uint32_t phypageno;
    uint32_t pageoffset;
    uint32_t blockoffset;

    for (i = 0; i < sectorcount; i++)
    {
        lbnno = (sectorno + i) / (nand_dev.block_pagenum * (nand_dev.page_mainsize / sectorsize));
        pbnno = ftl_lbn_to_pbn(lbnno);

        if (pbnno >= nand_dev.block_totalnum)
        {
            return 1;
        }

        blockoffset = ((sectorno + i) % (nand_dev.block_pagenum * (nand_dev.page_mainsize / sectorsize))) * sectorsize;
        phypageno = pbnno * nand_dev.block_pagenum + blockoffset / nand_dev.page_mainsize;
        pageoffset = blockoffset % nand_dev.page_mainsize;
        rsecs = (nand_dev.page_mainsize - pageoffset) / sectorsize;

        if (rsecs > (sectorcount - i))
        {
            rsecs = sectorcount - i;
        }

        flag = nand_readpage(phypageno, pageoffset, pbuffer, rsecs * sectorsize);

        if (flag == NSTA_ECC1BITERR)
        {
            flag = nand_readpage(phypageno, pageoffset, pbuffer, rsecs * sectorsize);

            if (flag == NSTA_ECC1BITERR)
            {
                ftl_copy_and_write_to_block(phypageno, pageoffset, pbuffer, rsecs * sectorsize);
                flag = ftl_blockcompare(phypageno / nand_dev.block_pagenum, 0XFFFFFFFF);

                if (flag == 0)
                {
                    flag = ftl_blockcompare(phypageno / nand_dev.block_pagenum, 0X00);
                    nand_eraseblock(phypageno / nand_dev.block_pagenum);
                }

                if (flag)
                {
                    ftl_badblock_mark(phypageno / nand_dev.block_pagenum);
                    ftl_create_lut(1);
                }

                flag = 0;
            }
        }

        if (flag == NSTA_ECC2BITERR)
        {
            flag = 0;
        }

        if (flag)
        {
            return 2;
        }

        pbuffer += sectorsize * rsecs;
        i += rsecs - 1;
    }

    return 0;
}

uint8_t ftl_create_lut(uint8_t mode)
{
    uint32_t i;
    uint8_t buf[4];
    uint32_t lbnnum = 0;

    for (i = 0; i < nand_dev.block_totalnum; i++)
    {
        nand_dev.lut[i] = 0XFFFF;
    }

    nand_dev.good_blocknum = 0;

    for (i = 0; i < nand_dev.block_totalnum; i++)
    {
        nand_readspare(i * nand_dev.block_pagenum, 0, buf, 4);

        if (buf[0] == 0XFF && mode)
        {
            nand_readspare(i * nand_dev.block_pagenum + 1, 0, buf, 1);
        }

        if (buf[0] == 0XFF)
        {
            lbnnum = ((uint16_t)buf[3] << 8) + buf[2];

            if (lbnnum < nand_dev.block_totalnum)
            {
                nand_dev.lut[lbnnum] = i;
            }

            nand_dev.good_blocknum++;
        }
        else
        {
            printf("bad block index:%d\r\n", i);
        }
    }

    for (i = 0; i < nand_dev.block_totalnum; i++)
    {
        if (nand_dev.lut[i] >= nand_dev.block_totalnum)
        {
            nand_dev.valid_blocknum = i;
            break;
        }
    }

    if (nand_dev.valid_blocknum < 100)
    {
        return 2;
    }

    return 0;
}

uint8_t ftl_blockcompare(uint32_t blockx, uint32_t cmpval)
{
    uint8_t res;
    uint16_t i, j, k;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < nand_dev.block_pagenum; j++)
        {
            nand_readpagecomp(blockx * nand_dev.block_pagenum, 0, cmpval, nand_dev.page_mainsize / 4, &k);

            if (k != (nand_dev.page_mainsize / 4))break;
        }

        if (j == nand_dev.block_pagenum)
        {
            return 0;
        }

        res = nand_eraseblock(blockx);

        if (res)
        {
            printf("error erase block:%d\r\n", i);
        }
        else
        {
            if (cmpval != 0XFFFFFFFF)
            {
                for (k = 0; k < nand_dev.block_pagenum; k++)
                {
                    nand_write_pageconst(blockx * nand_dev.block_pagenum + k, 0, 0, nand_dev.page_mainsize / 4);
                }
            }
        }
    }

    printf("bad block checked:%d\r\n", blockx);
    return 1;
}

uint32_t ftl_search_badblock(void)
{
    uint8_t *blktbl;
    uint8_t res;
    uint32_t i, j;
    uint32_t goodblock = 0;

    blktbl = g_ftl_blktbl;
    nand_erasechip();

    for (i = 0; i < nand_dev.block_totalnum; i++)
    {
        res = ftl_blockcompare(i, 0XFFFFFFFF);

        if (res)
        {
            blktbl[i] = 1;
        }
        else
        {
            blktbl[i] = 0;

            for (j = 0; j < nand_dev.block_pagenum; j++)
            {
                nand_write_pageconst(i * nand_dev.block_pagenum + j, 0, 0, nand_dev.page_mainsize / 4);
            }
        }
    }

    for (i = 0; i < nand_dev.block_totalnum; i++)
    {
        if (blktbl[i] == 0)
        {
            res = ftl_blockcompare(i, 0);

            if (res)
            {
                blktbl[i] = 1;
            }
            else
            {
                goodblock++;
            }
        }
    }

    nand_erasechip();

    for (i = 0; i < nand_dev.block_totalnum; i++)
    {
        if (blktbl[i])
        {
            ftl_badblock_mark(i);
        }
    }

    return goodblock;
}

uint8_t ftl_format(void)
{
    uint8_t temp;
    uint8_t buf[6];
    uint32_t i, n;
    uint32_t goodblock = 0;
    nand_dev.good_blocknum = 0;

#if FTL_USE_BAD_BLOCK_SEARCH == 1
    nand_dev.good_blocknum = FTL_SearchBadBlock();
#else

    for (i = 0; i < nand_dev.block_totalnum; i++)
    {
        temp = ftl_check_badblock(i);

        if (temp == 0)
        {
            temp = nand_eraseblock(i);

            if (temp)
            {
                printf("Bad block:%d\r\n", i);
                ftl_badblock_mark(i);
            }
            else
            {
                nand_dev.good_blocknum++;
            }
        }
    }

#endif
    printf("good_blocknum:%d\r\n", nand_dev.good_blocknum);

    if (nand_dev.good_blocknum < 100)
    {
        return 1;
    }

    goodblock = (nand_dev.good_blocknum * 93) / 100;

    n = 0;

    for (i = 0; i < nand_dev.block_totalnum; i++)
    {
        temp = ftl_check_badblock(i);

        if (temp == 0)
        {
            nand_readspare(i * nand_dev.block_pagenum, 0, buf, 4);
            buf[2] = (uint8_t)n;
            buf[3] = (uint8_t)(n >> 8);

            nand_writespare(i * nand_dev.block_pagenum, 0, buf, 4);
            n++;

            if (n == goodblock) break;
        }
    }

    if (ftl_create_lut(1))
    {
        return 2;
    }

    return 0;
}
