#ifndef STM32_FLASH_H
#define STM32_FLASH_H

/* Public Macros */
#define STM32_FLASH_ADDR_START (0x08000000)
#define STM32_FLASH_ADDR_END   (0x0807FFFF)

uint8_t stm32_flash_erase_page(uint8_t page);

uint8_t stm32_flash_write(uint32_t address, uint32_t data);
uint8_t stm32_flash_read(uint32_t address, uint32_t* data);
#endif // STM32_FLASH