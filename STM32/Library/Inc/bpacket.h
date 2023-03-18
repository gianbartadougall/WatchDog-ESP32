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

// 16 bit start and end byte means when checking
// there is a 1 in 2^32 chance of getting a start
// and stop byte together randomly. The start and
// stop bytes were chosen so they wouldn't occur
// very frequently together
#define BPACKET_START_BYTE_UPPER 'B'
#define BPACKET_START_BYTE_LOWER 'z'

#define BPACKET_STOP_BYTE_UPPER 'j'
#define BPACKET_STOP_BYTE_LOWER 'Y'

// #define BPACKET_START_BYTE 'A'
// #define BPACKET_STOP_BYTE  'B'

/**
 * @brief BPacket codes. These gives context to the
 * request in a bpacket.
 *
 * E.g if you receive a bpacket with the code
 * BPACKET_CODE_ERROR then this states the sender
 * of the bpacket failed to execute the request.
 *
 * If you receive a bpacket with the code
 * BPACKET_CODE_IN_PROGRESS then the sender of the
 * bpacket is stil processing the request
 */
#define BPACKET_CODE_ERROR       0
#define BPACKET_CODE_SUCCESS     1
#define BPACKET_CODE_IN_PROGRESS 2
#define BPACKET_CODE_UNKNOWN     3
#define BPACKET_CODE_EXECUTE     4
#define BPACKET_CODE_TODO        5
#define BPACKET_CODE_DEBUG       6
#define BPACKET_CODE_EMPTY_3     7 // Free spot for code to be added in future if needed
#define BPACKET_MAX_CODE_VALUE   7

#define BPACKET_MAX_REQUEST_VALUE 63 // Allow maximum of 63 different request values

#define BPACKET_MIN_REQUEST_INDEX 2 // Start at 2 so no code is the same as TRUE/FALSE
#define BPACKET_GEN_R_HELP        (BPACKET_MIN_REQUEST_INDEX + 1)
#define BPACKET_GEN_R_PING        (BPACKET_MIN_REQUEST_INDEX + 2)
#define BPACKET_GET_R_STATUS      (BPACKET_MIN_REQUEST_INDEX + 3)
#define BPACKET_GEN_R_MESSAGE     (BPACKET_MIN_REQUEST_INDEX + 4) // Used for debugging purposes and general messages
#define BPACKET_SPECIFIC_R_OFFSET (BPACKET_MIN_REQUEST_INDEX + 5) // This is the offset applied to specific projects

#define BPACKET_CODE_IS_INVALID(code)         ((code > BPACKET_CODE_EXECUTE) == TRUE)
#define BPACKET_SENDER_IS_INVALID(sender)     ((sender > BPACKET_ADDRESS_15) == TRUE)
#define BPACKET_RECEIVER_IS_INVALID(receiver) ((receiver > BPACKET_ADDRESS_15) == TRUE)
#define BPACKET_REQUEST_IS_INVALID(request)   ((request > 31) == TRUE) // Max value for request is 31

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

#define BPACKET_MAX_NUM_DATA_BYTES  255 // Chosen to be 65535 as the max number that can fit into one byte is 255
#define BPACKET_NUM_START_BYTES     2
#define BPACKET_NUM_ADDRESS_BYTES   2 // The address bytes contains one byte for receiver and one for sender
#define BPACKET_NUM_REQUEST_BYTES   2
#define BPACKET_NUM_INFO_BYTES      1 // Indicateds number of data bytes (0 - max number of data bytes allowed)
#define BPACKET_NUM_STOP_BYTES      2
#define BPACKET_NUM_NON_DATA_BYTES  9
#define BPACKET_BUFFER_LENGTH_BYTES (BPACKET_MAX_NUM_DATA_BYTES + BPACKET_NUM_NON_DATA_BYTES)

#define BPACKET_CIRCULAR_BUFFER_SIZE 10

// Bpacket data type ids
#define BPACKET_START_BYTE_UPPER_ID 0
#define BPACKET_START_BYTE_LOWER_ID 1
#define BPACKET_STOP_BYTE_UPPER_ID  2
#define BPACKET_STOP_BYTE_LOWER_ID  3
#define BPACKET_DATA_BYTE_ID        4
#define BPACKET_NUM_BYTES_BYTE_ID   5
#define BPACKET_CODE_BYTE_ID        6
#define BPACKET_REQUEST_BYTE_ID     7
#define BPACKET_SENDER_BYTE_ID      8
#define BPACKET_RECEIVER_BYTE_ID    9

// Bpacket Errors
#define BPACKET_ERR_OFFSET                 2 // Offset so no error code = TRUE/FALSE
#define BPACKET_ERR_INVALID_SENDER         (BPACKET_ERR_OFFSET + 0)
#define BPACKET_ERR_INVALID_RECEIVER       (BPACKET_ERR_OFFSET + 1)
#define BPACKET_ERR_INVALID_REQUEST        (BPACKET_ERR_OFFSET + 2)
#define BPACKET_ERR_INVALID_CODE           (BPACKET_ERR_OFFSET + 3)
#define BPACKET_ERR_INVALID_NUM_DATA_BYTES (BPACKET_ERR_OFFSET + 4)
#define BPACKET_ERR_INVALID_START_BYTE     (BPACKET_ERR_OFFSET + 5)

#define BPACKET_START_BYTE(byteUpper, byteLower) \
    ((byteUpper == BPACKET_START_BYTE_UPPER) && (byteLower == BPACKET_START_BYTE_LOWER))

#define BPACKET_STOP_BYTE(byteUpper, byteLower) \
    ((byteUpper == BPACKET_STOP_BYTE_UPPER) && (byteLower == BPACKET_STOP_BYTE_LOWER))

#define BPACKET_ASSERT_VALID_SENDER(sender)    \
    do {                                       \
        if (sender > BPACKET_MAX_ADDRESS) {    \
            return BPACKET_ERR_INVALID_SENDER; \
        }                                      \
    } while (0)

#define BPACKET_ASSERT_VALID_RECEIVER(receiver)  \
    do {                                         \
        if (receiver > BPACKET_MAX_ADDRESS) {    \
            return BPACKET_ERR_INVALID_RECEIVER; \
        }                                        \
    } while (0)

#define BPACKET_ASSERT_VALID_REQUEST(request)      \
    do {                                           \
        if (request > BPACKET_MAX_REQUEST_VALUE) { \
            return BPACKET_ERR_INVALID_REQUEST;    \
        }                                          \
    } while (0)

#define BPACKET_ASSERT_VALID_CODE(code)      \
    do {                                     \
        if (code > BPACKET_MAX_CODE_VALUE) { \
            return BPACKET_ERR_INVALID_CODE; \
        }                                    \
    } while (0)

#define BPACKET_ASSERT_VALID_NUM_BYTES(numBytes)       \
    do {                                               \
        if (numBytes > BPACKET_MAX_NUM_DATA_BYTES) {   \
            return BPACKET_ERR_INVALID_NUM_DATA_BYTES; \
        }                                              \
    } while (0)

#define BPACKET_ASSERT_VALID_START_BYTE(startByteUpper, startByteLower)                                     \
    do {                                                                                                    \
        if ((startByteUpper != BPACKET_START_BYTE_UPPER) || (BPACKET_START_BYTE_LOWER != startByteLower)) { \
            return BPACKET_ERR_INVALID_START_BYTE;                                                          \
        }                                                                                                   \
    } while (0)

typedef struct bpacket_t {
    uint8_t receiver;
    uint8_t sender;
    uint8_t numBytes; // The number of bytes in the bytes array
    uint8_t request;
    /**
     * @brief The code gives context to the request. If you receive a request
     * and the code is BPACKET_CODE_EXECUTE then the bpacket is asking for
     * the receiver to execute the request. If the code is BPACKET_CODE_ERROR
     * then the bpacket is a response stating the request it tried to exute
     * failed
     */
    uint8_t code;
    uint8_t bytes[BPACKET_MAX_NUM_DATA_BYTES];
    uint8_t status; // Code from 0 - 255 which holds the current status of the bpacket
} bpacket_t;

typedef struct bpacket_buffer_t {
    uint16_t numBytes; // Bpacket size needs to be a uint16_t because bpacket buffer > 255 bytes when put into a buffer
    uint8_t buffer[BPACKET_BUFFER_LENGTH_BYTES];
} bpacket_buffer_t;

typedef struct bpacket_char_array_t {
    uint16_t numBytes;
    char string[BPACKET_MAX_NUM_DATA_BYTES + 1]; // One extra for null character
} bpacket_char_array_t;

typedef struct bpacket_circular_buffer_t {
    uint8_t* rIndex; // The index that the writing end of the buffer would use to know where to put the bpacket
    uint8_t* wIndex; // The index that the reading end of the buffer would use to see if they are up to date with
                     // the reading
    bpacket_t* buffer[BPACKET_CIRCULAR_BUFFER_SIZE];
} bpacket_circular_buffer_t;

typedef struct bp_send_address_t {
    const uint8_t val;
} bp_send_address_t;

typedef struct bp_receive_address_t {
    const uint8_t val;
} bp_receive_address_t;

typedef struct bp_code_t {
    const uint8_t val;
} bp_code_t;

typedef struct bp_request_t {
    const uint8_t val;
} bp_request_t;

static const bp_send_address_t BP_ADDRESS_S_STM32 = {.val = BPACKET_ADDRESS_STM32};
static const bp_send_address_t BP_ADDRESS_S_ESP32 = {.val = BPACKET_ADDRESS_ESP32};
static const bp_send_address_t BP_ADDRESS_S_MAPLE = {.val = BPACKET_ADDRESS_MAPLE};

static const bp_receive_address_t BP_ADDRESS_R_STM32 = {.val = BPACKET_ADDRESS_STM32};
static const bp_receive_address_t BP_ADDRESS_R_ESP32 = {.val = BPACKET_ADDRESS_ESP32};
static const bp_receive_address_t BP_ADDRESS_R_MAPLE = {.val = BPACKET_ADDRESS_MAPLE};

static const bp_code_t BP_CODE_ERROR       = {.val = BPACKET_CODE_ERROR};
static const bp_code_t BP_CODE_SUCCESS     = {.val = BPACKET_CODE_SUCCESS};
static const bp_code_t BP_CODE_IN_PROGRESS = {.val = BPACKET_CODE_IN_PROGRESS};
static const bp_code_t BP_CODE_UNKNOWN     = {.val = BPACKET_CODE_UNKNOWN};
static const bp_code_t BP_CODE_EXECUTE     = {.val = BPACKET_CODE_EXECUTE};
static const bp_code_t BP_CODE_TODO        = {.val = BPACKET_CODE_TODO};
static const bp_code_t BP_CODE_DEBUG       = {.val = BPACKET_CODE_DEBUG};
static const bp_code_t BP_CODE_EMPTY_3     = {.val = BPACKET_CODE_EMPTY_3};

static const bp_request_t BP_GEN_R_HELP    = {.val = BPACKET_GEN_R_HELP};
static const bp_request_t BP_GEN_R_PING    = {.val = BPACKET_GEN_R_PING};
static const bp_request_t BP_GEN_R_STATUS  = {.val = BPACKET_GET_R_STATUS};
static const bp_request_t BP_GEN_R_MESSAGE = {.val = BPACKET_GEN_R_MESSAGE};

void bpacket_increment_circular_buffer_index(uint8_t* writeIndex);

void bpacket_create_circular_buffer(bpacket_circular_buffer_t* bufferStruct, uint8_t* writeIndex, uint8_t* readIndex,
                                    bpacket_t* buffer);

uint8_t bpacket_buffer_decode(bpacket_t* bpacket, uint8_t data[BPACKET_BUFFER_LENGTH_BYTES]);

uint8_t bpacket_create_p(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                         uint8_t numDataBytes, uint8_t* data);

uint8_t bp_create_packet(bpacket_t* bpacket, const bp_receive_address_t RAddress, const bp_send_address_t SAddress,
                         const bp_request_t Request, const bp_code_t Code, uint8_t numDataBytes, uint8_t* data);

uint8_t bpacket_create_sp(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                          char* string);

void bpacket_to_buffer(bpacket_t* bpacket, bpacket_buffer_t* packetBuffer);

void bpacket_data_to_string(bpacket_t* bpacket, bpacket_char_array_t* bpacketCharArray);

void bpacket_print_bytes(bpacket_t* bpacket);

void bpacket_get_info(bpacket_t* bpacket, char* string);

void bpacket_increment_circ_buff_index(uint32_t* cbIndex, uint32_t bufferMaxIndex);

void bpacket_get_error(uint8_t bpacketError, char* errorMsg);

/* Bpacket helper functions */
void bpacket_bytes_is_start_byte(void);

uint8_t bpacket_send_data(void (*transmit_bpacket)(uint8_t* data, uint16_t bufferNumBytes), uint8_t receiver,
                          uint8_t sender, uint8_t request, uint8_t* data, uint32_t numBytesToSend);

uint8_t bpacket_confirm_values(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                               uint8_t numDataBytes, char* errMsg);

#endif // BPACKET_H