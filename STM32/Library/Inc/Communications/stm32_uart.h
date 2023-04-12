#ifndef STM32_UART_H
#define STM32_UART_H

/* C Library Includes */
#include <stdint.h>

/* STM32 Includes */
#include "stm32l432xx.h"

/* Personal Includes */

int uart_transmit_string(const char* msg, ...);

void uart_transmit_data(USART_TypeDef* Uart, uint8_t* data, uint8_t length);

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
char uart_getc(USART_TypeDef* Uart);

#endif // STM32_UART_H