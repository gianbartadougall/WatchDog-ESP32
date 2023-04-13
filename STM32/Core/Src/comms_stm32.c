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
#include "utils.h"
#include "log.h"
#include "watchdog_defines.h"
#include "chars.h"
#include "stm32_uart.h"
#include "cbuffer.h"

#define PACKET_BUFFER_SIZE 10

#define RX_BUFFER_SIZE (BPACKET_BUFFER_LENGTH_BYTES * PACKET_BUFFER_SIZE)

/* Private Macros */

/* Private Variables */
USART_TypeDef* uarts[NUM_BUFFERS] = {
    BUFFER_1,
    BUFFER_2,
};

uint8_t bpacketByteIndex[NUM_BUFFERS];
uint8_t divertBytes[NUM_BUFFERS];

cbuffer_t rxBuffersNew[2];
uint8_t rxBufNew[2][RX_BUFFER_SIZE] = {{0}, {0}};

cbuffer_t ByteBuffer[2];
uint8_t byteBuffer[8];

bpk_packet_t ErrorPacket;
bpk_buffer_t ErrorBuffer;
char errMsg[50];

/* Function Prototyes */
void comms_send_byte(uint8_t bufferId, uint8_t byte);

void comms_stm32_init(void) {

    // Initialise both circular buffers for expected bytes
    bpk_utils_init_expected_byte_buffer(byteBuffer);
    for (int i = 0; i < 2; i++) {
        cbuffer_init(&ByteBuffer[i], (void*)byteBuffer, sizeof(uint8_t), 8);
    }

    // Initialise both circular buffers for storing rx data
    for (int i = 0; i < 2; i++) {
        cbuffer_init(&rxBuffersNew[i], (void*)rxBufNew[i], sizeof(uint8_t), RX_BUFFER_SIZE);
    }
}

void comms_add_to_buffer(uint8_t bufferId, uint8_t byte) {
    cbuffer_write_element(&rxBuffersNew[bufferId], (void*)(&byte));
}

uint8_t comms_process_rxbuffer(uint8_t bufferId, bpk_packet_t* Bpacket) {

    uint8_t byte, expectedByte;

    while (cbuffer_read_next_element(&rxBuffersNew[bufferId], (void*)(&byte)) == TRUE) {

        // Read the current index of the byte buffer and store the result into
        // expected byte
        cbuffer_read_current_byte(&ByteBuffer[bufferId], (void*)(&expectedByte));

        switch (expectedByte) {

            /* Expecting bpacket 'data' bytes */
            case BPK_BYTE_DATA:

                if (divertBytes[bufferId] == TRUE) {
                    comms_send_byte(bufferId, byte);
                } else {
                    Bpacket->Data.bytes[bpacketByteIndex[bufferId]] = byte;
                }

                // Increment the byte index
                bpacketByteIndex[bufferId]++;

                // Skip resetting if there are more date bytes to read
                if (bpacketByteIndex[bufferId] < Bpacket->Data.numBytes) {
                    continue;
                }

                // No more data bytes to read. Execute request and then exit switch statement
                // to reset
                cbuffer_reset_read_index(&ByteBuffer[bufferId]);
                bpacketByteIndex[bufferId] = 0;
                return TRUE;

            /* Expecting bpacket 'length' byte */
            case BPK_BYTE_LENGTH:

                // Store the length and updated the expected byte
                Bpacket->Data.numBytes = byte;
                cbuffer_increment_read_index(&ByteBuffer[bufferId]);

                if (Bpacket->Receiver.val != BPK_ADDRESS_STM32) {

                    divertBytes[bufferId] = TRUE;
                    uint8_t bufId;
                    if (Bpacket->Receiver.val == BPK_ADDRESS_ESP32) {
                        bufId = BUFFER_1_ID;
                    } else {
                        bufId = BUFFER_2_ID;
                    }

                    comms_send_byte(bufId, BPK_BYTE_START_BYTE_UPPER);
                    comms_send_byte(bufId, BPK_BYTE_START_BYTE_LOWER);
                    comms_send_byte(bufId, Bpacket->Receiver.val);
                    comms_send_byte(bufId, Bpacket->Sender.val);
                    comms_send_byte(bufId, Bpacket->Request.val);
                    comms_send_byte(bufId, Bpacket->Code.val);
                    comms_send_byte(bufId, Bpacket->Data.numBytes);
                } else {
                    divertBytes[bufferId] = FALSE;
                }

                // Skip reseting if there is data to be read
                if (Bpacket->Data.numBytes > 0) {
                    continue;
                }

                // No data in Bpacket. Execute request and exit switch statement to reset
                cbuffer_reset_read_index(&ByteBuffer[bufferId]);
                bpacketByteIndex[bufferId] = 0;

                if (divertBytes[bufferId] == TRUE) {
                    return FALSE;
                }

                return TRUE;

            /* Expecting another bpacket byte that is not 'data' or 'length' */
            default:

                cbuffer_increment_read_index(&ByteBuffer[bufferId]);
                // If decoding the byte succeeded, move to the next expected byte by
                // incrementing the circrular buffer holding the expected bytes
                if (bpk_utils_decode_non_data_byte(Bpacket, expectedByte, byte) == TRUE) {
                    continue;
                }
        }

        // Decoding failed, print error.
        sprintf(errMsg, "Expected %i. Found %i (%c)", expectedByte, byte, byte);
        bpacket_create_sp(&ErrorPacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message,
                          BPK_Code_Error, errMsg);

        bpacket_to_buffer(&ErrorPacket, &ErrorBuffer);
        uart_transmit_data(USART2, ErrorBuffer.buffer, ErrorBuffer.numBytes);

        // Reset the expected byte back to the start and bpacket buffer index
        cbuffer_reset_read_index(&ByteBuffer[bufferId]);
        bpacketByteIndex[bufferId] = 0;
    }

    return FALSE;
}

uint8_t comms_stm32_request_pending(uint8_t bufferId) {
    return cbuffer_element_is_pending(&rxBuffersNew[bufferId]);
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