#ifndef BPACKET_UTILS_H
#define BPACKET_UTILS_H

/* C Library Includes */
#include <stdint.h>

/* Personal Includes */
#include "bpacket.h"
#include "custom_data_types.h"
#include "datetime.h"

uint8_t bp_utils_store_data(bpk_packet_t* Packet, cdt_dbl_16_t* data, uint8_t length);

uint8_t bp_utils_store_datetime(bpk_packet_t* Packet, dt_datetime_t* DateTime);

uint8_t bp_utils_get_datetime(bpk_packet_t* Packet, dt_datetime_t* DateTime);

uint8_t bpk_utils_confirm_params(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                                 bpk_request_t Request, bpk_code_t Code, uint8_t numBytes);

void bpk_utils_write_cdt_4_data_u8(bpk_packet_t* Bpacket, cdt_4_data_u8_t* Data);

void bpk_utils_read_cdt_4_data_u8(bpk_packet_t* Bpacket, cdt_4_data_u8_t* Data);

void bpk_utils_write_cdt_u8(bpk_packet_t* Bpacket, cdt_u8_t* Data);

void bpk_utils_read_cdt_u8(bpk_packet_t* Bpacket, cdt_u8_t* Data);

void bpk_utils_create_string_response(bpk_packet_t* Bpacket, bpk_code_t Code, const char* format, ...);

uint8_t bpk_utils_decode_non_data_byte(bpk_packet_t* Bpacket, uint8_t expectedByte, uint8_t byte);

#endif // BPACKET_UTILS_H