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

void bpacket_create_p(bpacket_t* bpacket, uint8_t request, uint8_t numDataBytes,
                      uint8_t data[BPACKET_MAX_NUM_DATA_BYTES]) {

    bpacket->request  = request;
    bpacket->numBytes = numDataBytes;

    // Copy data into bpacket
    if (data != NULL) {
        for (int i = 0; i < numDataBytes; i++) {
            bpacket->bytes[i] = data[i];
        }
    }
}

void bpacket_create_sp(bpacket_t* bpacket, uint8_t request, char string[BPACKET_MAX_NUM_DATA_BYTES + 1]) {

    bpacket->request  = request;
    bpacket->numBytes = chars_get_num_bytes(string);

    // Copy data into bpacket
    for (int i = 0; i < bpacket->numBytes; i++) {
        bpacket->bytes[i] = string[i];
    }
}

void bpacket_to_buffer(bpacket_t* bpacket, bpacket_buffer_t* packetBuffer) {

    packetBuffer->buffer[0] = BPACKET_START_BYTE;
    packetBuffer->buffer[1] = bpacket->numBytes + BPACKET_REQUEST_SIZE_BYTES;
    packetBuffer->buffer[2] = bpacket->request;

    // Copy data into buffer
    int i;
    for (i = 0; i < bpacket->numBytes; i++) {
        packetBuffer->buffer[i + 3] = bpacket->bytes[i];
    }

    packetBuffer->buffer[i + 3] = BPACKET_STOP_BYTE;

    packetBuffer->numBytes = bpacket->numBytes + BPACKET_REQUEST_SIZE_BYTES + BPACKET_BUFFER_NUM_NON_DATA_BYTES;
}

void bpacket_decode(bpacket_t* bpacket, uint8_t data[BPACKET_BUFFER_LENGTH_BYTES]) {

    // Confirm first 3 bytes are not null
    if (data[0] == '\0' || data[1] == '\0' || data[2] == '\0') {
        bpacket->request  = BPACKET_R_NULL;
        bpacket->numBytes = 0;
        return;
    }

    // Extract the number of data bytes and the command from the data
    bpacket->numBytes = data[1] - BPACKET_REQUEST_SIZE_BYTES;
    bpacket->request  = data[2];

    // Copy the data to the packet.
    for (int i = 0; i < bpacket->numBytes; i++) {
        bpacket->bytes[i] = data[i + 3]; // The data starts on the 4th byte thus adding 3 (starting from 1)
    }
}