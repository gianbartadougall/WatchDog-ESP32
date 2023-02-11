/**
 * @file com_ports.h
 * @author Gian Barta-Dougall
 * @brief Wrapper file for libSerialPort for opening com ports
 * @version 0.1
 * @date 2023-01-23
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef COM_PORTS_H
#define COM_PORTS_H

/* Library Includes */
#include <libserialport.h>

/* Personal Includes */
#include "bpacket.h"

uint8_t com_ports_open_connection(uint8_t address, uint8_t pingResponse);

uint8_t com_ports_close_connection(void);

uint8_t com_ports_send_bpacket(bpacket_t* bpacket);

int com_ports_read(void* buf, size_t count, unsigned int timeout_ms);

void comms_port_test(void);
#endif // COM_PORTS_H