app_begin(01_led)

use_target()
use_cmsis_core()
use_hal_driver(RCC RCC_EX FLASH FLASH_EX FLASH_RAMFUNC GPIO CORTEX PWR PWR_EX DMA DMA_EX UART)
use_bsp(sys delay usart led)

app_add()
