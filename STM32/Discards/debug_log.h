/**
 * @file debug_log.c
 * @author Gian Barta-Dougall
 * @brief Library for debugging
 * @version 0.1
 * @date 2022-05-31
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

/* Public Includes */
#include <stdint.h>
#include <stdio.h>

/* STM32 Includes */
#include "stm32l432xx.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_uart.h"

/**
 * @brief Initialises the UART Handle so the serial communication functions
 * in this class can operate correctly
 *
 * @param uart_handle Handle to the uart line that the serial communication
 * should operate over
 */
void debug_log_init(UART_HandleTypeDef *uart_handle);
/**
 * @brief Writes data of USART using the STM32 HAL library.
 * Maximum write size of 256 bytes per function call.
 *
 * @note This function has a timeout of 1000ms
 *
 * @param data Pointer to the data to be transmitted
 * @return Returns HAL_OK if print was succesful; HAL_ERROR otherwise
 */
HAL_StatusTypeDef debug_prints(char *pdata);

HAL_StatusTypeDef debug_printa(uint8_t *ptr, uint8_t length);

/**
 * @brief Recives a single character from a serial input. For example
 * this function will detect a char written in putty. If a null
 * terminator is receivied, this signifies there was an error
 * when receiving the data
 *
 * @note This function has a timeout of 1ms
 *
 * @return The char that was written in serial if transmission was
 * succesful; Null terminator character if transmission failed.
 */
char debug_getc(void);

/**
 * @brief Clears the serial terminal
 *
 * @return Return HAL_OK if clear was succesful; HAL_ERROR otherwise
 */
HAL_StatusTypeDef debug_clear(void);

HAL_StatusTypeDef debug_print_uint64(uint64_t number);

#endif // DEBUG_LOG_H