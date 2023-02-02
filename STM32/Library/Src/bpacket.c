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

uint8_t bpacket_create_p(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                         uint8_t numDataBytes, uint8_t* data) {

    BPACKET_ASSERT_VALID_ADDRESS(sender);
    BPACKET_ASSERT_VALID_ADDRESS(receiver);
    BPACKET_ASSERT_VALID_REQUEST(request);
    BPACKET_ASSERT_VALID_CODE(code);
    BPACKET_ASSERT_VALID_NUM_BYTES(numDataBytes);

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

    BPACKET_ASSERT_VALID_ADDRESS(sender);
    BPACKET_ASSERT_VALID_ADDRESS(receiver);
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

    packetBuffer->buffer[0] = BPACKET_START_BYTE;
    packetBuffer->buffer[1] = BPACKET_SENDER_RECEIVER_TO_BYTE(bpacket->sender, bpacket->receiver);
    packetBuffer->buffer[2] = BPACKET_REQUEST_CODE_TO_BYTE(bpacket->request, bpacket->code);
    packetBuffer->buffer[3] = bpacket->numBytes;

    // Copy data into buffer
    int i;
    for (i = 0; i < bpacket->numBytes; i++) {
        packetBuffer->buffer[i + 4] = bpacket->bytes[i];
    }

    packetBuffer->buffer[i + 4] = BPACKET_STOP_BYTE;

    packetBuffer->numBytes = bpacket->numBytes + BPACKET_NUM_NON_DATA_BYTES;
}

uint8_t bpacket_buffer_decode(bpacket_t* bpacket, uint8_t data[BPACKET_BUFFER_LENGTH_BYTES]) {

    uint8_t receiver     = BPACKET_BYTE_TO_RECEIVER(data[1]);
    uint8_t sender       = BPACKET_BYTE_TO_SENDER(data[1]);
    uint8_t request      = BPACKET_BYTE_TO_REQUEST(data[2]);
    uint8_t code         = BPACKET_BYTE_TO_CODE(data[2]);
    uint8_t numDataBytes = data[3];

    BPACKET_ASSERT_VALID_START_BYTE(data[0]);
    BPACKET_ASSERT_VALID_ADDRESS(sender);
    BPACKET_ASSERT_VALID_ADDRESS(receiver);
    BPACKET_ASSERT_VALID_REQUEST(request);
    BPACKET_ASSERT_VALID_CODE(code);
    BPACKET_ASSERT_VALID_NUM_BYTES(numDataBytes);

    bpacket->receiver = receiver;
    bpacket->sender   = sender;
    bpacket->request  = request;
    bpacket->code     = code;
    bpacket->numBytes = numDataBytes;

    // Copy the data to the packet
    for (int i = 0; i < bpacket->numBytes; i++) {
        bpacket->bytes[i] = data[i + 4]; // The data starts on the 5th byte thus adding 4 (starting from 1)
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
            sprintf(errorMsg, "Invalid receiver address\r\n");
            break;
        case BPACKET_ERR_INVALID_SENDER:
            sprintf(errorMsg, "Invalid sender address\r\n");
            break;
        case BPACKET_ERR_INVALID_REQUEST:
            sprintf(errorMsg, "Invalid request\r\n");
            break;
        case BPACKET_ERR_INVALID_CODE:
            sprintf(errorMsg, "Invalid code\r\n");
            break;
        case BPACKET_ERR_INVALID_NUM_DATA_BYTES:
            sprintf(errorMsg, "Invalid number of data bytes\r\n");
            break;
        case BPACKET_ERR_INVALID_START_BYTE:
            sprintf(errorMsg, "Invalid start byte\r\n");
            break;
        default:
            sprintf(errorMsg, "Unknown error code %i\r\n", bpacketError);
            break;
    }
}