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

void bpk_utils_write_cdt_4_data_u8(bpk_packet_t* Bpacket, cdt_4_data_u8_t* Data) {

    Bpacket->Data.bytes[0] = Data->bytes[0];
    Bpacket->Data.bytes[1] = Data->bytes[1];
    Bpacket->Data.bytes[2] = Data->bytes[2];
    Bpacket->Data.bytes[3] = Data->bytes[3];

    Bpacket->Data.numBytes = 4;
}

void bpk_utils_read_cdt_4_data_u8(bpk_packet_t* Bpacket, cdt_4_data_u8_t* Data) {

    Data->bytes[0] = Bpacket->Data.bytes[0];
    Data->bytes[1] = Bpacket->Data.bytes[1];
    Data->bytes[2] = Bpacket->Data.bytes[2];
    Data->bytes[3] = Bpacket->Data.bytes[3];
}

void bpk_utils_write_cdt_u8(bpk_packet_t* Bpacket, cdt_u8_t* Data) {
    Bpacket->Data.bytes[0] = Data->value;
    Bpacket->Data.numBytes = 1;
}

void bpk_utils_read_cdt_u8(bpk_packet_t* Bpacket, cdt_u8_t* Data) {
    Data->value = Bpacket->Data.bytes[0];
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