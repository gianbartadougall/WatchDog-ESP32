/**
 * @file comms.h
 * @author Gian Barta-Dougall
 * @brief Library for communicating
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
void comms_send_data(USART_TypeDef* uart, char* msg, uint8_t sendNull);

/**
 * @brief Reads a single character from UART line
 *
 * @return char the character it read
 */
int comms_read_data(USART_TypeDef* uart, char msg[RX_BUF_SIZE], uint32_t timeout);

void comms_add_to_buffer(USART_TypeDef* usart, char c);

void comms_usart1_print_buffer(void);

void comms_create_packet(packet_t* packet, uint8_t request, char* instruction, char* data);

#endif // COMMS_H