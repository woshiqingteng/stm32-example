/**
 * @file    nand_storage.h
 * @brief   NAND sector storage glue over the FTL (FatFs drive + USB-MSC LUN).
 */

#ifndef LIB_NAND_STORAGE_H
#define LIB_NAND_STORAGE_H

/**
 * @brief  Force-link the strong NAND hooks in this component so they override
 *         the weak stubs kept by the port layer. Call once from an app that
 *         exposes NAND storage.
 */
void nand_storage_activate(void);

#endif /* LIB_NAND_STORAGE_H */
