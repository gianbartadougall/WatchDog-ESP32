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

const bpk_request_t BPK_WD_Request_List_Dir                  = {.val = BPK_WD_REQUEST_LIST_DIR};
const bpk_request_t BPK_WD_Request_Copy_File                 = {.val = BPK_WD_REQUEST_COPY_FILE};
const bpk_request_t BPK_WD_Request_Take_Photo                = {.val = BPK_WD_REQUEST_TAKE_PHOTO};
const bpk_request_t BPK_WD_Request_Write_To_File             = {.val = BPK_WD_REQUEST_WRITE_TO_FILE};
const bpk_request_t BPK_WD_Request_Record_Data               = {.val = BPK_WD_REQUEST_RECORD_DATA};
const bpk_request_t BPK_WD_Request_Led_Red_On                = {.val = BPK_WD_REQUEST_LED_RED_ON};
const bpk_request_t BPK_WD_Request_Led_Red_Off               = {.val = BPK_WD_REQUEST_LED_RED_OFF};
const bpk_request_t BPK_WD_Request_Camera_View               = {.val = BPK_WD_REQUEST_CAMERA_VIEW};
const bpk_request_t BPK_WD_Request_Get_Datetime              = {.val = BPK_WD_REQUEST_GET_DATETIME};
const bpk_request_t BPK_WD_Request_Set_Datetime              = {.val = BPK_WD_REQUEST_SET_DATETIME};
const bpk_request_t BPK_WD_Request_Get_Camera_Settings       = {.val = BPK_WD_REQUEST_GET_CAMERA_SETTINGS};
const bpk_request_t BPK_WD_Request_Set_Camera_Settings       = {.val = BPK_WD_REQUEST_SET_CAMERA_SETTINGS};
const bpk_request_t BPK_WD_Request_Get_Capture_Time_Settings = {.val = BPK_WD_REQUEST_GET_CAPTURE_TIME_SETTINGS};
const bpk_request_t BPK_WD_Request_Set_Capture_Time_Settings = {.val = BPK_WD_REQUEST_SET_CAPTURE_TIME_SETTINGS};
const bpk_request_t BPK_WD_Request_Stream_Images             = {.val = BPK_WD_REQUEST_STREAM_IMAGE};
const bpk_request_t BPK_WD_Request_Turn_On                   = {.val = BPK_WD_REQUEST_TURN_ON};
const bpk_request_t BPK_WD_Request_Turn_Off                  = {.val = BPK_WD_REQUEST_TURN_OFF};

const bpk_error_code_t BPK_Err_Invalid_Sender     = {.val = BPK_ERR_INVALID_SENDER};
const bpk_error_code_t BPK_Err_Invalid_Receiver   = {.val = BPK_ERR_INVALID_RECEIVER};
const bpk_error_code_t BPK_Err_Invalid_Request    = {.val = BPK_ERR_INVLIAD_REQUEST};
const bpk_error_code_t BPK_Err_Invalid_Code       = {.val = BPK_ERR_INVALID_CODE};
const bpk_error_code_t BPK_Err_Invalid_Data       = {.val = BPK_ERR_INVALID_DATA};
const bpk_error_code_t BPK_Err_Invalid_Start_Byte = {.val = BPK_ERR_INVALD_START_BYTE};

const bpk_error_code_t BPK_Err_Invalid_Start_Time    = {.val = BPK_ERR_INVALID_START_TIME};
const bpk_error_code_t BPK_Err_Invalid_End_Time      = {.val = BPK_ERR_INVALID_END_TIME};
const bpk_error_code_t BPK_Err_Invalid_Interval_Time = {.val = BPK_ERR_INVALID_INTERVAL_TIME};
const bpk_error_code_t BPK_Err_Invalid_Date          = {.val = BPK_ERR_INVALID_DATE};
const bpk_error_code_t BPK_Err_Invalid_Year          = {.val = BPK_ERR_INVALID_YEAR};
const bpk_error_code_t BPK_Err_Invalid_Bpacket_Size  = {.val = BPK_ERR_INVALID_PACKET_SIZE};

/* Function Prototypes */
void bp_erase_all_data(bpk_packet_t* Bpacket);

void bpacket_create_circular_buffer(bpacket_circular_buffer_t* cBuffer, uint8_t* wIndex, uint8_t* rIndex,
                                    bpk_packet_t* buffer) {
    cBuffer->wIndex = wIndex;
    cBuffer->rIndex = rIndex;

    for (int i = 0; i < BPACKET_CIRCULAR_BUFFER_SIZE; i++) {
        cBuffer->buffer[i] = (buffer + i);
    }
}

void bpacket_increment_circular_buffer_index(uint8_t* wIndex) {
    (*wIndex)++;

    if (*wIndex >= BPACKET_CIRCULAR_BUFFER_SIZE) {
        *wIndex = 0;
    }
}

void bpacket_increment_circ_buff_index(uint32_t* index, uint32_t maxBufferIndex) {

    if (*index == (maxBufferIndex - 1)) {
        *index = 0;
        return;
    }

    *index += 1;
}

uint8_t bpacket_create_p(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                         bpk_request_t Request, bpk_code_t Code, uint8_t numDataBytes, uint8_t* data) {

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

uint8_t bp_create_packet(bpk_packet_t* Bpacket, const bpk_addr_receive_t Receiver, const bpk_addr_send_t Sender,
                         const bpk_request_t Request, const bpk_code_t Code, uint8_t numDataBytes, uint8_t* data) {

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

uint8_t bp_create_string_packet(bpk_packet_t* Bpacket, const bpk_addr_receive_t Receiver, const bpk_addr_send_t Sender,
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

uint8_t bpacket_create_sp(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                          bpk_request_t Request, bpk_code_t Code, char* string) {

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

void bpacket_to_buffer(bpk_packet_t* Bpacket, bpk_buffer_t* packetBuffer) {

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

    // Set the stop bytes at the end
    packetBuffer->buffer[i + 7] = BPACKET_STOP_BYTE_UPPER;
    packetBuffer->buffer[i + 8] = BPACKET_STOP_BYTE_LOWER;

    packetBuffer->numBytes = Bpacket->Data.numBytes + BPACKET_NUM_NON_DATA_BYTES;
}

uint8_t bpacket_buffer_decode(bpk_packet_t* Bpacket, uint8_t data[BPACKET_BUFFER_LENGTH_BYTES]) {

    if ((data[0] != BPACKET_START_BYTE_UPPER) || (data[1] != BPACKET_START_BYTE_LOWER)) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Start_Byte;
        return FALSE;
    }

    if (bpk_address_get_receiver(Bpacket, data[2]) != TRUE) {
        return FALSE;
    }

    if (bpk_address_get_sender(Bpacket, data[3]) != TRUE) {
        return FALSE;
    }

    if (bpk_address_get_request(Bpacket, data[4]) != TRUE) {
        return FALSE;
    }

    if (bpk_address_get_code(Bpacket, data[5]) != TRUE) {
        return FALSE;
    }

    Bpacket->Data.numBytes = data[6];

    // Copy the data to the packet
    for (int i = 0; i < Bpacket->Data.numBytes; i++) {
        Bpacket->Data.bytes[i] = data[i + 7]; // The data starts on the 8th byte thus adding 7 (starting from 1)
    }

    return TRUE;
}

uint8_t bpk_address_get_sender(bpk_packet_t* Bpacket, uint8_t sender) {

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

uint8_t bpk_address_get_receiver(bpk_packet_t* Bpacket, uint8_t receiver) {

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

uint8_t bpk_address_get_code(bpk_packet_t* Bpacket, uint8_t code) {

    switch (code) {
        case BPK_ADDRESS_ESP32:
        case BPK_ADDRESS_STM32:
        case BPK_ADDRESS_MAPLE:
            Bpacket->Code.val = code;
            return TRUE;

        default:
            Bpacket->ErrorCode = BPK_Err_Invalid_Code;
    }

    return FALSE;
}

uint8_t bpk_address_get_request(bpk_packet_t* Bpacket, uint8_t request) {

    switch (request) {
        case BPK_REQUEST_HELP:
        case BPK_REQUEST_PING:
        case BPK_REQUEST_STATUS:
        case BPK_REQUEST_MESSAGE:
        case BPK_WD_REQUEST_LIST_DIR:
        case BPK_WD_REQUEST_COPY_FILE:
        case BPK_WD_REQUEST_TAKE_PHOTO:
        case BPK_WD_REQUEST_WRITE_TO_FILE:
        case BPK_WD_REQUEST_RECORD_DATA:
        case BPK_WD_REQUEST_LED_RED_ON:
        case BPK_WD_REQUEST_LED_RED_OFF:
        case BPK_WD_REQUEST_CAMERA_VIEW:
        case BPK_WD_REQUEST_GET_DATETIME:
        case BPK_WD_REQUEST_SET_DATETIME:
        case BPK_WD_REQUEST_GET_CAMERA_SETTINGS:
        case BPK_WD_REQUEST_SET_CAMERA_SETTINGS:
        case BPK_WD_REQUEST_GET_CAPTURE_TIME_SETTINGS:
        case BPK_WD_REQUEST_SET_CAPTURE_TIME_SETTINGS:
        case BPK_WD_REQUEST_STREAM_IMAGE:
        case BPK_WD_REQUEST_TURN_ON:
        case BPK_WD_REQUEST_TURN_OFF:
            Bpacket->Request.val = request;
            return TRUE;

        default:
            Bpacket->ErrorCode = BPK_Err_Invalid_Request;
    }

    return FALSE;
}

void bpacket_data_to_string(bpk_packet_t* Bpacket, bpacket_char_array_t* bpacketCharArray) {

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

void bpacket_get_info(bpk_packet_t* Bpacket, char* string) {
    sprintf(string, "Receiver: %i Sender: %i Request: %i Code: %i num bytes: %i\r\n", Bpacket->Receiver.val,
            Bpacket->Sender.val, Bpacket->Request.val, Bpacket->Code.val, Bpacket->Data.numBytes);
}

uint8_t bpacket_send_data(void (*transmit_bpacket)(uint8_t* data, uint16_t bufferNumBytes), bpk_addr_receive_t Receiver,
                          bpk_addr_send_t Sender, bpk_request_t Request, uint8_t* data, uint32_t numBytesToSend) {

    // Create the Bpacket
    bpk_packet_t Bpacket;

    if (bpacket_create_p(&Bpacket, Receiver, Sender, Request, BPK_Code_In_Progress, 0, NULL) != TRUE) {
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
        bpacket_to_buffer(&Bpacket, &bpacketBuffer);
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
        bpacket_to_buffer(&Bpacket, &bpacketBuffer);
        transmit_bpacket(bpacketBuffer.buffer, bpacketBuffer.numBytes);
    }

    return TRUE;
}

uint8_t bpacket_confirm_values(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                               bpk_request_t Request, bpk_code_t Code, uint8_t numBytes, char* errMsg) {

    if (Bpacket->Receiver.val != Receiver.val) {
        sprintf(errMsg, "Invalid receiver. Expected %i but got %i\r\n", Receiver.val, Bpacket->Receiver.val);
        return FALSE;
    }

    if (Bpacket->Sender.val != Sender.val) {
        sprintf(errMsg, "Invalid Sender. Expected %i but got %i\r\n", Sender.val, Bpacket->Sender.val);
        return FALSE;
    }

    if (Bpacket->Request.val != Request.val) {
        sprintf(errMsg, "Invalid Request. Expected %i but got %i\r\n", Request.val, Bpacket->Request.val);
        return FALSE;
    }

    if (Bpacket->Code.val != Code.val) {
        sprintf(errMsg, "Invalid Code. Expected %i but got %i\r\n", Code.val, Bpacket->Code.val);
        return FALSE;
    }

    if (Bpacket->Data.numBytes != numBytes) {
        sprintf(errMsg, "Invalid num bytes. Expected %i but got %i\r\n", numBytes, Bpacket->Data.numBytes);
        return FALSE;
    }

    return TRUE;
}

void bp_convert_to_response(bpk_packet_t* Bpacket, bpk_code_t Code, uint8_t numBytes, uint8_t* data) {

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

void bp_erase_all_data(bpk_packet_t* Bpacket) {
    for (int i = 0; i < Bpacket->Data.numBytes; i++) {
        Bpacket->Data.bytes[i] = BP_DEFAULT_DATA_VALUE;
    }
}

uint8_t bp_create_string_response(bpk_packet_t* Bpacket, bpk_code_t Code, char* string) {

    // Swap the addresses
    uint8_t senderAddress = Bpacket->Sender.val;
    Bpacket->Sender.val   = Bpacket->Receiver.val;
    Bpacket->Receiver.val = senderAddress;

    // Update the code
    Bpacket->Code = Code;

    bp_erase_all_data(Bpacket);

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

void bpk_swap_address(bpk_packet_t* Bpacket) {
    uint8_t senderAddress = Bpacket->Sender.val;
    Bpacket->Sender.val   = Bpacket->Receiver.val;
    Bpacket->Receiver.val = senderAddress;
}