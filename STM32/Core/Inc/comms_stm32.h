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

#define COMMS_STM32_BPACKET_READY (0x01 << 0)

void comms_add_to_buffer(uint8_t byte);

uint8_t comms_process_rxbuffer(bpacket_t* bpacket);

void comms_usart2_print_buffer(void);

void comms_stm32_init(void);

uint8_t comms_stm32_get_bpacket(bpacket_t* bpacket);

void comms_transmit(USART_TypeDef* usart, uint8_t* data, uint16_t numBytes);