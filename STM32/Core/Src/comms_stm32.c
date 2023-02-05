/**
 * @file comms_stm32.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-01-29
 *
 * @copyright Copyright (c) 2023
 *
 */

/* Personal Includes */
#include "comms_stm32.h"
#include "utilities.h"
#include "log.h"
#include "watchdog_defines.h"
#include "chars.h"

#define PACKET_BUFFER_SIZE 1

#define RX_BUFFER_SIZE (BPACKET_BUFFER_LENGTH_BYTES * PACKET_BUFFER_SIZE)

/* Private Macros */

/* Private Variables */
USART_TypeDef* uarts[NUM_BUFFERS] = {
    BUFFER_1,
    BUFFER_2,
};

// Create a list for every single UART line
uint8_t rxBuffers[NUM_BUFFERS][RX_BUFFER_SIZE] = {{0}, {0}};
uint32_t rxBufIndexes[NUM_BUFFERS];
uint32_t rxBufProcessedIndexes[NUM_BUFFERS];

#define BUFFER(id) (rxBuffers[id][rxBufIndexes[id]])

// New Bpacket system variables
uint8_t expectedByteId[NUM_BUFFERS];
uint8_t numDataBytesExpected[NUM_BUFFERS];
uint8_t numDataBytesReceived[NUM_BUFFERS];
uint8_t divertedBytesBufferId[NUM_BUFFERS];
uint8_t bpacketByteIndex[NUM_BUFFERS];
uint8_t divertBytes[NUM_BUFFERS];
uint8_t lastByte = 0;

// For some reason haven't worked it out, if you set this to false at the start of the rxProcessBuffer
// it will stuff up the diversion of bytes. Need to keep it global
// uint8_t divertBytes           = FALSE;
// uint8_t divertedBytesBufferId = BUFFER_1_ID;

/* Function Prototyes */
void comms_send_byte(uint8_t bufferId, uint8_t byte);

void comms_stm32_init(void) {

    for (int i = 0; i < NUM_BUFFERS; i++) {
        rxBufIndexes[i]          = 0;
        rxBufProcessedIndexes[i] = 0;

        expectedByteId[i]   = BPACKET_START_BYTE_UPPER_ID;
        bpacketByteIndex[i] = 0;
        divertBytes[i]      = FALSE;
    }
}

void commms_stm32_increment_circ_buff_index(uint32_t* index, uint32_t bufferLength) {

    if (*index == (bufferLength - 1)) {
        *index = 0;
        return;
    }

    *index += 1;
}

void comms_add_to_buffer(uint8_t bufferId, uint8_t byte) {
    BUFFER(bufferId) = byte;
    commms_stm32_increment_circ_buff_index(&rxBufIndexes[bufferId], RX_BUFFER_SIZE);
}

void update_past_bytes(uint8_t byte) {
    lastByte = byte;
    // pastByte3             = pastByte2;
    // pastByte2             = pastByte1;
    // pastByte1             = byte;
}

uint8_t comms_process_rxbuffer(uint8_t bufferId, bpacket_t* bpacket) {

    while (rxBufProcessedIndexes[bufferId] != rxBufIndexes[bufferId]) {

        uint8_t byte = rxBuffers[bufferId][rxBufProcessedIndexes[bufferId]];
        commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);

        if (expectedByteId[bufferId] == BPACKET_DATA_BYTE_ID) {
            update_past_bytes(byte);
            // Increment the number of data bytes received
            numDataBytesReceived[bufferId]++;

            // Set the expected byte to stop byte if this is the last data byte expected
            if (numDataBytesReceived[bufferId] == numDataBytesExpected[bufferId]) {
                expectedByteId[bufferId] = BPACKET_STOP_BYTE_UPPER_ID;
            }

            // char msg[40];
            // sprintf(msg, "DATA BYTE[%i]: [%i] [%i]", bufferId, numDataBytesReceived[bufferId],
            //         numDataBytesExpected[bufferId]);
            // log_send_data(msg, sizeof(msg));

            if (divertBytes[bufferId] == TRUE) {

                // Divert byte to intended receiver
                comms_send_byte(divertedBytesBufferId[bufferId], byte);
                continue;
            }

            // Add byte to bpacket
            bpacket->bytes[bpacketByteIndex[bufferId]++] = byte;
            continue;
        }

        if ((expectedByteId[bufferId] == BPACKET_STOP_BYTE_LOWER_ID) && (byte == BPACKET_STOP_BYTE_LOWER)) {
            update_past_bytes(byte);
            // if (bufferId == BUFFER_1_ID) {
            //     log_send_data(" STP BYTE LW ", 13);
            // }
            // End of packet reached. Reset the system. It is important that this is done
            // regardless of whether the bytes were diverted or not. This ensures the next
            // packet can be read
            expectedByteId[bufferId] = BPACKET_START_BYTE_UPPER_ID;

            if (divertBytes[bufferId] == TRUE) {
                comms_send_byte(divertedBytesBufferId[bufferId], byte);

                continue;
            }

            // Reset the divertBytes flag to FALSE
            divertBytes[bufferId] = FALSE;

            return TRUE;
        }

        if ((expectedByteId[bufferId] == BPACKET_STOP_BYTE_UPPER_ID) && (byte == BPACKET_STOP_BYTE_UPPER)) {
            update_past_bytes(byte);
            // if (bufferId == BUFFER_1_ID) {
            //     log_send_data(" STP BYTE UP ", 13);
            // }
            expectedByteId[bufferId] = BPACKET_STOP_BYTE_LOWER_ID;

            if (divertBytes[bufferId] == TRUE) {
                comms_send_byte(divertedBytesBufferId[bufferId], byte);
            }

            continue;
        }

        if (expectedByteId[bufferId] == BPACKET_NUM_BYTES_BYTE_ID) {
            // if (bufferId == BUFFER_1_ID) {
            //     char y[20];
            //     sprintf(y, " LEN BYTE[%i] ", byte);
            //     log_send_data(y, chars_get_num_bytes(y));
            // }
            update_past_bytes(byte);
            if (divertBytes[bufferId] == TRUE) {
                // log_send_data(" LEN DIV ", 8);
                comms_send_byte(divertedBytesBufferId[bufferId], byte);
            } else {

                // Set the number of bytes in the bpacket
                bpacket->numBytes = byte;
            }
            // Set the number of data bytes expected
            numDataBytesExpected[bufferId] = byte;
            numDataBytesReceived[bufferId] = 0;

            // Update the expected byte to data bytes
            if (byte == 0) {
                expectedByteId[bufferId] = BPACKET_STOP_BYTE_UPPER_ID;
            } else {
                expectedByteId[bufferId] = BPACKET_DATA_BYTE_ID;
            }

            continue;
        }

        if (expectedByteId[bufferId] == BPACKET_CODE_BYTE_ID) {
            // if (bufferId == BUFFER_1_ID) {
            //     char y[20];
            //     sprintf(y, " COD BYTE[%i] ", byte);
            //     log_send_data(y, chars_get_num_bytes(y));
            // }
            update_past_bytes(byte);
            if (divertBytes[bufferId] == TRUE) {
                comms_send_byte(divertedBytesBufferId[bufferId], byte);
            } else {
                bpacket->code = byte;
            }
            expectedByteId[bufferId] = BPACKET_NUM_BYTES_BYTE_ID;
            continue;
        }

        if (expectedByteId[bufferId] == BPACKET_REQUEST_BYTE_ID) {
            // if (bufferId == BUFFER_1_ID) {
            //     char y[20];
            //     sprintf(y, " REQ BYTE[%i] ", byte);
            //     log_send_data(y, chars_get_num_bytes(y));
            // }
            update_past_bytes(byte);
            if (divertBytes[bufferId] == TRUE) {
                comms_send_byte(divertedBytesBufferId[bufferId], byte);
            } else {
                bpacket->request = byte;
            }
            expectedByteId[bufferId] = BPACKET_CODE_BYTE_ID;
            continue;
        }

        if (expectedByteId[bufferId] == BPACKET_SENDER_BYTE_ID) {
            // if (bufferId == BUFFER_1_ID) {
            //     char y[20];
            //     sprintf(y, " SND BYTE[%i] ", byte);
            //     log_send_data(y, chars_get_num_bytes(y));
            // }
            update_past_bytes(byte);
            if (divertBytes[bufferId] == TRUE) {
                comms_send_byte(divertedBytesBufferId[bufferId], byte);
            } else {
                bpacket->sender = byte;
            }
            expectedByteId[bufferId] = BPACKET_REQUEST_BYTE_ID;
            continue;
        }

        if (expectedByteId[bufferId] == BPACKET_RECEIVER_BYTE_ID) {
            // if (bufferId == BUFFER_1_ID) {
            //     log_send_data(" REC BYTE  ", 10);
            // }
            update_past_bytes(byte);
            // Determine if this bpacket needs to be diverted to another mcu or not
            switch (byte) {

                case BPACKET_ADDRESS_ESP32: // Required to divert bytes to ESP32
                    // if (bufferId == BUFFER_1_ID) {
                    //     log_send_data(" REC ESP ", 9);
                    // }
                    // log_send_data(" DIV ESP ", 9);
                    // Set the flag to divert bytes
                    divertBytes[bufferId]           = TRUE;
                    divertedBytesBufferId[bufferId] = BUFFER_1_ID;

                    // Send the start bytes and the receiver address to the ESP32
                    comms_send_byte(BUFFER_1_ID, BPACKET_START_BYTE_UPPER);
                    comms_send_byte(BUFFER_1_ID, BPACKET_START_BYTE_LOWER);
                    comms_send_byte(BUFFER_1_ID, BPACKET_ADDRESS_ESP32);
                    break;

                case BPACKET_ADDRESS_MAPLE: // Required to divert bytes to STM32
                    // log_send_data(" DIV MPL ", 9);
                    // if (bufferId == BUFFER_1_ID) {
                    //     log_send_data(" REC MPL", 9);
                    // }
                    // Set the flag to divert bytes
                    divertBytes[bufferId]           = TRUE;
                    divertedBytesBufferId[bufferId] = BUFFER_2_ID;

                    // Send the start bytes and the receiver address to Maple
                    comms_send_byte(BUFFER_2_ID, BPACKET_START_BYTE_UPPER);
                    comms_send_byte(BUFFER_2_ID, BPACKET_START_BYTE_LOWER);
                    comms_send_byte(BUFFER_2_ID, BPACKET_ADDRESS_MAPLE);
                    break;

                case BPACKET_ADDRESS_STM32: // No need to divert bytes
                                            // if (bufferId == BUFFER_1_ID) {
                                            //     log_send_data(" REC STM ", 9);
                                            // }
                default:                    // Unknown receiver address. Pass onto STM32 and let STM32 handle it
                    bpacket->receiver     = BPACKET_ADDRESS_STM32;
                    divertBytes[bufferId] = FALSE;
                    break;
            }

            expectedByteId[bufferId] = BPACKET_SENDER_BYTE_ID;
            continue;
        }

        if ((expectedByteId[bufferId] == BPACKET_START_BYTE_LOWER_ID) && (byte == BPACKET_START_BYTE_LOWER)) {
            // if (bufferId == BUFFER_1_ID) {
            //     log_send_data(" START LOWER ", 13);
            // }
            update_past_bytes(byte);
            expectedByteId[bufferId] = BPACKET_RECEIVER_BYTE_ID;
            continue;
        }

        if ((expectedByteId[bufferId] == BPACKET_START_BYTE_UPPER_ID) && (byte == BPACKET_START_BYTE_UPPER)) {
            // if (bufferId == BUFFER_1_ID) {
            //     log_send_data(" START UPPER ", 13);
            // }
            expectedByteId[bufferId] = BPACKET_START_BYTE_LOWER_ID;
            // Reset the byte index
            update_past_bytes(byte);
            bpacketByteIndex[bufferId] = 0;

            continue;
        }

        char m[40];
        if (bufferId == BUFFER_1_ID) {
            // comms_send_byte(BUFFER_2_ID, byte);
            sprintf(m, " F1[%i][%i][%i] Len: %i got: %i", expectedByteId[bufferId], byte, lastByte,
                    numDataBytesExpected[bufferId], numDataBytesReceived[bufferId]);
            log_send_data(m, chars_get_num_bytes(m));
        } else {
            sprintf(m, " F2[%i][%i][%i] Len: %i got: %i", expectedByteId[bufferId], byte, lastByte,
                    numDataBytesExpected[bufferId], numDataBytesReceived[bufferId]);
            log_send_data(m, chars_get_num_bytes(m));
        }

        for (int i = 0; i < 30; i++) {
            m[i] = '\0';
        }

        // Erraneous byte. Reset the system
        expectedByteId[bufferId] = BPACKET_START_BYTE_UPPER_ID;
        update_past_bytes(byte);
    }

    return FALSE;
}

/* Generic USART Commuincation Functions */

void comms_send_byte(uint8_t bufferId, uint8_t byte) {

    // Wait for USART to be ready to send a byte
    while ((uarts[bufferId]->ISR & USART_ISR_TXE) == 0) {};

    // Send byte of usart
    uarts[bufferId]->TDR = byte;
}

void comms_transmit(uint8_t bufferId, uint8_t* data, uint16_t numBytes) {

    for (int i = 0; i < numBytes; i++) {

        // Wait for USART to be ready to send a byte
        while ((uarts[bufferId]->ISR & USART_ISR_TXE) == 0) {};

        // Send byte of usart
        uarts[bufferId]->TDR = data[i];
    }
}