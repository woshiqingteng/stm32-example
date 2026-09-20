/**
 * @file    key.h
 * @brief   On-board keys interface.
 */

#ifndef BSP_KEY_H
#define BSP_KEY_H

/** @brief On-board keys: KEY0 = PH3, KEY1 = PH2, KEY2 = PC13, WK_UP = PA0. */
typedef enum
{
    KEY0 = 0,
    KEY1,
    KEY2,
    KEY_WKUP,
    KEY_NUM,
} key_id_t;

typedef enum
{
    KEY_RELEASED = 0,
    KEY_PRESSED,
} key_state_t;

/** @brief  Initialise the key GPIOs. */
void key_init(void);

/**
 * @brief  Read the current debounced level of a key.
 * @param  id Key index.
 * @return KEY_PRESSED or KEY_RELEASED.
 */
key_state_t key_read(key_id_t id);

#endif /* BSP_KEY_H */
