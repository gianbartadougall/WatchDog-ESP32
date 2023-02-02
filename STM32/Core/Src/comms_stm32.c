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

#define PACKET_BUFFER_SIZE 1

#define RX_BUFFER_SIZE (BPACKET_BUFFER_LENGTH_BYTES * PACKET_BUFFER_SIZE)

/* Private Macros */

/* Private Variables */
USART_TypeDef* uarts[NUM_BUFFERS] = {
    BUFFER_1,
    BUFFER_2,
};

// Create a list for every single UART line
uint8_t rxBuffers1[NUM_BUFFERS][RX_BUFFER_SIZE] = {{0}, {0}};
uint32_t rxBufIndexes[NUM_BUFFERS];
uint32_t rxBufProcessedIndexes[NUM_BUFFERS];
uint8_t bpacketIndexes[NUM_BUFFERS];

#define BUFFER(id) (rxBuffers1[id][rxBufIndexes[id]])

uint8_t byte      = 0;
uint8_t pastByte1 = 0;
uint8_t pastByte2 = 0;
uint8_t pastByte3 = 0;

// For some reason haven't worked it out, if you set this to false at the start of the rxProcessBuffer
// it will stuff up the diversion of bytes. Need to keep it global
uint8_t divertBytes           = FALSE;
uint8_t divertedBytesBufferId = BUFFER_1_ID;

/* Function Prototyes */
void comms_send_byte(uint8_t bufferId, uint8_t byte);

void comms_stm32_init(void) {

    pastByte1 = BPACKET_STOP_BYTE;
    // Initialise all the buffers so their last value is the stop byte of a bpacket.
    // This ensures the first start byte recieved will be valid
    for (int i = 0; i < NUM_BUFFERS; i++) {
        //  rxBuffers1[i][RX_BUFFER_SIZE - 1] = BPACKET_STOP_BYTE;
        rxBufIndexes[i]          = 0;
        bpacketIndexes[i]        = 0;
        rxBufProcessedIndexes[i] = 0;
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

void update_past_bytes(void) {
    pastByte3 = pastByte2;
    pastByte2 = pastByte1;
    pastByte1 = byte;
}

uint8_t comms_process_rxbuffer(uint8_t bufferId, bpacket_t* bpacket) {

    while (rxBufProcessedIndexes[bufferId] != rxBufIndexes[bufferId]) {

        byte = rxBuffers1[bufferId][rxBufProcessedIndexes[bufferId]];

        if (byte == BPACKET_START_BYTE) {

            // If the byte just before was a stop byte we can be pretty
            // certain this is the start of a new packet
            if (pastByte1 == BPACKET_STOP_BYTE) {
                // if (rxBuffers1[bufferId][indexMin1] == BPACKET_STOP_BYTE) {
                bpacketIndexes[bufferId] = 0;
                // bpacketIndex = 0;
                // char b[30];
                // sprintf(b, "start [%li]: ", pindex);
                // log_send_data("Start ", 6);
                commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                update_past_bytes();
                continue;
            } else {
                // char msg[40];
                // sprintf(msg, "rx= {%i} {%i} {%i} {%i} {%i}", byte, pastByte1, pastByte2, pastByte3, pastByte4);
                // log_send_data(msg, sizeof(msg));
                // char msg[40];
                // sprintf(msg, "rx= {%i} {%i} {%i} {%i} {%i}", byte, pastByte1, pastByte2, pastByte3, pastByte4);
                // log_send_data(msg, sizeof(msg));
            }

            // Assume the start byte is actually a piece of data
        }

        if (pastByte1 == BPACKET_START_BYTE) {

            // If the byte before the start byte was a stop byte, very certain that
            // the current byte is the address byte of the bpacket
            if (pastByte2 == BPACKET_STOP_BYTE) {

                uint8_t receiver = BPACKET_BYTE_TO_RECEIVER(byte);

                // Determine whether we need to divert this packet or whether this packet
                // is for this mcu
                if (receiver == BPACKET_ADDRESS_ESP32) {
                    // log_send_data(" ESP ", 5);
                    divertedBytesBufferId = BUFFER_1_ID;
                    divertBytes           = TRUE;
                    comms_send_byte(divertedBytesBufferId, BPACKET_START_BYTE);
                    comms_send_byte(divertedBytesBufferId, byte);
                } else if (receiver == BPACKET_ADDRESS_STM32) {
                    // log_send_data(" STM ", 5);
                    divertBytes       = FALSE;
                    bpacket->receiver = receiver;
                    bpacket->sender   = BPACKET_BYTE_TO_SENDER(byte);
                } else if (receiver == BPACKET_ADDRESS_MAPLE) {
                    // log_send_data(" MPL ", 5);
                    divertBytes           = TRUE;
                    divertedBytesBufferId = BUFFER_2_ID;
                    comms_send_byte(divertedBytesBufferId, BPACKET_START_BYTE);
                    comms_send_byte(divertedBytesBufferId, byte);
                }

                commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                update_past_bytes();
                continue;
            }
        }

        if (pastByte2 == BPACKET_START_BYTE) {

            // If the byte just before looks like a sender byte, very certain that this is the
            // request byte
            uint8_t receiver = BPACKET_BYTE_TO_RECEIVER(pastByte1);
            uint8_t sender   = BPACKET_BYTE_TO_SENDER(pastByte1);
            if ((receiver <= BPACKET_MAX_ADDRESS) && (sender <= BPACKET_MAX_ADDRESS)) {

                // Pretty sure this is the request byte. Check whether the packet needs to
                // be diverted or not

                if (divertBytes == TRUE) {
                    // log_send_data(" req passed ", 12);
                    comms_send_byte(divertedBytesBufferId, byte);
                } else {
                    // log_send_data(" req added ", 11);
                    uint8_t request  = BPACKET_BYTE_TO_REQUEST(byte);
                    uint8_t code     = BPACKET_BYTE_TO_CODE(byte);
                    bpacket->request = request;
                    bpacket->code    = code;
                }
                commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                update_past_bytes();
                continue;
            }
        }

        if (pastByte3 == BPACKET_START_BYTE) {
            // If the second last byte looks like the sender, we can be pretty sure
            // this is the length byte
            uint8_t receiver = BPACKET_BYTE_TO_RECEIVER(pastByte2);
            uint8_t sender   = BPACKET_BYTE_TO_SENDER(pastByte2);
            if ((receiver <= BPACKET_MAX_ADDRESS) && (sender <= BPACKET_MAX_ADDRESS)) {

                // Pretty sure this is the length byte
                if (divertBytes == TRUE) {
                    // log_send_data(" len passed ", 12);
                    comms_send_byte(divertedBytesBufferId, byte);
                } else {
                    // log_send_data(" len added ", 11);
                    bpacket->numBytes = byte;
                }
                commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                update_past_bytes();
                continue;
            }
        }

        if (byte == BPACKET_STOP_BYTE) {

            if (divertBytes == TRUE) {
                comms_send_byte(divertedBytesBufferId, byte);
                commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                update_past_bytes();
                // log_send_data(" end false", 10);
                return FALSE;
            }

            commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
            update_past_bytes();
            // log_send_data(" end true", 9);
            return TRUE;
        }

        if (divertBytes) {
            comms_send_byte(divertedBytesBufferId, byte);
        } else {
            bpacket->bytes[bpacketIndexes[bufferId]] = byte;
            bpacketIndexes[bufferId]++;
        }
        commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
        update_past_bytes();
    }

    return FALSE;
}

// void comms_print_buffer(uint8_t bufferId) {

//     char msg[300];
//     msg[0] = '\0';

//     int i;
//     for (i = 0; i < rxBufIndexes[bufferId]; i++) {
//         msg[i] = rxBuffers1[bufferId][i];
//     }

//     msg[i] = '\0';
//     log_message(msg);
// }

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