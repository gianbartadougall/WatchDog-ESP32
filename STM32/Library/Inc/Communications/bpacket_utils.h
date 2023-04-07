#ifndef BPACKET_UTILS_H
#define BPACKET_UTILS_H

/* C Library Includes */
#include <stdint.h>

/* Personal Includes */
#include "bpacket.h"
#include "custom_data_types.h"
#include "datetime.h"

uint8_t bp_utils_store_data(bpk_packet_t* Packet, cdt_double16_t* data, uint8_t length);

uint8_t bp_utils_store_datetime(bpk_packet_t* Packet, dt_datetime_t* DateTime);

uint8_t bp_utils_get_datetime(bpk_packet_t* Packet, dt_datetime_t* DateTime);

#endif // BPACKET_UTILS_H