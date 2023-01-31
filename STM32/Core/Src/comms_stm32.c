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

int rxBuffer[RX_BUFFER_SIZE];
uint32_t bufferIndex = 0;

uint32_t pindex       = 0;
uint8_t passOnMessage = TRUE;
uint8_t bpacketIndex  = 0;

/* Function Prototyes */
void comms_send_byte(USART_TypeDef* usart, uint8_t byte);

void comms_stm32_init(void) {
    // Set the last index of the circular buffer to a stop byte
    // so the first start byte will be valid
    rxBuffer[RX_BUFFER_SIZE - 1] = BPACKET_STOP_BYTE;
}

void commms_stm32_increment_circ_buff_index(uint32_t* index, uint32_t bufferLength) {

    if (*index == (bufferLength - 1)) {
        *index = 0;
        return;
    }

    *index += 1;
}

void comms_add_to_buffer(uint8_t byte) {
    rxBuffer[bufferIndex] = byte;
    commms_stm32_increment_circ_buff_index(&bufferIndex, RX_BUFFER_SIZE);
}

uint8_t comms_process_rxbuffer(bpacket_t* bpacket) {

    uint32_t indexMin1, indexMin2;

    while (pindex != bufferIndex) {

        uint8_t byte = rxBuffer[pindex];
        // comms_send_byte(USART2, byte);
        // commms_stm32_increment_circ_buff_index(&pindex, RX_BUFFER_SIZE);
        // continue;

        if (pindex == 0) {
            indexMin1 = RX_BUFFER_SIZE - 1;
            indexMin2 = RX_BUFFER_SIZE - 2;
        } else if (pindex == 1) {
            indexMin1 = 0;
            indexMin2 = RX_BUFFER_SIZE - 1;
        } else {
            indexMin1 = pindex - 1;
            indexMin2 = pindex - 2;
        }

        if (byte == BPACKET_START_BYTE) {

            // If the byte just before was a stop byte we can be pretty
            // certain this is the start of a new packet
            if (rxBuffer[indexMin1] == BPACKET_STOP_BYTE) {
                bpacketIndex = 0;
                // char b[30];
                // sprintf(b, "start [%li]: ", pindex);
                // log_send_data("Start ", 6);
                commms_stm32_increment_circ_buff_index(&pindex, RX_BUFFER_SIZE);
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

            // If the data is being sent to the STM32, check whether the stop byte
            // is in the right
            // char msg[15];
            // sprintf(msg, "%i", bpacket->request);
            // log_send_data(" End", 4);

            // log_send_data(" end", 4);
            commms_stm32_increment_circ_buff_index(&pindex, RX_BUFFER_SIZE);
            return TRUE;
        }

        if (rxBuffer[indexMin1] == BPACKET_START_BYTE) {

            // If the byte before the start byte was a stop byte, very certain that
            // the current byte is the length of the packet
            if (rxBuffer[indexMin2] == BPACKET_STOP_BYTE) {
                commms_stm32_increment_circ_buff_index(&pindex, RX_BUFFER_SIZE);
                continue;
            }
        }

        if (rxBuffer[indexMin2] == BPACKET_START_BYTE) {

            // If the byte just before looks like a packet length byte, very certain that this is the
            // request byte
            if ((rxBuffer[indexMin1] > 0) && (rxBuffer[indexMin1] < (BPACKET_MAX_NUM_DATA_BYTES + 1))) {

                // Determine whether this packet is for the STM32 or is a message for another mcu
                switch (byte) {
                    case BPACKET_GEN_R_PING:
                    case BPACKET_GET_R_STATUS:
                    case BPACKET_GEN_R_HELP:
                    case WATCHDOG_BPK_R_SET_CAMERA_SETTINGS:
                    case WATCHDOG_BPK_R_GET_CAMERA_SETTINGS:
                    case WATCHDOG_BPK_R_GET_DATETIME:
                    case WATCHDOG_BPK_R_SET_DATETIME:
                        passOnMessage     = FALSE;
                        bpacket->numBytes = rxBuffer[indexMin1] - 1;
                        bpacket->request  = byte;
                        // char m[20];
                        // sprintf(m, " Req: [%i] ", bpacket->request);
                        // log_send_data(m, sizeof(m));
                        break;
                    default:
                        passOnMessage = TRUE;
                        // log_send_data(" pass msg ", 10);
                        // Send the last 3 bytes + this byte onwards
                        comms_send_byte(USART1, rxBuffer[indexMin2]);
                        comms_send_byte(USART1, rxBuffer[indexMin1]);
                        comms_send_byte(USART1, rxBuffer[pindex]);
                        commms_stm32_increment_circ_buff_index(&pindex, RX_BUFFER_SIZE);
                }

                commms_stm32_increment_circ_buff_index(&pindex, RX_BUFFER_SIZE);
                continue;
            }
        }

        // if (passOnMessage == TRUE) {
        //     comms_send_byte(USART1, byte);
        // } else {
        bpacket->bytes[bpacketIndex] = byte;
        // char msg[20];
        // sprintf(msg, "(%i)", bpacketIndex);
        // log_send_data(msg, 5);
        bpacketIndex++;
        // }

        commms_stm32_increment_circ_buff_index(&pindex, RX_BUFFER_SIZE);
    }

    return FALSE;
}

void comms_usart2_print_buffer(void) {

    char msg[300];
    msg[0] = '\0';

    int i;
    for (i = 0; i < bufferIndex; i++) {
        msg[i] = rxBuffer[i];
    }

    msg[i] = '\0';
    log_message(msg);
}

/* Generic USART Commuincation Functions */

void comms_send_byte(USART_TypeDef* usart, uint8_t byte) {

    // Wait for USART to be ready to send a byte
    while ((usart->ISR & USART_ISR_TXE) == 0) {};

    // Send byte of usart
    usart->TDR = byte;
}

void comms_transmit(USART_TypeDef* usart, uint8_t* data, uint16_t numBytes) {

    for (int i = 0; i < numBytes; i++) {

        // Wait for USART to be ready to send a byte
        while ((usart->ISR & USART_ISR_TXE) == 0) {};

        // Send byte of usart
        usart->TDR = data[i];
    }
}