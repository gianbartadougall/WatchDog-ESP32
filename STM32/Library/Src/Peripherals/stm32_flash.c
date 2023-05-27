/* C Library Includes */

/* STM32 Includes */
#include "stm32l432xx.h"

/* Personal Includes */
#include "stm32_flash.h"

uint8_t stm32_flash_write() {

    // Check no flash operation is currently ongoing
    while ((FLASH->SR & FLASH_SR_BSY) != 0) {}

    // Clear any errors due to previous programming of flash

    // Enable programming of flash
    FLASH->CR |= FLASH_SR_PG;

    // Perform Data write operation to desired memory address

    // Wait until busy flag has been cleared
    while ((FLASH->SR & FLASH_SR_BSY) != 0) {}

    // Confirm the flash programming succeeded
    if ((FLASH->SR & FLASH_SR_EOP) == 0) {
        return FALSE; // Programming flash memory failed
    }

    // Disable programming mode
    FLASH->CR &= ~(FLASH_SR_PG);

    return TRUE;
}