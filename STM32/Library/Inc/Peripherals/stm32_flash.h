#ifndef STM32_FLASH_H
#define STM32_FLASH_H

/* Public Macros */

void stm32_flash_unlock(void);

void stm32_flash_lock(void);

uint8_t stm32_write();

#endif // STM32_FLASH