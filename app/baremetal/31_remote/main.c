/**
 * @file    main.c
 * @brief   31_remote: NEC infrared remote test. The decoded key code and repeat
 *          count are printed over USART1.
 */

#include <stdio.h>
#include "bsp.h"

#define KEY_SETTLE_DELAY_MS 100U
#define POLL_DELAY_MS   10U

static const char *ir_symbol(uint8_t key)
{
    switch (key)
    {
        case IR_KEY_POWER:    return "POWER";
        case IR_KEY_UP:       return "UP";
        case IR_KEY_PLAY:     return "PLAY";
        case IR_KEY_ALIENTEK: return "ALIENTEK";
        case IR_KEY_RIGHT:    return "RIGHT";
        case IR_KEY_LEFT:     return "LEFT";
        case IR_KEY_VOL_DOWN: return "VOL-";
        case IR_KEY_DOWN:     return "DOWN";
        case IR_KEY_VOL_UP:   return "VOL+";
        case IR_KEY_1:        return "1";
        case IR_KEY_2:        return "2";
        case IR_KEY_3:        return "3";
        case IR_KEY_4:        return "4";
        case IR_KEY_5:        return "5";
        case IR_KEY_6:        return "6";
        case IR_KEY_7:        return "7";
        case IR_KEY_8:        return "8";
        case IR_KEY_9:        return "9";
        case IR_KEY_0:        return "0";
        case IR_KEY_DELETE:   return "DELETE";
        default:                  return "UNKNOWN";
    }
}

int main(void)
{
    uint8_t key;
    uint8_t led_tick = 0U;
    char    line[48];

    bsp_init();
    ir_init();

    printf("31_remote ready\r\n");

    for (;;)
    {
        key = ir_scan();

        if (key != 0U)
        {
            sprintf(line, "KEY: %u CNT: %u SYM: %s", key, ir_repeat_count(), ir_symbol(key));
            printf("%s\r\n", line);

            delay_ms(KEY_SETTLE_DELAY_MS);
        }
        else
        {
            delay_ms(POLL_DELAY_MS);
        }

        led_tick++;
        if (led_tick >= 50U)
        {
            led_tick = 0U;
            led_toggle(LED0);
        }
    }
}
