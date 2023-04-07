/* Personal Includes */
#include "bpacket_utils.h"
#include "utils.h"

uint8_t bp_utils_store_data(bpk_packet_t* Bpacket, cdt_double16_t* data, uint8_t length) {

    // Check there is enough room for all the data
    if ((length * 4) > BPACKET_MAX_NUM_DATA_BYTES) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Data;
        return FALSE;
    }

    for (uint8_t i = 0; i < length; i++) {
        uint8_t index                  = i * 4;
        Bpacket->Data.bytes[index + 0] = data[i].integer;
        Bpacket->Data.bytes[index + 1] = data[i].decimal >> 8;
        Bpacket->Data.bytes[index + 2] = data[i].decimal & 0xFF;
        Bpacket->Data.bytes[index + 3] = data[i].info;
    }

    Bpacket->Data.numBytes = (length * 4);

    return TRUE;
}

uint8_t bp_utils_store_datetimes(bpk_packet_t* Bpacket, dt_datetime_t* DateTime) {

    // Store the time
    Bpacket->Data.bytes[0] = DateTime->Time.second;
    Bpacket->Data.bytes[1] = DateTime->Time.minute;
    Bpacket->Data.bytes[2] = DateTime->Time.hour;

    // Store the date
    Bpacket->Data.bytes[3] = DateTime->Date.day;
    Bpacket->Data.bytes[4] = DateTime->Date.month;
    Bpacket->Data.bytes[5] = DateTime->Date.year >> 8;
    Bpacket->Data.bytes[6] = DateTime->Date.year & 0xFF;

    Bpacket->Data.numBytes = 7;

    return TRUE;
}

uint8_t bp_utils_get_datetimes(bpk_packet_t* Bpacket, dt_datetime_t* DateTime) {
    // Get the time
    DateTime->Time.second = Bpacket->Data.bytes[0];
    DateTime->Time.minute = Bpacket->Data.bytes[1];
    DateTime->Time.hour   = Bpacket->Data.bytes[2];

    // Get the date
    DateTime->Date.day   = Bpacket->Data.bytes[3];
    DateTime->Date.month = Bpacket->Data.bytes[4];
    DateTime->Date.year  = (Bpacket->Data.bytes[5] << 8) | (Bpacket->Data.bytes[6]);

    return TRUE;
}