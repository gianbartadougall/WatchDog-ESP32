/**
 * @file comms_stm32.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-01-29
 *
 * @copyright Copyright (c) 2023
 *
 */

/* STM32 Library Includes */
#include "stm32l432xx.h"

/* Personal Includes */
#include "bpacket.h"
#include "hardware_config.h"

#define NUM_BUFFERS 2

#define BUFFER_1_ID 0
#define BUFFER_2_ID 1
#define BUFFER_3_ID 3

#define BUFFER_1 UART_ESP32
#define BUFFER_2 UART_LOG
// #define BUFFER_3

void comms_add_to_buffer(uint8_t bufferId, uint8_t byte);

uint8_t comms_process_rxbuffer(uint8_t bufferId, bpk_packet_t* Bpacket);

void comms_print_buffer(uint8_t bufferId);

void comms_stm32_init(void);

uint8_t comms_stm32_get_bpacket(bpk_packet_t* Bpacket);

void comms_transmit(uint8_t bufferId, uint8_t* data, uint16_t numBytes);

uint8_t comms_stm32_request_pending(uint8_t bufferId);

void comms_open_connection(uint8_t bufferId);

void comms_close_connection(uint8_t bufferId);