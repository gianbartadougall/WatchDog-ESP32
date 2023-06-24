/**
 * @file Bpacket.c
 * @author Gian Barta-Dougall
 * @brief
 *
 * The structure of a Bpacket is as follows
 * [START BYTE][PACKET DATA LENGTH][PACKET DATA][STOP BYTE]
 *
 * The packet data is structured as follows
 * [COMMAND][DATA]
 * @version 0.1
 * @date 2023-01-18
 *
 * @copyright Copyright (c) 2023
 *
 */

/* Public Includes */
#include <stdio.h> // Required for the use of NULL

/* Personal Includes */
#include "bpacket.h"
#include "chars.h"

/* Private Macros */

/* Private Variables */

const bpk_addr_send_t BPK_Addr_Send_Stm32 = {.val = BPK_ADDRESS_STM32};
const bpk_addr_send_t BPK_Addr_Send_Esp32 = {.val = BPK_ADDRESS_ESP32};
const bpk_addr_send_t BPK_Addr_Send_Maple = {.val = BPK_ADDRESS_MAPLE};

const bpk_addr_receive_t BPK_Addr_Receive_Stm32 = {.val = BPK_ADDRESS_STM32};
const bpk_addr_receive_t BPK_Addr_Receive_Esp32 = {.val = BPK_ADDRESS_ESP32};
const bpk_addr_receive_t BPK_Addr_Receive_Maple = {.val = BPK_ADDRESS_MAPLE};

const bpk_code_t BPK_Code_Error       = {.val = BPK_CODE_ERROR};
const bpk_code_t BPK_Code_Success     = {.val = BPK_CODE_SUCCESS};
const bpk_code_t BPK_Code_In_Progress = {.val = BPK_CODE_IN_PROGRESS};
const bpk_code_t BPK_Code_Unknown     = {.val = BPK_CODE_UNKNOWN};
const bpk_code_t BPK_Code_Execute     = {.val = BPK_CODE_EXECUTE};
const bpk_code_t BPK_Code_Todo        = {.val = BPK_CODE_TODO};
const bpk_code_t BPK_Code_Debug       = {.val = BPK_CODE_DEBUG};

const bpk_request_t BPK_Request_Help    = {.val = BPK_REQUEST_HELP};
const bpk_request_t BPK_Request_Ping    = {.val = BPK_REQUEST_PING};
const bpk_request_t BPK_Request_Status  = {.val = BPK_REQUEST_STATUS};
const bpk_request_t BPK_Request_Message = {.val = BPK_REQUEST_MESSAGE};

const bpk_request_t BPK_Req_List_Dir                 = {.val = BPK_REQ_LIST_DIR};
const bpk_request_t BPK_Req_Copy_File                = {.val = BPK_REQ_COPY_FILE};
const bpk_request_t BPK_Req_Take_Photo               = {.val = BPK_REQ_TAKE_PHOTO};
const bpk_request_t BPK_Req_Write_To_File            = {.val = BPK_REQ_WRITE_TO_FILE};
const bpk_request_t BPK_Req_Record_Data              = {.val = BPK_REQ_RECORD_DATA};
const bpk_request_t BPK_Req_Led_Red_On               = {.val = BPK_REQ_LED_RED_ON};
const bpk_request_t BPK_Req_Led_Red_Off              = {.val = BPK_REQ_LED_RED_OFF};
const bpk_request_t BPK_Req_Camera_View              = {.val = BPK_REQ_CAMERA_VIEW};
const bpk_request_t BPK_Req_Get_Datetime             = {.val = BPK_REQ_GET_DATETIME};
const bpk_request_t BPK_Req_Set_Datetime             = {.val = BPK_REQ_SET_DATETIME};
const bpk_request_t BPK_Req_Get_Camera_Settings      = {.val = BPK_REQ_GET_CAMERA_SETTINGS};
const bpk_request_t BPK_Req_Set_Camera_Settings      = {.val = BPK_REQ_SET_CAMERA_SETTINGS};
const bpk_request_t BPK_Req_Get_Camera_Capture_Times = {.val = BPK_REQ_GET_CAMERA_CAPTURE_TIMES};
const bpk_request_t BPK_Req_Set_Camera_Capture_Times = {.val = BPK_REQ_SET_CAMERA_CAPTURE_TIMES};
const bpk_request_t BPK_Req_Stream_Images            = {.val = BPK_REQ_STREAM_IMAGE};
const bpk_request_t BPK_Req_Esp32_On                 = {.val = BPK_REQ_ESP32_ON};
const bpk_request_t BPK_Req_Esp32_Off                = {.val = BPK_REQ_ESP32_OFF};

const bpk_error_code_t BPK_Err_Invalid_Sender     = {.val = BPK_ERR_INVALID_SENDER};
const bpk_error_code_t BPK_Err_Invalid_Receiver   = {.val = BPK_ERR_INVALID_RECEIVER};
const bpk_error_code_t BPK_Err_Invalid_Request    = {.val = BPK_ERR_INVLIAD_REQUEST};
const bpk_error_code_t BPK_Err_Invalid_Code       = {.val = BPK_ERR_INVALID_CODE};
const bpk_error_code_t BPK_Err_Invalid_Data       = {.val = BPK_ERR_INVALID_DATA};
const bpk_error_code_t BPK_Err_Invalid_Start_Byte = {.val = BPK_ERR_INVALD_START_BYTE};

const bpk_error_code_t BPK_Err_Invalid_Start_Time        = {.val = BPK_ERR_INVALID_START_TIME};
const bpk_error_code_t BPK_Err_Invalid_End_Time          = {.val = BPK_ERR_INVALID_END_TIME};
const bpk_error_code_t BPK_Err_Invalid_Interval_Time     = {.val = BPK_ERR_INVALID_INTERVAL_TIME};
const bpk_error_code_t BPK_Err_Invalid_Date              = {.val = BPK_ERR_INVALID_DATE};
const bpk_error_code_t BPK_Err_Invalid_Year              = {.val = BPK_ERR_INVALID_YEAR};
const bpk_error_code_t BPK_Err_Invalid_Bpacket_Size      = {.val = BPK_ERR_INVALID_PACKET_SIZE};
const bpk_error_code_t BPK_Err_Invalid_Camera_Resolution = {.val = BPK_ERR_INVALID_CAMERA_RESOLUTION};

/* Function Prototypes */
void bp_erase_all_data(bpk_t* Bpacket);

void bpk_create_circular_buffer(bpacket_circular_buffer_t* cBuffer, uint8_t* wIndex, uint8_t* rIndex, bpk_t* buffer) {
    cBuffer->wIndex = wIndex;
    cBuffer->rIndex = rIndex;

    for (int i = 0; i < BPACKET_CIRCULAR_BUFFER_SIZE; i++) {
        cBuffer->buffer[i] = (buffer + i);
    }
}

void bpk_increment_circular_buffer_index(uint8_t* wIndex) {
    (*wIndex)++;

    if (*wIndex >= BPACKET_CIRCULAR_BUFFER_SIZE) {
        *wIndex = 0;
    }
}

void bpk_increment_circ_buff_index(uint32_t* index, uint32_t maxBufferIndex) {

    if (*index == (maxBufferIndex - 1)) {
        *index = 0;
        return;
    }

    *index += 1;
}

uint8_t bpk_create(bpk_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender, bpk_request_t Request,
                   bpk_code_t Code, uint8_t numDataBytes, uint8_t* data) {

    bp_erase_all_data(Bpacket);

    Bpacket->Receiver      = Receiver;
    Bpacket->Sender        = Sender;
    Bpacket->Request       = Request;
    Bpacket->Code          = Code;
    Bpacket->Data.numBytes = numDataBytes;

    // Copy data into Bpacket
    if (data != NULL) {
        for (int i = 0; i < numDataBytes; i++) {
            Bpacket->Data.bytes[i] = data[i];
        }
    }

    return TRUE;
}

uint8_t bp_create_string_packet(bpk_t* Bpacket, const bpk_addr_receive_t Receiver, const bpk_addr_send_t Sender,
                                const bpk_request_t Request, const bpk_code_t Code, char* string) {
    Bpacket->Receiver = Receiver;
    Bpacket->Sender   = Sender;
    Bpacket->Code     = Code;
    Bpacket->Request  = Request;

    uint32_t numBytes = chars_get_num_bytes(string);

    if (numBytes > BPACKET_MAX_NUM_DATA_BYTES) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Data;
        return FALSE;
    }

    Bpacket->Data.numBytes = numBytes;

    // Copy data into Bpacket
    for (int i = 0; i < Bpacket->Data.numBytes; i++) {
        Bpacket->Data.bytes[i] = string[i];
    }

    return TRUE;
}

uint8_t bpk_create_sp(bpk_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender, bpk_request_t Request,
                      bpk_code_t Code, char* string) {

    if (chars_get_num_bytes(string) > BPACKET_MAX_NUM_DATA_BYTES) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Data;
    }

    Bpacket->Receiver      = Receiver;
    Bpacket->Sender        = Sender;
    Bpacket->Code          = Code;
    Bpacket->Request       = Request;
    Bpacket->Data.numBytes = chars_get_num_bytes(string);

    // Copy data into Bpacket
    for (int i = 0; i < Bpacket->Data.numBytes; i++) {
        Bpacket->Data.bytes[i] = string[i];
    }

    return TRUE;
}

void bpk_to_buffer(bpk_t* Bpacket, bpk_buffer_t* packetBuffer) {

    // Set the first two bytes to start bytes
    packetBuffer->buffer[0] = BPACKET_START_BYTE_UPPER;
    packetBuffer->buffer[1] = BPACKET_START_BYTE_LOWER;

    // Set the sender and receiver bytes
    packetBuffer->buffer[2] = Bpacket->Receiver.val;
    packetBuffer->buffer[3] = Bpacket->Sender.val;

    // Set the request and code
    packetBuffer->buffer[4] = Bpacket->Request.val;
    packetBuffer->buffer[5] = Bpacket->Code.val;

    // Set the length
    packetBuffer->buffer[6] = Bpacket->Data.numBytes;

    // Copy data into buffer
    int i;
    for (i = 0; i < Bpacket->Data.numBytes; i++) {
        packetBuffer->buffer[i + 7] = Bpacket->Data.bytes[i];
    }

    // // Set the stop bytes at the end
    // packetBuffer->buffer[i + 7] = BPACKET_STOP_BYTE_UPPER;
    // packetBuffer->buffer[i + 8] = BPACKET_STOP_BYTE_LOWER;

    packetBuffer->numBytes = Bpacket->Data.numBytes + BPACKET_NUM_NON_DATA_BYTES;
}

uint8_t bpk_buffer_decode(bpk_t* Bpacket, uint8_t data[BPACKET_BUFFER_LENGTH_BYTES]) {

    if ((data[0] != BPACKET_START_BYTE_UPPER) || (data[1] != BPACKET_START_BYTE_LOWER)) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Start_Byte;
        return FALSE;
    }

    if (bpk_set_receiver(Bpacket, data[2]) != TRUE) {
        return FALSE;
    }

    if (bpk_set_sender(Bpacket, data[3]) != TRUE) {
        return FALSE;
    }

    if (bpk_set_request(Bpacket, data[4]) != TRUE) {
        return FALSE;
    }

    if (bpk_set_code(Bpacket, data[5]) != TRUE) {
        return FALSE;
    }

    Bpacket->Data.numBytes = data[6];

    // Copy the data to the packet
    for (int i = 0; i < Bpacket->Data.numBytes; i++) {
        Bpacket->Data.bytes[i] = data[i + 7]; // The data starts on the 8th byte thus adding 7 (starting from 1)
    }

    return TRUE;
}

uint8_t bpk_set_sender(bpk_t* Bpacket, uint8_t sender) {

    switch (sender) {
        case BPK_ADDRESS_ESP32:
        case BPK_ADDRESS_STM32:
        case BPK_ADDRESS_MAPLE:
            Bpacket->Sender.val = sender;
            return TRUE;

        default:
            Bpacket->ErrorCode = BPK_Err_Invalid_Sender;
    }

    return FALSE;
}

uint8_t bpk_set_receiver(bpk_t* Bpacket, uint8_t receiver) {

    switch (receiver) {
        case BPK_ADDRESS_ESP32:
        case BPK_ADDRESS_STM32:
        case BPK_ADDRESS_MAPLE:
            Bpacket->Receiver.val = receiver;
            return TRUE;

        default:
            Bpacket->ErrorCode = BPK_Err_Invalid_Receiver;
    }

    return FALSE;
}

uint8_t bpk_set_code(bpk_t* Bpacket, uint8_t code) {

    switch (code) {
        case BPK_CODE_ERROR:
        case BPK_CODE_SUCCESS:
        case BPK_CODE_IN_PROGRESS:
        case BPK_CODE_UNKNOWN:
        case BPK_CODE_EXECUTE:
        case BPK_CODE_TODO:
        case BPK_CODE_DEBUG:
            Bpacket->Code.val = code;
            return TRUE;

        default:
            Bpacket->ErrorCode = BPK_Err_Invalid_Code;
    }

    return FALSE;
}

uint8_t bpk_set_request(bpk_t* Bpacket, uint8_t request) {

    switch (request) {
        case BPK_REQUEST_HELP:
        case BPK_REQUEST_PING:
        case BPK_REQUEST_STATUS:
        case BPK_REQUEST_MESSAGE:
        case BPK_REQ_LIST_DIR:
        case BPK_REQ_COPY_FILE:
        case BPK_REQ_TAKE_PHOTO:
        case BPK_REQ_WRITE_TO_FILE:
        case BPK_REQ_RECORD_DATA:
        case BPK_REQ_LED_RED_ON:
        case BPK_REQ_LED_RED_OFF:
        case BPK_REQ_CAMERA_VIEW:
        case BPK_REQ_GET_DATETIME:
        case BPK_REQ_SET_DATETIME:
        case BPK_REQ_GET_CAMERA_SETTINGS:
        case BPK_REQ_SET_CAMERA_SETTINGS:
        case BPK_REQ_GET_CAMERA_CAPTURE_TIMES:
        case BPK_REQ_SET_CAMERA_CAPTURE_TIMES:
        case BPK_REQ_STREAM_IMAGE:
        case BPK_REQ_ESP32_ON:
        case BPK_REQ_ESP32_OFF:
            Bpacket->Request.val = request;
            return TRUE;

        default:
            Bpacket->ErrorCode = BPK_Err_Invalid_Request;
    }

    return FALSE;
}

void bpk_data_to_string(bpk_t* Bpacket, bpacket_char_array_t* bpacketCharArray) {

    bpacketCharArray->numBytes = Bpacket->Data.numBytes;

    if (Bpacket->Data.numBytes == 0) {
        bpacketCharArray->string[0] = '\0';
        return;
    }

    // Copy the data to the packet.
    int i;
    for (i = 0; i < Bpacket->Data.numBytes; i++) {
        bpacketCharArray->string[i] = Bpacket->Data.bytes[i];
    }

    bpacketCharArray->string[i] = '\0';
}

void bpk_get_info(bpk_t* Bpacket, char* string) {
    sprintf(string, "Receiver: %i Sender: %i Request: %i Code: %i num bytes: %i\r\n", Bpacket->Receiver.val,
            Bpacket->Sender.val, Bpacket->Request.val, Bpacket->Code.val, Bpacket->Data.numBytes);
}

uint8_t bpk_send_data(void (*transmit_bpacket)(uint8_t* data, uint16_t bufferNumBytes), bpk_addr_receive_t Receiver,
                      bpk_addr_send_t Sender, bpk_request_t Request, uint8_t* data, uint32_t numBytesToSend) {

    // Create the Bpacket
    bpk_t Bpacket;

    if (bpk_create(&Bpacket, Receiver, Sender, Request, BPK_Code_In_Progress, 0, NULL) != TRUE) {
        return FALSE;
    }

    // Set the number of bytes to the maximum
    Bpacket.Data.numBytes = BPACKET_MAX_NUM_DATA_BYTES;
    bpk_buffer_t bpacketBuffer;

    uint32_t bytesSent = 0;
    uint32_t index     = 0;

    while (bytesSent < numBytesToSend) {

        if (index < BPACKET_MAX_NUM_DATA_BYTES) {
            Bpacket.Data.bytes[index++] = data[bytesSent++];
            continue;
        }

        // Send the Bpacket
        bpk_to_buffer(&Bpacket, &bpacketBuffer);
        transmit_bpacket(bpacketBuffer.buffer, bpacketBuffer.numBytes);

        // Reset the index
        index = 0;
    }

    // Send the last Bpacket
    if (index != 0) {

        // Update Bpacket info
        Bpacket.Data.numBytes = index;
        Bpacket.Code          = BPK_Code_Success;

        // Send the Bpacket
        bpk_to_buffer(&Bpacket, &bpacketBuffer);
        transmit_bpacket(bpacketBuffer.buffer, bpacketBuffer.numBytes);
    }

    return TRUE;
}

void bpk_convert_to_response(bpk_t* Bpacket, bpk_code_t Code, uint8_t numBytes, uint8_t* data) {

    // Swap the addresses
    bpk_addr_send_t Sender = Bpacket->Sender;
    Bpacket->Sender.val    = Bpacket->Receiver.val;
    Bpacket->Receiver.val  = Sender.val;

    // Update the code
    Bpacket->Code = Code;

    bp_erase_all_data(Bpacket);

    // Copy data into Bpacket
    if (data != NULL) {
        for (int i = 0; i < numBytes; i++) {
            Bpacket->Data.bytes[i] = data[i];
        }
    }
}

void bp_erase_all_data(bpk_t* Bpacket) {
    Bpacket->Data.numBytes = 0;
    for (int i = 0; i < Bpacket->Data.numBytes; i++) {
        Bpacket->Data.bytes[i] = BP_DEFAULT_DATA_VALUE;
    }
}

void bpk_create_response(bpk_t* Bpacket, bpk_code_t Code) {

    // Swap the addresses
    uint8_t senderAddress = Bpacket->Sender.val;
    Bpacket->Sender.val   = Bpacket->Receiver.val;
    Bpacket->Receiver.val = senderAddress;

    // Update the code if the user requests
    Bpacket->Code = Code;

    bp_erase_all_data(Bpacket);
}

uint8_t bpk_create_string_response(bpk_t* Bpacket, bpk_code_t Code, char* string) {

    bpk_create_response(Bpacket, Code);

    uint32_t numBytes = chars_get_num_bytes(string);

    if (numBytes > BPACKET_MAX_NUM_DATA_BYTES) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Data;
        return FALSE;
    }

    Bpacket->Data.numBytes = numBytes;

    // Copy data into Bpacket
    for (int i = 0; i < Bpacket->Data.numBytes; i++) {
        Bpacket->Data.bytes[i] = string[i];
    }

    return TRUE;
}

void bpk_swap_address(bpk_t* Bpacket) {
    uint8_t senderAddress = Bpacket->Sender.val;
    Bpacket->Sender.val   = Bpacket->Receiver.val;
    Bpacket->Receiver.val = senderAddress;
}

void bpk_set_sender_receiver(bpk_t* Bpacket, bpk_addr_send_t Sender, bpk_addr_receive_t Receiver) {
    Bpacket->Sender   = Sender;
    Bpacket->Receiver = Receiver;
}

void bpk_reset(bpk_t* Bpacket) {
    Bpacket->Receiver.val = 0;
    Bpacket->Sender.val   = 0;
    Bpacket->Request.val  = 0;
    Bpacket->Code.val     = 0;
    bp_erase_all_data(Bpacket);
}