/* C Library Includes */
#include <stdarg.h>
#include <stdio.h>

/* Personal Includes */
#include "bpacket_utils.h"
#include "utils.h"

uint8_t bp_utils_store_data(bpk_packet_t* Bpacket, cdt_dbl_16_t* data, uint8_t length) {

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

uint8_t bpk_utils_write_time(bpk_packet_t* Bpacket, dt_time_t* Time, uint8_t bpkIndex) {

    // Confirm the index is valid
    if ((bpkIndex * 3) > BPACKET_MAX_NUM_DATA_BYTES) {
        return FALSE;
    }

    // Calcualte the position in the byte array corresponding to the given index
    uint8_t index = bpkIndex * 3;

    // Extract the data
    Bpacket->Data.bytes[index + 0] = Time->second;
    Bpacket->Data.bytes[index + 1] = Time->minute;
    Bpacket->Data.bytes[index + 2] = Time->hour;

    return TRUE;
}

uint8_t bpk_utils_write_times(bpk_packet_t* Bpacket, dt_time_t** Times, uint8_t numTimes) {

    if ((numTimes * 3) > BPACKET_MAX_NUM_DATA_BYTES) {
        return FALSE;
    }

    for (uint8_t i = 0; i < numTimes; i++) {
        uint8_t index = i * 3;
        // Store the time
        Bpacket->Data.bytes[index + 0] = Times[i]->second;
        Bpacket->Data.bytes[index + 1] = Times[i]->minute;
        Bpacket->Data.bytes[index + 2] = Times[i]->hour;
    }

    Bpacket->Data.numBytes = 3 * numTimes;

    return TRUE;
}

uint8_t bpk_utils_read_times(bpk_packet_t* Bpacket, dt_time_t** Times, uint8_t* numTimesRead) {

    // Confirm the data in the bpacket is a multiple of datetime
    if ((Bpacket->Data.numBytes % 3) != 0) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Data;
        return FALSE;
    }

    *numTimesRead = Bpacket->Data.numBytes / 3;

    for (uint8_t i = 0; i < *numTimesRead; i++) {
        uint8_t index = i * 3;

        // Get the time
        Times[i]->second = Bpacket->Data.bytes[index + 0];
        Times[i]->minute = Bpacket->Data.bytes[index + 1];
        Times[i]->hour   = Bpacket->Data.bytes[index + 2];
    }

    return TRUE;
}

uint8_t bpk_utils_write_datetimes(bpk_packet_t* Bpacket, dt_datetime_t** DateTimes, uint8_t numDatetimes) {

    // Confirm the number of datetimes to write can fit in the bpacket
    if ((numDatetimes * 7) > BPACKET_MAX_NUM_DATA_BYTES) {
        return FALSE;
    }

    for (uint8_t i = 0; i < numDatetimes; i++) {
        uint8_t index = i * 7;
        // Store the time
        Bpacket->Data.bytes[index + 0] = DateTimes[i]->Time.second;
        Bpacket->Data.bytes[index + 1] = DateTimes[i]->Time.minute;
        Bpacket->Data.bytes[index + 2] = DateTimes[i]->Time.hour;

        // Store the date
        Bpacket->Data.bytes[index + 3] = DateTimes[i]->Date.day;
        Bpacket->Data.bytes[index + 4] = DateTimes[i]->Date.month;
        Bpacket->Data.bytes[index + 5] = DateTimes[i]->Date.year >> 8;
        Bpacket->Data.bytes[index + 6] = DateTimes[i]->Date.year & 0xFF;
    }

    Bpacket->Data.numBytes = 7 * numDatetimes;

    return TRUE;
}

uint8_t bpk_utils_read_datetimes(bpk_packet_t* Bpacket, dt_datetime_t** DateTimes, uint8_t* numDatetimesRead) {

    // Confirm the data in the bpacket is a multiple of datetime
    if ((Bpacket->Data.numBytes % 7) != 0) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Data;
        return FALSE;
    }

    *numDatetimesRead = Bpacket->Data.numBytes / 7;

    for (uint8_t i = 0; i < *numDatetimesRead; i++) {
        uint8_t index = i * 7;

        // Get the time
        DateTimes[i]->Time.second = Bpacket->Data.bytes[index + 0];
        DateTimes[i]->Time.minute = Bpacket->Data.bytes[index + 1];
        DateTimes[i]->Time.hour   = Bpacket->Data.bytes[index + 2];

        // Get the date
        DateTimes[i]->Date.day   = Bpacket->Data.bytes[index + 3];
        DateTimes[i]->Date.month = Bpacket->Data.bytes[index + 4];
        DateTimes[i]->Date.year  = (Bpacket->Data.bytes[index + 5] << 8) | (Bpacket->Data.bytes[index + 6]);
    }

    return TRUE;
}

uint8_t bpk_utils_confirm_params(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                                 bpk_request_t Request, bpk_code_t Code, uint8_t numBytes) {

    if (Bpacket->Receiver.val != Receiver.val) {
        return FALSE;
    }

    if (Bpacket->Sender.val != Sender.val) {
        return FALSE;
    }

    if (Bpacket->Request.val != Request.val) {
        return FALSE;
    }

    if (Bpacket->Code.val != Code.val) {
        return FALSE;
    }

    if (Bpacket->Data.numBytes != numBytes) {
        return FALSE;
    }

    return TRUE;
}

void bpk_utils_write_cdt_u8(bpk_packet_t* Bpacket, cdt_u8_t* Data, uint8_t numData) {

    for (uint8_t i = 0; i < numData; i++) {
        Bpacket->Data.bytes[i] = Data->value;
    }

    Bpacket->Data.numBytes = numData;
}

void bpk_utils_read_cdt_u8(bpk_packet_t* Bpacket, cdt_u8_t* Data, uint8_t numDataToRead) {

    for (uint8_t i = 0; i < numDataToRead; i++) {
        Data[i].value = Bpacket->Data.bytes[0];
    }
}

void bpk_utils_write_cdt_u8_array_4(bpk_packet_t* Bpacket, cdt_u8_array_4_t* Data) {

    Bpacket->Data.bytes[0] = Data->bytes[0];
    Bpacket->Data.bytes[1] = Data->bytes[1];
    Bpacket->Data.bytes[2] = Data->bytes[2];
    Bpacket->Data.bytes[3] = Data->bytes[3];

    Bpacket->Data.numBytes = 4;
}

void bpk_utils_read_cdt_u8_array_4(bpk_packet_t* Bpacket, cdt_u8_array_4_t* Data) {

    Data->bytes[0] = Bpacket->Data.bytes[0];
    Data->bytes[1] = Bpacket->Data.bytes[1];
    Data->bytes[2] = Bpacket->Data.bytes[2];
    Data->bytes[3] = Bpacket->Data.bytes[3];
}

void bpk_utils_create_string_response(bpk_packet_t* Bpacket, bpk_code_t Code, const char* format, ...) {
    char msg[120];

    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    bpk_create_string_response(Bpacket, Code, msg);
}

uint8_t bpk_utils_decode_non_data_byte(bpk_packet_t* Bpacket, uint8_t expectedByte, uint8_t byte) {

    if ((expectedByte == BPK_BYTE_START_BYTE_UPPER) || (expectedByte == BPK_BYTE_START_BYTE_LOWER)) {
        if (byte == expectedByte) {
            return TRUE;
        }
    }

    if (expectedByte == BPK_BYTE_ADDRESS_RECEIVER) {
        return bpk_set_receiver(Bpacket, byte);
    }

    if (expectedByte == BPK_BYTE_ADDRESS_SENDER) {
        return bpk_set_sender(Bpacket, byte);
    }

    if (expectedByte == BPK_BYTE_REQUEST) {
        return bpk_set_request(Bpacket, byte);
    }

    if (expectedByte == BPK_BYTE_CODE) {
        return bpk_set_code(Bpacket, byte);
    }

    return FALSE;
}

void bpk_utils_init_expected_byte_buffer(uint8_t byteBuffer[8]) {
    byteBuffer[0] = BPK_BYTE_START_BYTE_UPPER;
    byteBuffer[1] = BPK_BYTE_START_BYTE_LOWER;
    byteBuffer[2] = BPK_BYTE_ADDRESS_RECEIVER;
    byteBuffer[3] = BPK_BYTE_ADDRESS_SENDER;
    byteBuffer[4] = BPK_BYTE_REQUEST;
    byteBuffer[5] = BPK_BYTE_CODE;
    byteBuffer[6] = BPK_BYTE_LENGTH;
    byteBuffer[7] = BPK_BYTE_DATA;
}