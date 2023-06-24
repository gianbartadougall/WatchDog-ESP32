/* C Library Includes */

/* STM32 Includes */
#include "stm32l432xx.h"

/* Personal Includes */
#include "stm32_flash.h"
#include "utils.h"
#include "log.h"

/* Private Macros */
#define STM32_FLASH_ADDRESS_IS_VALID(address) (address >= STM32_FLASH_ADDR_START && address <= STM32_FLASH_ADDR_END)

/* Function Prototypes */
uint8_t stm32_flash_unlock_cr(void);
uint8_t stm32_flash_lock_cr(void);

uint8_t stm32_flash_write(uint32_t address, uint32_t data) {

    if (STM32_FLASH_ADDRESS_IS_VALID(address) != TRUE) {
        return FALSE;
    }

    // Check no flash operation is currently ongoing
    while ((FLASH->SR & FLASH_SR_BSY) != 0) {}

    // Clear any errors due to previous programming of flash

    // Enable programming of flash
    FLASH->CR |= FLASH_CR_PG;

    // Perform Data write operation to desired memory address
    *((volatile uint32_t*)address) = data;

    // Wait until busy flag has been cleared
    while ((FLASH->SR & FLASH_SR_BSY) != 0) {}

    // Confirm the flash programming succeeded
    if ((FLASH->SR & FLASH_SR_EOP) == 0) {
        return FALSE; // Programming flash memory failed
    }

    // Disable programming mode
    FLASH->CR &= ~(FLASH_CR_PG);

    return TRUE;
}

uint8_t stm32_flash_unlock_cr(void) {
    if ((FLASH->CR & FLASH_CR_LOCK) != 0) {
        FLASH->KEYR = (uint32_t)0x45670123;
        FLASH->KEYR = (uint32_t)0xCDEF89AB;

        // Verify the flash was unlocked
        if ((FLASH->CR & FLASH_CR_LOCK) != 0) {
            return FALSE;
        }
    }

    return TRUE;
}

uint8_t stm32_flash_lock_cr(void) {
    if ((FLASH->CR & FLASH_CR_LOCK) == 0) {

        // Lock the CR register
        FLASH->CR |= (FLASH_CR_LOCK);

        // Confirm flash is locked
        if ((FLASH->CR & FLASH_CR_LOCK) == 0) {
            return FALSE;
        }
    }

    return TRUE;
}

uint8_t stm32_flash_erase_page(uint8_t page) {

    // Check no flash operation is currently ongoing
    while ((FLASH->SR & FLASH_SR_BSY) != 0) {
        log_message("Stuck1\r\n");
    }

    if (stm32_flash_unlock_cr() != TRUE) {
        return FALSE;
    } else {
        log_message("Unlocked\r\n");
    }

    // Clear any errors due to previous programming
    // FLASH->SR |= (0xFF << 1);

    // Select the page to erase
    // FLASH->CR &= ~(((uint32_t)0xFF) << 3);
    // FLASH->CR |= ((uint32_t)page) << 3;

    // Enable page erase
    FLASH->CR |= (FLASH_CR_MER1);

    // Begin erase by setting the start bit
    FLASH->CR |= (FLASH_CR_STRT);

    // Wait until page has been erased
    while ((FLASH->SR & FLASH_SR_BSY) != 0) {
        log_message("Stuck2\r\n");
    }

    // Disable page erase
    FLASH->CR &= ~(FLASH_CR_PER);

    stm32_flash_lock_cr();

    return TRUE;
}

uint8_t stm32_flash_read(uint32_t address, uint32_t* data) {

    if (STM32_FLASH_ADDRESS_IS_VALID(address) != TRUE) {
        return FALSE;
    }

    *data = *((uint32_t*)address);

    return TRUE;
}