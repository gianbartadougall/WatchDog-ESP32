/**
 * @file log_log.c
 * @author Gian Barta-Dougall
 * @brief Library for logging
 * @version 0.1
 * @date 2022-05-31
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef log_LOG_H
#define log_LOG_H

/* Public Includes */
#include <stdint.h>
#include <stdio.h>

/* STM32 Includes */
#include "stm32l432xx.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_uart.h"

/**
 * @brief Writes data to console in red
 *
 * @param msg The message to write to the console
 */
void log_error(char* msg);
void log_success(char* msg);
void log_warning(char* msg);
void log_message(char* msg);

/**
 * @brief Writes data of USART using the STM32 HAL library.
 * Maximum write size of 256 bytes per function call.
 *
 * @note This function has a timeout of 1000ms
 *
 * @param msg Pointer to the data to be transmitted
 */
void log_prints(char* msg);

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
char log_getc(void);

void log_clear(void);

void log_print_const(const char* msg);

#endif // log_LOG_H