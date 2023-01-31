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
uint8_t pastByte4 = 0;

// For some reason haven't worked it out, if you set this to false at the start of the rxProcessBuffer
// it will stuff up the diversion of bytes. Need to keep it global
uint8_t divertBytes = FALSE;

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
    // rxBuffers1[bufferIndex][] = byte;
}

void update_past_bytes(void) {
    pastByte4 = pastByte3;
    pastByte3 = pastByte2;
    pastByte2 = pastByte1;
    pastByte1 = byte;
}

uint8_t comms_process_rxbuffer(uint8_t bufferId, bpacket_t* bpacket) {

    // uint32_t indexMin1, indexMin2, indexMin3, indexMin4;
    // uint8_t divertBytes           = FALSE;
    uint8_t divertedBytesBufferId = bufferId;

    // while (pindex != bufferIndex) {
    while (rxBufProcessedIndexes[bufferId] != rxBufIndexes[bufferId]) {

        byte = rxBuffers1[bufferId][rxBufProcessedIndexes[bufferId]];
        // if (rxBufProcessedIndexes[bufferId] == 0) {
        //     indexMin1 = RX_BUFFER_SIZE - 1;
        //     indexMin2 = RX_BUFFER_SIZE - 2;
        //     indexMin3 = RX_BUFFER_SIZE - 3;
        //     indexMin4 = RX_BUFFER_SIZE - 4;
        // } else if (rxBufProcessedIndexes[bufferId] == 1) {
        //     indexMin1 = 0;
        //     indexMin2 = RX_BUFFER_SIZE - 1;
        //     indexMin3 = RX_BUFFER_SIZE - 2;
        //     indexMin4 = RX_BUFFER_SIZE - 3;
        // } else if (rxBufProcessedIndexes[bufferId] == 2) {
        //     indexMin1 = 1;
        //     indexMin2 = 0;
        //     indexMin3 = RX_BUFFER_SIZE - 1;
        //     indexMin4 = RX_BUFFER_SIZE - 2;
        // } else if (rxBufProcessedIndexes[bufferId] == 3) {
        //     indexMin1 = 1;
        //     indexMin2 = 0;
        //     indexMin3 = 2;
        //     indexMin4 = RX_BUFFER_SIZE - 1;
        // } else {
        //     indexMin1 = rxBufProcessedIndexes[bufferId] - 1;
        //     indexMin2 = rxBufProcessedIndexes[bufferId] - 2;
        //     indexMin3 = rxBufProcessedIndexes[bufferId] - 3;
        //     indexMin4 = rxBufProcessedIndexes[bufferId] - 4;
        // }

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
            // if (rxBuffers1[bufferId][indexMin1] == BPACKET_START_BYTE) {

            // If the byte before the start byte was a stop byte, very certain that
            // the current byte is the receiver of the bpacket
            // if (rxBuffers1[bufferId][indexMin2] == BPACKET_STOP_BYTE) {
            if (pastByte2 == BPACKET_STOP_BYTE) {

                // Determine whether we need to divert this packet or whether this packet
                // is for this mcu
                // log_send_data(" SEE ", 5);
                if (byte == BPACKET_ADDRESS_ESP32) {
                    divertedBytesBufferId = BUFFER_1_ID;
                    divertBytes           = TRUE;
                    comms_send_byte(divertedBytesBufferId, BPACKET_START_BYTE);
                    comms_send_byte(divertedBytesBufferId, byte);
                } else if (byte == BPACKET_ADDRESS_STM32) {
                    // log_send_data(" STM ", 5);
                    divertBytes       = FALSE;
                    bpacket->receiver = BPACKET_ADDRESS_STM32;
                } else if (byte == BPACKET_ADDRESS_MAPLE) {
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
            // if (rxBuffers1[bufferId][indexMin2] == BPACKET_START_BYTE) {

            // If the byte in front the start byte has a value of a valid address
            // pretty certain this is the sender of the bpacket
            // if ((rxBuffers1[bufferId][indexMin1] >= BPACKET_MIN_ADDRESS) &&
            //     (rxBuffers1[bufferId][indexMin1] <= BPACKET_MAX_ADDRESS)) {
            if ((pastByte1 >= BPACKET_MIN_ADDRESS) && (pastByte1 <= BPACKET_MAX_ADDRESS)) {

                // Check whether the packet needs to be diverted or not
                if (divertBytes == TRUE) {
                    // log_send_data(" sdr passed ", 12);
                    comms_send_byte(divertedBytesBufferId, byte);
                } else {
                    // log_send_data(" sdr added ", 11);
                    bpacket->sender = byte;
                }
                commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                update_past_bytes();
                continue;
            }
        }

        if (pastByte3 == BPACKET_START_BYTE) {
            // if (rxBuffers1[bufferId][indexMin3] == BPACKET_START_BYTE) {

            // If the byte just before looks like a sender byte, very certain that this is the
            // request byte
            // if ((rxBuffers1[bufferId][indexMin1] >= BPACKET_MIN_ADDRESS) &&
            //     (rxBuffers1[bufferId][indexMin1] <= BPACKET_MAX_ADDRESS)) {
            if ((pastByte1 >= BPACKET_MIN_ADDRESS) && (pastByte1 <= BPACKET_MAX_ADDRESS)) {

                // Check whether the packet needs to be diverted or not
                if (divertBytes == TRUE) {
                    // log_send_data(" req passed ", 12);
                    comms_send_byte(divertedBytesBufferId, byte);
                } else {
                    // log_send_data(" req added ", 11);
                    bpacket->request = byte;
                }
                commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                update_past_bytes();
                continue;

                // Determine whether this packet is for the STM32 or is a message for another mcu
                // switch (byte) {
                //     case BPACKET_GEN_R_PING:
                //     case BPACKET_GET_R_STATUS:
                //     case BPACKET_GEN_R_HELP:
                //     case WATCHDOG_BPK_R_SET_CAMERA_SETTINGS:
                //     case WATCHDOG_BPK_R_GET_CAMERA_SETTINGS:
                //     case WATCHDOG_BPK_R_GET_DATETIME:
                //     case WATCHDOG_BPK_R_SET_DATETIME:
                //         // case WATCHDOG_BPK_R_LED_RED_ON:
                //         // case WATCHDOG_BPK_R_LED_RED_OFF:
                //         passOnMessage     = FALSE;
                //         bpacket->numBytes = rxBuffers1[bufferId][indexMin1] - 1;
                //         bpacket->request  = byte;
                //         // char m[20];
                //         // sprintf(m, " Req: [%i] ", bpacket->request);
                //         // log_send_data(m, sizeof(m));
                //         break;
                //     default:
                //         passOnMessage = TRUE;
                //         log_send_data(" pass msg ", 10);
                //         // Send the last 3 bytes + this byte onwards
                //         if (bufferId == BUFFER_1_ID) {
                //             comms_send_byte(BUFFER_2_ID, rxBuffers1[bufferId][indexMin2]);
                //             comms_send_byte(BUFFER_2_ID, rxBuffers1[bufferId][indexMin1]);
                //             comms_send_byte(BUFFER_2_ID, rxBuffers1[bufferId][rxBufProcessedIndexes[bufferId]]);

                //         } else {
                //             comms_send_byte(BUFFER_1_ID, rxBuffers1[bufferId][indexMin2]);
                //             comms_send_byte(BUFFER_1_ID, rxBuffers1[bufferId][indexMin1]);
                //             comms_send_byte(BUFFER_1_ID, rxBuffers1[bufferId][rxBufProcessedIndexes[bufferId]]);
                //         }
                //         // comms_send_byte(bufferId, rxBuffers1[bufferId][indexMin2]);
                //         // comms_send_byte(bufferId, rxBuffers1[bufferId][indexMin1]);
                //         // comms_send_byte(bufferId, rxBuffers1[bufferId][rxBufProcessedIndexes[bufferId]]);
                //         commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                // }

                // commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                // continue;
            }
        }

        if (pastByte4 == BPACKET_START_BYTE) {
            // if (rxBuffers1[bufferId][indexMin4] == BPACKET_START_BYTE) {

            // If the second last byte looks like the sender, we can be pretty sure
            // this is the length byte
            // if ((rxBuffers1[bufferId][indexMin2] >= BPACKET_MIN_ADDRESS) &&
            //     (rxBuffers1[bufferId][indexMin2] <= BPACKET_MAX_ADDRESS)) {
            if ((pastByte2 >= BPACKET_MIN_ADDRESS) && (pastByte2 <= BPACKET_MAX_ADDRESS)) {

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

        // if (passOnMessage == TRUE) {
        //     if (bufferId == BUFFER_1_ID) {
        //         comms_send_byte(BUFFER_1_ID, byte);
        //     } else {
        //         comms_send_byte(BUFFER_2_ID, byte);
        //     }
        // } else {
        //     bpacket->bytes[bpacketIndexes[bufferId]] = byte;
        //     bpacketIndexes[bufferId]++;
        // }
        // bpacket->bytes[bpacketIndex] = byte;
        // char msg[20];
        // sprintf(msg, "(%i)", bpacketIndex);
        // log_send_data(msg, 5);
        // bpacketIndex++;

        // commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
    }

    return FALSE;
}

void comms_print_buffer(uint8_t bufferId) {

    char msg[300];
    msg[0] = '\0';

    int i;
    for (i = 0; i < rxBufIndexes[bufferId]; i++) {
        msg[i] = rxBuffers1[bufferId][i];
    }

    msg[i] = '\0';
    log_message(msg);
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