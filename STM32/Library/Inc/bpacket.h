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

#define START                     2 // Start at 2 so no code is the same as TRUE/FALSE
#define BPACKET_R_FAILED          (START + 0)
#define BPACKET_R_SUCCESS         (START + 1)
#define BPACKET_R_UNKNOWN         (START + 2)
#define BPACKET_R_IN_PROGRESS     (START + 3)
#define BPACKET_GEN_R_HELP        (START + 4)
#define BPACKET_GEN_R_PING        (START + 5)
#define BPACKET_GET_R_STATUS      (START + 6)
#define BPACKET_SPECIFIC_R_OFFSET (START + 7) // This is the offset applied to specific projects

// #define BPACKET_R_LIST_DIR      (4 + START)
// #define BPACKET_R_COPY_FILE     (5 + START)
// #define BPACKET_R_TAKE_PHOTO    (6 + START)
// #define BPACKET_R_WRITE_TO_FILE (7 + START)
// #define BPACKET_R_RECORD_DATA   (8 + START)
// #define BPACKET_R_PING          (9 + START)
// #define BPACKET_R_LED_RED_ON    (10 + START)
// #define BPACKET_R_LED_RED_OFF   (11 + START)
// #define BPACKET_R_ACKNOWLEDGE   (12 + START)

#define BPACKET_REQUEST_SIZE_BYTES   1
#define BPACKET_MAX_NUM_DATA_BYTES   31
#define BPACKET_CIRCULAR_BUFFER_SIZE 10

#define BPACKET_BUFFER_NUM_START_STOP_BYTES 2 // One start and one stop byte
#define BPACKET_NUM_INFO_BYTES              1 // This is the byte that represents the length of the data in bytes
#define BPACKET_BUFFER_NUM_NON_DATA_BYTES   BPACKET_BUFFER_NUM_START_STOP_BYTES + BPACKET_NUM_INFO_BYTES
#define BPACKET_BUFFER_LENGTH_BYTES \
    (BPACKET_MAX_NUM_DATA_BYTES + BPACKET_REQUEST_SIZE_BYTES + BPACKET_BUFFER_NUM_NON_DATA_BYTES)

typedef struct bpacket_t {
    uint8_t numBytes;
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

void bpacket_create_p(bpacket_t* bpacket, uint8_t request, uint8_t numDataBytes, uint8_t* data);

void bpacket_create_sp(bpacket_t* bpacket, uint8_t request, char* string);

void bpacket_to_buffer(bpacket_t* bpacket, bpacket_buffer_t* packetBuffer);

void bpacket_data_to_string(bpacket_t* bpacket, bpacket_char_array_t* bpacketCharArray);

#endif // BPACKET_H