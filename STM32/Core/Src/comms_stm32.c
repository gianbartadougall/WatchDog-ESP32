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

#define PACKET_BUFFER_MAX_PACKETS 6

#define RX_BUFFER_SIZE (BPACKET_BUFFER_LENGTH_BYTES * PACKET_BUFFER_MAX_PACKETS)
#define BYPASS         0
#define NO_BYPASS      1

bpacket_t packetBuffer[PACKET_BUFFER_MAX_PACKETS];
int rxBuffer[RX_BUFFER_SIZE];
int bufferIndex = 0;

int packetLengthFlag  = FALSE;
int packetCommandFlag = FALSE;
int packetByteIndex   = 0;
int startByteIndex    = 0;
uint8_t lastByte      = 0;

uint8_t packetType = NO_BYPASS;

int* flag;

uint8_t packetBufferIndex  = 0;
uint8_t packetPendingIndex = 0;
bpacket_t packetBuffer[PACKET_BUFFER_SIZE];

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

void comms_stm32_init(int* commsFlag) {
    flag = commsFlag;
}

void comms_process_byte(uint8_t byte) {

    // uint8_t c[1];
    // uint8_t lastChar  = BPACKET_STOP_BYTE;
    // int numBytes      = 0;
    // packetBufferIndex = 0;

    // int bufferIndex = 0;

    // int packetLengthFlag  = FALSE;
    // int packetCommandFlag = FALSE;
    // int packetByteIndex   = 0;
    // int startByteIndex    = 0;

    // // while ((numBytes = sp_blocking_read(port, c, 1, 0)) > 0) {
    // while ((numBytes = com_ports_read(c, 1, 0)) > 0) {

    if (bufferIndex == RX_BUFFER_SIZE) {
        bufferIndex = 0;
    }

    rxBuffer[bufferIndex++] = byte;

    if (packetLengthFlag == TRUE) {
        // Subtract 1 because this number includes the request
        packetBuffer[packetBufferIndex].numBytes = (byte - 1);
        packetLengthFlag                         = FALSE;
        packetCommandFlag                        = TRUE;
        lastByte                                 = byte;
        continue;
    }

    if (packetCommandFlag == TRUE) {
        packetBuffer[packetBufferIndex].request = byte;
        packetCommandFlag                       = FALSE;
        lastByte                                = byte;
        continue;
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
            continue;
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

            continue;
        }
    }

    // Byte is a data byte, add to the bpacket
    packetBuffer[packetBufferIndex].bytes[packetByteIndex++] = byte;

    // }

    // if (numBytes < 0) {
    //     printf("Error reading COM port\n");
    // }

    // return FALSE;
}

uint8_t comms_stm32_get_bpacket(bpacket_t* bpacket) {

    if (packetPendingIndex != packetBufferIndex) {
        bpacket = &packetBuffer[packetPendingIndex];
        maple_increment_packet_pending_index();
        return TRUE;
    }

    return FALSE;
}