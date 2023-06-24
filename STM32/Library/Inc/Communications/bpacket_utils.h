#ifndef BPACKET_UTILS_H
#define BPACKET_UTILS_H

/* C Library Includes */
#include <stdint.h>

/* Personal Includes */
#include "bpacket.h"
#include "custom_data_types.h"
#include "datetime.h"

uint8_t bp_utils_store_data(bpk_t* Packet, cdt_dbl_16_t* data, uint8_t length);

uint8_t bpk_utils_write_time(bpk_t* Bpacket, dt_time_t* Time, uint8_t bpkIndex);
uint8_t bpk_utils_write_times(bpk_t* Bpacket, dt_time_t** Times, uint8_t numTimes);
uint8_t bpk_utils_read_times(bpk_t* Bpacket, dt_time_t** Times, uint8_t* numTimesRead);

uint8_t bpk_utils_write_datetimes(bpk_t* Packet, dt_datetime_t** DateTimes, uint8_t numDatetimes);
uint8_t bpk_utils_read_datetimes(bpk_t* Packet, dt_datetime_t** DateTimes, uint8_t* numDatetimesRead);

uint8_t bpk_utils_confirm_params(bpk_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                                 bpk_request_t Request, bpk_code_t Code, uint8_t numBytes);

void bpk_utils_write_cdt_u8_array_4(bpk_t* Bpacket, cdt_u8_array_4_t* Data);

void bpk_utils_read_cdt_u8_array_4(bpk_t* Bpacket, cdt_u8_array_4_t* Data);

void bpk_utils_write_cdt_u8(bpk_t* Bpacket, cdt_u8_t* Data, uint8_t numData);

void bpk_utils_read_cdt_u8(bpk_t* Bpacket, cdt_u8_t* Data, uint8_t numDataToRead);

void bpk_utils_create_string_response(bpk_t* Bpacket, bpk_code_t Code, const char* format, ...);

uint8_t bpk_utils_decode_non_data_byte(bpk_t* Bpacket, uint8_t expectedByte, uint8_t byte);

#endif // BPACKET_UTILS_H