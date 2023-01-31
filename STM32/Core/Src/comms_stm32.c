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

// uint8_t rxBuffer[RX_BUFFER_SIZE];

// uint32_t bufferIndex = 0;

// uint32_t pindex       = 0;
uint8_t passOnMessage = TRUE;
// uint8_t bpacketIndex  = 0;

/* Function Prototyes */
void comms_send_byte(uint8_t bufferId, uint8_t byte);

void comms_stm32_init(void) {

    // Initialise all the buffers so their last value is the stop byte of a bpacket.
    // This ensures the first start byte recieved will be valid
    for (int i = 0; i < NUM_BUFFERS; i++) {
        rxBuffers1[i][RX_BUFFER_SIZE - 1] = BPACKET_STOP_BYTE;
        rxBufIndexes[i]                   = 0;
        bpacketIndexes[i]                 = 0;
        rxBufProcessedIndexes[i]          = 0;
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

uint8_t comms_process_rxbuffer(uint8_t bufferId, bpacket_t* bpacket) {

    uint32_t indexMin1, indexMin2;

    // while (pindex != bufferIndex) {
    while (rxBufProcessedIndexes[bufferId] != rxBufIndexes[bufferId]) {

        uint8_t byte = rxBuffers1[bufferId][rxBufProcessedIndexes[bufferId]];
        // uint8_t byte = rxBuffer[pindex];
        // comms_send_byte(bufferId, byte);
        // commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
        // continue;

        if (rxBufProcessedIndexes[bufferId] == 0) {
            indexMin1 = RX_BUFFER_SIZE - 1;
            indexMin2 = RX_BUFFER_SIZE - 2;
        } else if (rxBufProcessedIndexes[bufferId] == 1) {
            indexMin1 = 0;
            indexMin2 = RX_BUFFER_SIZE - 1;
        } else {
            indexMin1 = rxBufProcessedIndexes[bufferId] - 1;
            indexMin2 = rxBufProcessedIndexes[bufferId] - 2;
        }

        if (byte == BPACKET_START_BYTE) {

            // If the byte just before was a stop byte we can be pretty
            // certain this is the start of a new packet
            if (rxBuffers1[bufferId][indexMin1] == BPACKET_STOP_BYTE) {
                bpacketIndexes[bufferId] = 0;
                // bpacketIndex = 0;
                // char b[30];
                // sprintf(b, "start [%li]: ", pindex);
                // log_send_data("Start ", 6);
                commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                continue;
            } else {
                // char msg[30];
                // sprintf(msg, "rx= {%i} {%i} {%i} {req: %i}", rxBuffer[indexMin2], rxBuffer[indexMin1],
                // rxBuffer[pindex],
                //         bpacket->request);
                // log_send_data(msg, sizeof(msg));
            }

            // Assume the start byte is actually a piece of data
        }

        if (byte == BPACKET_STOP_BYTE) {

            if (passOnMessage) {
                if (bufferId == BUFFER_1_ID) {
                    comms_send_byte(BUFFER_1_ID, byte);

                } else {
                    comms_send_byte(BUFFER_2_ID, byte);
                }
                continue;
            }

            // If the data is being sent to the STM32, check whether the stop byte
            // is in the right
            // char msg[15];
            // sprintf(msg, "%i", bpacket->request);
            // log_send_data(" End", 4);

            // log_send_data(" end", 4);
            commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
            return TRUE;
        }

        if (rxBuffers1[bufferId][indexMin1] == BPACKET_START_BYTE) {

            // If the byte before the start byte was a stop byte, very certain that
            // the current byte is the length of the packet
            if (rxBuffers1[bufferId][indexMin2] == BPACKET_STOP_BYTE) {
                commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                continue;
            }
        }

        if (rxBuffers1[bufferId][indexMin2] == BPACKET_START_BYTE) {

            // If the byte just before looks like a packet length byte, very certain that this is the
            // request byte
            if ((rxBuffers1[bufferId][indexMin1] > 0) &&
                (rxBuffers1[bufferId][indexMin1] < (BPACKET_MAX_NUM_DATA_BYTES + 1))) {

                // Determine whether this packet is for the STM32 or is a message for another mcu
                switch (byte) {
                    case BPACKET_GEN_R_PING:
                    case BPACKET_GET_R_STATUS:
                    case BPACKET_GEN_R_HELP:
                    case WATCHDOG_BPK_R_SET_CAMERA_SETTINGS:
                    case WATCHDOG_BPK_R_GET_CAMERA_SETTINGS:
                    case WATCHDOG_BPK_R_GET_DATETIME:
                    case WATCHDOG_BPK_R_SET_DATETIME:
                        // case WATCHDOG_BPK_R_LED_RED_ON:
                        // case WATCHDOG_BPK_R_LED_RED_OFF:
                        passOnMessage     = FALSE;
                        bpacket->numBytes = rxBuffers1[bufferId][indexMin1] - 1;
                        bpacket->request  = byte;
                        // char m[20];
                        // sprintf(m, " Req: [%i] ", bpacket->request);
                        // log_send_data(m, sizeof(m));
                        break;
                    default:
                        passOnMessage = TRUE;
                        log_send_data(" pass msg ", 10);
                        // Send the last 3 bytes + this byte onwards
                        if (bufferId == BUFFER_1_ID) {
                            comms_send_byte(BUFFER_2_ID, rxBuffers1[bufferId][indexMin2]);
                            comms_send_byte(BUFFER_2_ID, rxBuffers1[bufferId][indexMin1]);
                            comms_send_byte(BUFFER_2_ID, rxBuffers1[bufferId][rxBufProcessedIndexes[bufferId]]);

                        } else {
                            comms_send_byte(BUFFER_1_ID, rxBuffers1[bufferId][indexMin2]);
                            comms_send_byte(BUFFER_1_ID, rxBuffers1[bufferId][indexMin1]);
                            comms_send_byte(BUFFER_1_ID, rxBuffers1[bufferId][rxBufProcessedIndexes[bufferId]]);
                        }
                        // comms_send_byte(bufferId, rxBuffers1[bufferId][indexMin2]);
                        // comms_send_byte(bufferId, rxBuffers1[bufferId][indexMin1]);
                        // comms_send_byte(bufferId, rxBuffers1[bufferId][rxBufProcessedIndexes[bufferId]]);
                        commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                }

                commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
                continue;
            }
        }

        if (passOnMessage == TRUE) {
            if (bufferId == BUFFER_1_ID) {
                comms_send_byte(BUFFER_1_ID, byte);
            } else {
                comms_send_byte(BUFFER_2_ID, byte);
            }
        } else {
            bpacket->bytes[bpacketIndexes[bufferId]] = byte;
            bpacketIndexes[bufferId]++;
        }
        // bpacket->bytes[bpacketIndex] = byte;
        // char msg[20];
        // sprintf(msg, "(%i)", bpacketIndex);
        // log_send_data(msg, 5);
        // bpacketIndex++;

        commms_stm32_increment_circ_buff_index(&rxBufProcessedIndexes[bufferId], RX_BUFFER_SIZE);
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