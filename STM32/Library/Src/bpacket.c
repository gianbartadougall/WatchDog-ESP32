/**
 * @file bpacket.c
 * @author Gian Barta-Dougall
 * @brief
 *
 * The structure of a bpacket is as follows
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

void bpacket_create_circular_buffer(bpacket_circular_buffer_t* bufferStruct, uint8_t* writeIndex, uint8_t* readIndex,
                                    bpacket_t* circularBuffer) {
    bufferStruct->writeIndex = writeIndex;
    bufferStruct->readIndex  = readIndex;

    for (int i = 0; i < BPACKET_CIRCULAR_BUFFER_SIZE; i++) {
        bufferStruct->circularBuffer[i] = (circularBuffer + i);
    }
}

void bpacket_increment_circular_buffer_index(uint8_t* writeIndex) {
    (*writeIndex)++;

    if (*writeIndex >= BPACKET_CIRCULAR_BUFFER_SIZE) {
        *writeIndex = 0;
    }
}

void bpacket_increment_circ_buff_index(uint32_t* cbIndex, uint32_t bufferMaxIndex) {
    (*cbIndex)++;

    if (*cbIndex > bufferMaxIndex) {
        (*cbIndex) = 0;
    }
}

uint8_t bpacket_create_p(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                         uint8_t numDataBytes, uint8_t* data) {

    BPACKET_ASSERT_VALID_RECEIVER(receiver);
    BPACKET_ASSERT_VALID_SENDER(sender);
    BPACKET_ASSERT_VALID_REQUEST(request);
    BPACKET_ASSERT_VALID_CODE(code);

    bpacket->receiver = receiver;
    bpacket->sender   = sender;
    bpacket->request  = request;
    bpacket->code     = code;
    bpacket->numBytes = numDataBytes;

    // Copy data into bpacket
    if (data != NULL) {
        for (int i = 0; i < numDataBytes; i++) {
            bpacket->bytes[i] = data[i];
        }
    }

    return TRUE;
}

uint8_t bpacket_create_sp(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                          char* string) {

    BPACKET_ASSERT_VALID_RECEIVER(receiver);
    BPACKET_ASSERT_VALID_SENDER(sender);
    BPACKET_ASSERT_VALID_REQUEST(request);
    BPACKET_ASSERT_VALID_CODE(code);
    BPACKET_ASSERT_VALID_NUM_BYTES(chars_get_num_bytes(string));

    bpacket->receiver = receiver;
    bpacket->sender   = sender;
    bpacket->code     = code;
    bpacket->request  = request;
    bpacket->numBytes = chars_get_num_bytes(string);

    // Copy data into bpacket
    for (int i = 0; i < bpacket->numBytes; i++) {
        bpacket->bytes[i] = string[i];
    }

    return TRUE;
}

void bpacket_to_buffer(bpacket_t* bpacket, bpacket_buffer_t* packetBuffer) {

    // Set the first two bytes to start bytes
    packetBuffer->buffer[0] = BPACKET_START_BYTE_UPPER;
    packetBuffer->buffer[1] = BPACKET_START_BYTE_LOWER;

    // Set the sender and receiver bytes
    packetBuffer->buffer[2] = bpacket->receiver;
    packetBuffer->buffer[3] = bpacket->sender;

    // Set the request and code
    packetBuffer->buffer[4] = bpacket->request;
    packetBuffer->buffer[5] = bpacket->code;

    // Set the length
    packetBuffer->buffer[6] = bpacket->numBytes;

    // Copy data into buffer
    int i;
    for (i = 0; i < bpacket->numBytes; i++) {
        packetBuffer->buffer[i + 7] = bpacket->bytes[i];
    }

    // Set the stop bytes at the end
    packetBuffer->buffer[i + 7] = BPACKET_STOP_BYTE_UPPER;
    packetBuffer->buffer[i + 8] = BPACKET_STOP_BYTE_LOWER;

    packetBuffer->numBytes = bpacket->numBytes + BPACKET_NUM_NON_DATA_BYTES;
}

uint8_t bpacket_buffer_decode(bpacket_t* bpacket, uint8_t data[BPACKET_BUFFER_LENGTH_BYTES]) {

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

    bpacket->receiver = receiver;
    bpacket->sender   = sender;
    bpacket->request  = request;
    bpacket->code     = code;
    bpacket->numBytes = numDataBytes;

    // Copy the data to the packet
    for (int i = 0; i < bpacket->numBytes; i++) {
        bpacket->bytes[i] = data[i + 7]; // The data starts on the 8th byte thus adding 7 (starting from 1)
    }

    return TRUE;
}

void bpacket_data_to_string(bpacket_t* bpacket, bpacket_char_array_t* bpacketCharArray) {

    bpacketCharArray->numBytes = bpacket->numBytes;

    if (bpacket->numBytes == 0) {
        bpacketCharArray->string[0] = '\0';
        return;
    }

    // Copy the data to the packet.
    int i;
    for (i = 0; i < bpacket->numBytes; i++) {
        bpacketCharArray->string[i] = bpacket->bytes[i];
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