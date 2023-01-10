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

#ifndef COMMS_H
#define COMMS_H

/* Public Includes */
#include <stdint.h>
#include <stdio.h>

/* STM32 Includes */
#include "stm32l432xx.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_uart.h"

#include "uart_comms.h"

/**
 * @brief Writes data over UART
 *
 * @note This function has a timeout of 1000ms
 *
 * @param msg Pointer to the data to be transmitted
 */
void comms_send_data(USART_TypeDef* uart, char* msg);

/**
 * @brief Reads a single character from UART line
 * 
 * @return char the character it read
 */
void comms_read_data(USART_TypeDef* uart, char msg[RX_BUF_SIZE]);

#endif // COMMS_H