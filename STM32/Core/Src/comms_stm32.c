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

#define PACKET_BUFFER_SIZE 6

#define RX_BUFFER_SIZE (BPACKET_BUFFER_LENGTH_BYTES * PACKET_BUFFER_SIZE)

bpacket_t packetBuffer[PACKET_BUFFER_SIZE];
int rxBuffer[RX_BUFFER_SIZE];
int bufferIndex = 0;

int packetLengthFlag  = FALSE;
int packetCommandFlag = FALSE;
int packetByteIndex   = 0;
int startByteIndex    = 0;
uint8_t lastByte      = 0;

int processedBufferIndex = 0;

uint32_t* flag;

uint8_t packetBufferIndex  = 0;
uint8_t packetPendingIndex = 0;
bpacket_t packetBuffer[PACKET_BUFFER_SIZE];

uint8_t passOnMessage = TRUE;

/* Function Prototyes */
void comms_send_byte(USART_TypeDef usart, uint8_t byte);

void comms_stm32_increment_packet_buffer_index(void) {
    packetBufferIndex++;

    if (packetBufferIndex == PACKET_BUFFER_SIZE) {
        packetBufferIndex = 0;
    }
}

void comms_stm32_increment_packet_pending_index(void) {
    packetPendingIndex++;

    if (packetPendingIndex == PACKET_BUFFER_SIZE) {
        packetPendingIndex = 0;
    }
}

void comms_stm32_init(uint32_t* commsFlag) {
    flag = commsFlag;
}

// void comms_add_to_buffer(uint8_t byte) {

//     if (bufferIndex == RX_BUFFER_SIZE) {
//         bufferIndex = 0;
//     }

//     rxBuffer[bufferIndex++] = byte;

// }

void comms_add_to_buffer(uint8_t byte) {

    // if (bufferIndex == RX_BUFFER_SIZE) {
    //     bufferIndex = 0;
    // }

    rxBuffer[processedBufferIndex++] = byte;

    if (packetLengthFlag == TRUE) {
        // Subtract 1 because this number includes the request
        packetBuffer[packetBufferIndex].numBytes = (byte - 1);
        packetLengthFlag                         = FALSE;
        packetCommandFlag                        = TRUE;
        lastByte                                 = byte;
        return;
    }

    if (packetCommandFlag == TRUE) {
        packetBuffer[packetBufferIndex].request = byte;
        packetCommandFlag                       = FALSE;
        lastByte                                = byte;
        return;
    }

    if (byte == BPACKET_START_BYTE) {

        // Need to determine whether the character recieved is a start byte or part
        // of a message. We can be very confident the character represents a start
        // byte if:
        // 1) The character that was recieved previously was a stop byte, and
        // 2) The start byte agrees with the length of the last message that
        //      was recieved

        if (lastByte == BPACKET_STOP_BYTE) {

            // Check whether the length of the message agrees with the position
            // of this recieved start byte
            // if (bufferIndex == (stopByteIndex + 1)) {
            //     // Very certain that this byte is the start of a new packet
            // }

            // Start of new packet
            packetLengthFlag = TRUE;
            startByteIndex   = bufferIndex - 1; // Minus 1 because one is added at the top
            lastByte         = byte;

            // printf("Start byte recieved\n");
            // printf("Start byte acted upon\n");
            return;
        }
    }

    if (byte == BPACKET_STOP_BYTE) {
        // printf("Buffer index: %i Start index: %i\n", bufferIndex, startByteIndex);

        // Calculate the
        int stopByteIndex = (bufferIndex == 0 ? RX_BUFFER_SIZE : bufferIndex) - 1;

        // The added 3 is for the start byte and the second byte in the bpacket that holds
        // the number of data bytes the packet should contain and the request byte
        int predictedStopByteIndex = (startByteIndex + 3 + packetBuffer[packetBufferIndex].numBytes) % RX_BUFFER_SIZE;

        // Need to determine whether the character recieved was a stop byte or
        // part of the message. If the stop byte is in the predicted spot, then
        // we can be farily certain that this byte is a stop byte
        if (stopByteIndex == predictedStopByteIndex) {

            // Very sure this is the end of a packet. Reset byte index to 0
            packetByteIndex = 0;
            lastByte        = byte;

            // Set interrupt flag so watchdog processes buffer straight away
            *flag |= COMMS_STM32_BPACKET_READY;

            comms_stm32_increment_packet_buffer_index();

            return;
        }
    }

    if (passOnMessage == TRUE) {
        // Send byte onto uart 1
        comms_send_byte(USART1, byte);

    } else {
        // Store byte into buffer
    }

    // Byte is a data byte, add to the bpacket
    packetBuffer[packetBufferIndex].bytes[packetByteIndex++] = byte;
}

uint8_t comms_stm32_get_bpacket(bpacket_t* bpacket) {

    if (packetPendingIndex != packetBufferIndex) {
        bpacket = &packetBuffer[packetPendingIndex];
        comms_stm32_increment_packet_pending_index();
        return TRUE;
    }

    return FALSE;
}

void comms_usart2_print_buffer(void) {

    char msg[100];
    msg[0]  = '\0';
    int i   = 0;
    int buf = processedBufferIndex;

    if (buf > bufferIndex) {
        while (buf > bufferIndex) {
            msg[i++] = rxBuffer[buf++];

            if (buf == RX_BUFFER_SIZE) {
                buf = 0;
            }
        }
    }

    while (buf < bufferIndex) {
        msg[i++] = rxBuffer[buf++];
    }

    log_message(msg);
}

/* Generic USART Commuincation Functions */

void comms_send_byte(USART_TypeDef usart, uint8_t byte) {

    // Wait for USART to be ready to send a byte
    while ((usart->ISR & USART_ISR_TXE) == 0) {};

    // Send byte of usart
    usart->TDR = byte;
}