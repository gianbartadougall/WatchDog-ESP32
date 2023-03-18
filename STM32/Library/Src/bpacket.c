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
#include "Bpacket.h"
#include "chars.h"

/* Function Prototypes */
void bp_erase_all_data(bpacket_t* Bpacket);

void bpacket_create_circular_buffer(bpacket_circular_buffer_t* cBuffer, uint8_t* wIndex, uint8_t* rIndex,
                                    bpacket_t* buffer) {
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

uint8_t bpacket_create_p(bpacket_t* Bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                         uint8_t numDataBytes, uint8_t* data) {

    BPACKET_ASSERT_VALID_RECEIVER(receiver);
    BPACKET_ASSERT_VALID_SENDER(sender);
    BPACKET_ASSERT_VALID_REQUEST(request);
    BPACKET_ASSERT_VALID_CODE(code);

    Bpacket->receiver = receiver;
    Bpacket->sender   = sender;
    Bpacket->request  = request;
    Bpacket->code     = code;
    Bpacket->numBytes = numDataBytes;

    // Copy data into Bpacket
    if (data != NULL) {
        for (int i = 0; i < numDataBytes; i++) {
            Bpacket->bytes[i] = data[i];
        }
    }

    return TRUE;
}

uint8_t bp_create_packet(bpacket_t* Bpacket, const bp_receive_address_t RAddress, const bp_send_address_t SAddress,
                         const bp_request_t Request, const bp_code_t Code, uint8_t numDataBytes, uint8_t* data) {

    bp_erase_all_data(Bpacket);

    Bpacket->receiver = RAddress.val;
    Bpacket->sender   = SAddress.val;
    Bpacket->request  = Request.val;
    Bpacket->code     = Code.val;
    Bpacket->numBytes = numDataBytes;

    // Copy data into Bpacket
    if (data != NULL) {
        for (int i = 0; i < numDataBytes; i++) {
            Bpacket->bytes[i] = data[i];
        }
    }

    return TRUE;
}

uint8_t bp_create_string_packet(bpacket_t* Bpacket, const bp_receive_address_t RAddress,
                                const bp_send_address_t SAddress, const bp_request_t Request, const bp_code_t Code,
                                char* string) {
    Bpacket->receiver = RAddress.val;
    Bpacket->sender   = SAddress.val;
    Bpacket->code     = Code.val;
    Bpacket->request  = Request.val;

    uint32_t numBytes = chars_get_num_bytes(string);

    if (numBytes > BPACKET_MAX_NUM_DATA_BYTES) {
        Bpacket->status = BP_STATUS_STRING_PACKET_CREATION_FAILED.val;
        return FALSE;
    }

    Bpacket->numBytes = numBytes;

    // Copy data into Bpacket
    for (int i = 0; i < Bpacket->numBytes; i++) {
        Bpacket->bytes[i] = string[i];
    }

    return TRUE;
}

uint8_t bpacket_create_sp(bpacket_t* Bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                          char* string) {

    BPACKET_ASSERT_VALID_RECEIVER(receiver);
    BPACKET_ASSERT_VALID_SENDER(sender);
    BPACKET_ASSERT_VALID_REQUEST(request);
    BPACKET_ASSERT_VALID_CODE(code);
    BPACKET_ASSERT_VALID_NUM_BYTES(chars_get_num_bytes(string));

    Bpacket->receiver = receiver;
    Bpacket->sender   = sender;
    Bpacket->code     = code;
    Bpacket->request  = request;
    Bpacket->numBytes = chars_get_num_bytes(string);

    // Copy data into Bpacket
    for (int i = 0; i < Bpacket->numBytes; i++) {
        Bpacket->bytes[i] = string[i];
    }

    return TRUE;
}

void bpacket_to_buffer(bpacket_t* Bpacket, bpacket_buffer_t* packetBuffer) {

    // Set the first two bytes to start bytes
    packetBuffer->buffer[0] = BPACKET_START_BYTE_UPPER;
    packetBuffer->buffer[1] = BPACKET_START_BYTE_LOWER;

    // Set the sender and receiver bytes
    packetBuffer->buffer[2] = Bpacket->receiver;
    packetBuffer->buffer[3] = Bpacket->sender;

    // Set the request and code
    packetBuffer->buffer[4] = Bpacket->request;
    packetBuffer->buffer[5] = Bpacket->code;

    // Set the length
    packetBuffer->buffer[6] = Bpacket->numBytes;

    // Copy data into buffer
    int i;
    for (i = 0; i < Bpacket->numBytes; i++) {
        packetBuffer->buffer[i + 7] = Bpacket->bytes[i];
    }

    // Set the stop bytes at the end
    packetBuffer->buffer[i + 7] = BPACKET_STOP_BYTE_UPPER;
    packetBuffer->buffer[i + 8] = BPACKET_STOP_BYTE_LOWER;

    packetBuffer->numBytes = Bpacket->numBytes + BPACKET_NUM_NON_DATA_BYTES;
}

uint8_t bpacket_buffer_decode(bpacket_t* Bpacket, uint8_t data[BPACKET_BUFFER_LENGTH_BYTES]) {

    uint8_t receiver     = data[2];
    uint8_t sender       = data[3];
    uint8_t request      = data[4];
    uint8_t code         = data[5];
    uint8_t numDataBytes = data[6];

    BPACKET_ASSERT_VALID_START_BYTE(data[0], data[1]);
    BPACKET_ASSERT_VALID_RECEIVER(receiver);
    BPACKET_ASSERT_VALID_SENDER(sender);
    BPACKET_ASSERT_VALID_REQUEST(request);
    BPACKET_ASSERT_VALID_CODE(code);

    Bpacket->receiver = receiver;
    Bpacket->sender   = sender;
    Bpacket->request  = request;
    Bpacket->code     = code;
    Bpacket->numBytes = numDataBytes;

    // Copy the data to the packet
    for (int i = 0; i < Bpacket->numBytes; i++) {
        Bpacket->bytes[i] = data[i + 7]; // The data starts on the 8th byte thus adding 7 (starting from 1)
    }

    return TRUE;
}

void bpacket_data_to_string(bpacket_t* Bpacket, bpacket_char_array_t* bpacketCharArray) {

    bpacketCharArray->numBytes = Bpacket->numBytes;

    if (Bpacket->numBytes == 0) {
        bpacketCharArray->string[0] = '\0';
        return;
    }

    // Copy the data to the packet.
    int i;
    for (i = 0; i < Bpacket->numBytes; i++) {
        bpacketCharArray->string[i] = Bpacket->bytes[i];
    }

    bpacketCharArray->string[i] = '\0';
}

void bpacket_get_error(uint8_t bpacketError, char* errorMsg) {

    switch (bpacketError) {
        case BPACKET_ERR_INVALID_RECEIVER:
            sprintf(errorMsg, "Bpacket err: Invalid receiver address\r\n");
            break;
        case BPACKET_ERR_INVALID_SENDER:
            sprintf(errorMsg, "Bpacket err: Invalid sender address\r\n");
            break;
        case BPACKET_ERR_INVALID_REQUEST:
            sprintf(errorMsg, "Bpacket err: Invalid request\r\n");
            break;
        case BPACKET_ERR_INVALID_CODE:
            sprintf(errorMsg, "Bpacket err: Invalid code\r\n");
            break;
        case BPACKET_ERR_INVALID_NUM_DATA_BYTES:
            sprintf(errorMsg, "Bpacket err: Invalid number of data bytes\r\n");
            break;
        case BPACKET_ERR_INVALID_START_BYTE:
            sprintf(errorMsg, "Bpacket err: Invalid start byte\r\n");
            break;
        default:
            sprintf(errorMsg, "Bpacket err: Unknown error code %i\r\n", bpacketError);
            break;
    }
}

void bpacket_get_info(bpacket_t* Bpacket, char* string) {
    sprintf(string, "Receiver: %i Sender: %i Request: %i Code: %i num bytes: %i\r\n", Bpacket->receiver,
            Bpacket->sender, Bpacket->request, Bpacket->code, Bpacket->numBytes);
}

uint8_t bpacket_send_data(void (*transmit_bpacket)(uint8_t* data, uint16_t bufferNumBytes), uint8_t receiver,
                          uint8_t sender, uint8_t request, uint8_t* data, uint32_t numBytesToSend) {

    // Create the Bpacket
    bpacket_t Bpacket;

    if (bpacket_create_p(&Bpacket, receiver, sender, request, BPACKET_CODE_IN_PROGRESS, 0, NULL) != TRUE) {
        return FALSE;
    }

    // Set the number of bytes to the maximum
    Bpacket.numBytes = BPACKET_MAX_NUM_DATA_BYTES;
    bpacket_buffer_t bpacketBuffer;

    uint32_t bytesSent = 0;
    uint32_t index     = 0;

    while (bytesSent < numBytesToSend) {

        if (index < BPACKET_MAX_NUM_DATA_BYTES) {
            Bpacket.bytes[index++] = data[bytesSent++];
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
        Bpacket.numBytes = index;
        Bpacket.code     = BPACKET_CODE_SUCCESS;

        // Send the Bpacket
        bpacket_to_buffer(&Bpacket, &bpacketBuffer);
        transmit_bpacket(bpacketBuffer.buffer, bpacketBuffer.numBytes);
    }

    return TRUE;
}

uint8_t bpacket_confirm_values(bpacket_t* Bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                               uint8_t numBytes, char* errMsg) {

    if (Bpacket->receiver != receiver) {
        sprintf(errMsg, "Invalid receiver. Expected %i but got %i\r\n", receiver, Bpacket->receiver);
        return FALSE;
    }

    if (Bpacket->sender != sender) {
        sprintf(errMsg, "Invalid Sender. Expected %i but got %i\r\n", sender, Bpacket->sender);
        return FALSE;
    }

    if (Bpacket->request != request) {
        sprintf(errMsg, "Invalid Request. Expected %i but got %i\r\n", request, Bpacket->request);
        return FALSE;
    }

    if (Bpacket->code != code) {
        sprintf(errMsg, "Invalid Code. Expected %i but got %i\r\n", code, Bpacket->code);
        return FALSE;
    }

    if (Bpacket->numBytes != numBytes) {
        sprintf(errMsg, "Invalid num bytes. Expected %i but got %i\r\n", numBytes, Bpacket->numBytes);
        return FALSE;
    }

    return TRUE;
}

void bp_convert_to_response(bpacket_t* Bpacket, bp_code_t Code, uint8_t numBytes, uint8_t* data) {

    // Swap the addresses
    uint8_t sender    = Bpacket->sender;
    Bpacket->sender   = Bpacket->receiver;
    Bpacket->receiver = sender;

    // Update the code
    Bpacket->code = Code.val;

    bp_erase_all_data(Bpacket);

    // Copy data into Bpacket
    if (data != NULL) {
        for (int i = 0; i < numBytes; i++) {
            Bpacket->bytes[i] = data[i];
        }
    }
}

void bp_erase_all_data(bpacket_t* Bpacket) {
    for (int i = 0; i < Bpacket->numBytes; i++) {
        Bpacket->bytes[i] = BP_DEFAULT_DATA_VALUE;
    }
}

uint8_t bp_convert_to_string_response(bpacket_t* Bpacket, bp_code_t Code, char* string) {

    // Swap the addresses
    uint8_t sender    = Bpacket->sender;
    Bpacket->sender   = Bpacket->receiver;
    Bpacket->receiver = sender;

    // Update the code
    Bpacket->code = Code.val;

    bp_erase_all_data(Bpacket);

    uint32_t numBytes = chars_get_num_bytes(string);

    if (numBytes > BPACKET_MAX_NUM_DATA_BYTES) {
        Bpacket->status = BP_STATUS_STRING_PACKET_RESPONSE_CREATION_FAILED.val;
        return FALSE;
    }

    Bpacket->numBytes = numBytes;

    // Copy data into Bpacket
    for (int i = 0; i < Bpacket->numBytes; i++) {
        Bpacket->bytes[i] = string[i];
    }

    return TRUE;
}