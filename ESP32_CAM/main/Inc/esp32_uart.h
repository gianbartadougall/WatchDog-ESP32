/**
 * @file esp32_uart.h
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-17
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef ESP32_UART_H
#define ESP32_UART_H

/* Personal Includes */
#include "bpacket.h"

void esp32_uart_send_bpacket(bpacket_t* bpacket);

int esp32_uart_send_data(const char* data);

// void esp32_uart_get_packet(char msg[200]);

// void esp32_uart_send_packet(packet_t* packet);

int esp32_uart_read_bpacket(uint8_t bpacketBuffer[BPACKET_BUFFER_LENGTH_BYTES]);

#endif // ESP32_UART_H