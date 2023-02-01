/**
 * @file bpacket.h
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-18
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef BPACKET_H
#define BPACKET_H

/* C Library Includes */
#include "stdint.h"

#define BPACKET_START_BYTE 'A'
#define BPACKET_STOP_BYTE  'B'

#define BPACKET_MIN_REQUEST_INDEX 2 // Start at 2 so no code is the same as TRUE/FALSE
#define BPACKET_R_FAILED          (BPACKET_MIN_REQUEST_INDEX + 0)
#define BPACKET_R_SUCCESS         (BPACKET_MIN_REQUEST_INDEX + 1)
#define BPACKET_R_UNKNOWN         (BPACKET_MIN_REQUEST_INDEX + 2)
#define BPACKET_R_IN_PROGRESS     (BPACKET_MIN_REQUEST_INDEX + 3)
#define BPACKET_GEN_R_HELP        (BPACKET_MIN_REQUEST_INDEX + 4)
#define BPACKET_GEN_R_PING        (BPACKET_MIN_REQUEST_INDEX + 5)
#define BPACKET_GET_R_STATUS      (BPACKET_MIN_REQUEST_INDEX + 6)
#define BPACKET_SPECIFIC_R_OFFSET (BPACKET_MIN_REQUEST_INDEX + 7) // This is the offset applied to specific projects

/**
 * @brief The address byte in the bpacket is one byte.
 * The byte is split up into the receiver (first four
 * bits) and the sender address (last four bits). Thus
 * the maxiumum address a sender or receiver can have
 * is 15 because 2^4 - 1 = 15
 */
#define BPACKET_ADDRESS_0  0x00
#define BPACKET_ADDRESS_1  0x01
#define BPACKET_ADDRESS_2  0x02
#define BPACKET_ADDRESS_3  0x03
#define BPACKET_ADDRESS_4  0x04
#define BPACKET_ADDRESS_5  0x05
#define BPACKET_ADDRESS_6  0x06
#define BPACKET_ADDRESS_7  0x07
#define BPACKET_ADDRESS_8  0x08
#define BPACKET_ADDRESS_9  0x09
#define BPACKET_ADDRESS_10 0x0A
#define BPACKET_ADDRESS_11 0x0B
#define BPACKET_ADDRESS_12 0x0C
#define BPACKET_ADDRESS_13 0x0D
#define BPACKET_ADDRESS_14 0x0E
#define BPACKET_ADDRESS_15 0x0F

#define BPACKET_MIN_ADDRESS BPACKET_ADDRESS_0
#define BPACKET_MAX_ADDRESS BPACKET_ADDRESS_15

#define BPACKET_BYTE_TO_RECEIVER(byte)                    (byte & 0x0F)
#define BPACKET_BYTE_TO_SENDER(byte)                      (byte >> 4)
#define BPACKET_SENDER_RECEIVER_TO_BYTE(sender, receiver) ((sender << 4) | receiver)

// These represent the addresses of each system in the project. Sending a bpacket
// requires you to supply an address so the reciever knows whether the bpacket
// is for them or if they are supposed to pass the bpacket on
#define BPACKET_ADDRESS_ESP32 BPACKET_ADDRESS_1
#define BPACKET_ADDRESS_STM32 BPACKET_ADDRESS_2
#define BPACKET_ADDRESS_MAPLE BPACKET_ADDRESS_3

#define BPACKET_MAX_NUM_DATA_BYTES  255 // Chosen to be 255 as the max number that can fit into one byte is 255
#define BPACKET_NUM_START_BYTES     1
#define BPACKET_NUM_ADDRESS_BYTES   1 // The address bytes contain the sender and reciever information
#define BPACKET_NUM_REQUEST_BYTES   1
#define BPACKET_NUM_INFO_BYTES      1 // Indicateds number of data bytes (0 - max number of data bytes allowed)
#define BPACKET_NUM_STOP_BYTES      1
#define BPACKET_NUM_NON_DATA_BYTES  5
#define BPACKET_BUFFER_LENGTH_BYTES (BPACKET_MAX_NUM_DATA_BYTES + BPACKET_NUM_NON_DATA_BYTES)

#define BPACKET_CIRCULAR_BUFFER_SIZE 10

typedef struct bpacket_t {
    uint8_t receiver;
    uint8_t sender;
    uint8_t numBytes; // The number of bytes in the bytes array
    uint8_t request;
    uint8_t bytes[BPACKET_MAX_NUM_DATA_BYTES]; // Minus 1 because this includes request
} bpacket_t;

typedef struct bpacket_buffer_t {
    uint8_t numBytes;
    uint8_t buffer[BPACKET_BUFFER_LENGTH_BYTES];
} bpacket_buffer_t;

typedef struct bpacket_char_array_t {
    uint8_t numBytes;
    char string[BPACKET_MAX_NUM_DATA_BYTES + 1]; // One extra for null character
} bpacket_char_array_t;

typedef struct bpacket_circular_buffer_t {
    uint8_t* readIndex;  // The index that the writing end of the buffer would use to know where to put the bpacket
    uint8_t* writeIndex; // The index that the reading end of the buffer would use to see if they are up to date with
                         // the reading
    bpacket_t* circularBuffer[BPACKET_CIRCULAR_BUFFER_SIZE];
} bpacket_circular_buffer_t;

void bpacket_increment_circular_buffer_index(uint8_t* writeIndex);

void bpacket_create_circular_buffer(bpacket_circular_buffer_t* bufferStruct, uint8_t* writeIndex, uint8_t* readIndex,
                                    bpacket_t* circularBuffer);

void bpacket_buffer_decode(bpacket_t* bpacket, uint8_t data[BPACKET_BUFFER_LENGTH_BYTES]);

void bpacket_create_p(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t numDataBytes,
                      uint8_t* data);

void bpacket_create_sp(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, char* string);

void bpacket_to_buffer(bpacket_t* bpacket, bpacket_buffer_t* packetBuffer);

void bpacket_data_to_string(bpacket_t* bpacket, bpacket_char_array_t* bpacketCharArray);

void bpacket_print_bytes(bpacket_t* bpacket);

#endif // BPACKET_H