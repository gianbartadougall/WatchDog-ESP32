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
uint8_t divertedBytesAddress[NUM_BUFFERS];

cbuffer_t rxBuffersNew[2];
uint8_t rxBufNew[2][RX_BUFFER_SIZE] = {{0}, {0}};

cbuffer_t ByteBuffer[2];
uint8_t byteBuffer[8];

bpk_t ErrorPacket;
bpk_buffer_t ErrorBuffer;
char errMsg[50];

/* Function Prototyes */
void uart_send_byte(uint8_t bufferId, uint8_t byte);

void uart_init(void) {

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

void uart_append_to_buffer(uint8_t bufferId, uint8_t byte) {
    cbuffer_append_element(&rxBuffersNew[bufferId], (void*)(&byte));
}

uint8_t uart_process_rxbuffer(uint8_t bufferId, bpk_t* Bpacket) {

    uint8_t byte, expectedByte;

    while (cbuffer_read_next_element(&rxBuffersNew[bufferId], (void*)(&byte)) == TRUE) {

        // Read the current index of the byte buffer and store the result into
        // expected byte
        cbuffer_read_current_byte(&ByteBuffer[bufferId], (void*)(&expectedByte));

        switch (expectedByte) {

            /* Expecting bpacket 'data' bytes */
            case BPK_BYTE_DATA:

                if (divertBytes[bufferId] == TRUE) {
                    uart_send_byte(divertedBytesAddress[bufferId], byte);
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

                // If the bytes were diverted, nothing for the STM32 to do so return FALSE. If the
                // bytes were not diverted then this was a message for the STM32 so return TRUE
                if (divertBytes[bufferId] == TRUE) {
                    return FALSE;
                } else {
                    return TRUE;
                }

            /* Expecting bpacket 'length' byte */
            case BPK_BYTE_LENGTH:

                // Store the length and updated the expected byte
                Bpacket->Data.numBytes = byte;
                cbuffer_increment_read_index(&ByteBuffer[bufferId]);

                if (Bpacket->Receiver.val != BPK_ADDRESS_STM32) {

                    divertBytes[bufferId] = TRUE;
                    //  uint8_t bufId;
                    if (Bpacket->Receiver.val == BPK_ADDRESS_ESP32) {
                        divertedBytesAddress[bufferId] = BUFFER_1_ID;
                        // sprintf(errMsg, "Diverted to ESP32: %i %i %i %i %i", Bpacket->Receiver.val,
                        // Bpacket->Sender.val,
                        //         Bpacket->Code.val, Bpacket->Request.val, Bpacket->Data.numBytes);
                        // bpk_create_sp(&ErrorPacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32,
                        //                   BPK_Request_Message, BPK_Code_Debug, errMsg);
                        // bpk_to_buffer(&ErrorPacket, &ErrorBuffer);
                        // uart_transmit_data(USART2, ErrorBuffer.buffer, ErrorBuffer.numBytes);
                    } else {
                        divertedBytesAddress[bufferId] = BUFFER_2_ID;
                        // bpk_create_sp(&ErrorPacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32,
                        //                   BPK_Request_Message, BPK_Code_Debug, "Diverted to Maple");
                        // bpk_to_buffer(&ErrorPacket, &ErrorBuffer);
                        // uart_transmit_data(USART2, ErrorBuffer.buffer, ErrorBuffer.numBytes);
                    }

                    uart_send_byte(divertedBytesAddress[bufferId], BPK_BYTE_START_BYTE_UPPER);
                    uart_send_byte(divertedBytesAddress[bufferId], BPK_BYTE_START_BYTE_LOWER);
                    uart_send_byte(divertedBytesAddress[bufferId], Bpacket->Receiver.val);
                    uart_send_byte(divertedBytesAddress[bufferId], Bpacket->Sender.val);
                    uart_send_byte(divertedBytesAddress[bufferId], Bpacket->Request.val);
                    uart_send_byte(divertedBytesAddress[bufferId], Bpacket->Code.val);
                    uart_send_byte(divertedBytesAddress[bufferId], Bpacket->Data.numBytes);
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

                // Updated the expected byte
                cbuffer_increment_read_index(&ByteBuffer[bufferId]);

                // If decoding the byte succeeded, move to the next expected byte by
                // incrementing the circrular buffer holding the expected bytes
                if (bpk_utils_decode_non_data_byte(Bpacket, expectedByte, byte) == TRUE) {
                    continue;
                }

                sprintf(errMsg, "Dec. Exp %i. Fnd %i (%c)", expectedByte, byte, byte);
                bpk_create_sp(&ErrorPacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message,
                              BPK_Code_Error, errMsg);
                bpk_to_buffer(&ErrorPacket, &ErrorBuffer);
                uart_transmit_data(USART2, ErrorBuffer.buffer, ErrorBuffer.numBytes);
        }

        // Decoding failed, print error.
        sprintf(errMsg, "Expected %i. Found %i (%c)", expectedByte, byte, byte);
        bpk_create_sp(&ErrorPacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message, BPK_Code_Error,
                      errMsg);

        bpk_to_buffer(&ErrorPacket, &ErrorBuffer);
        uart_transmit_data(USART2, ErrorBuffer.buffer, ErrorBuffer.numBytes);

        // Reset the expected byte back to the start and bpacket buffer index
        cbuffer_reset_read_index(&ByteBuffer[bufferId]);
        bpacketByteIndex[bufferId] = 0;
    }

    return FALSE;
}

uint8_t uart_buffer_not_empty(uint8_t bufferId) {
    return cbuffer_is_empty(&rxBuffersNew[bufferId]);
}

/* Generic USART Commuincation Functions */

void uart_send_byte(uint8_t bufferId, uint8_t byte) {
    // Wait for USART to be ready to send a byte
    while ((uarts[bufferId]->ISR & USART_ISR_TXE) == 0) {};

    // Send byte of usart
    uarts[bufferId]->TDR = byte;
}

void uart_write(uint8_t bufferId, uint8_t* data, uint16_t numBytes) {

    for (int i = 0; i < numBytes; i++) {

        // Wait for USART to be ready to send a byte
        while ((uarts[bufferId]->ISR & USART_ISR_TXE) == 0) {};

        // Send byte of usart
        uarts[bufferId]->TDR = data[i];
    }
}

void uart_open_connection(uint8_t bufferId) {

    if (bufferId == BUFFER_1_ID) {
        BUFFER_1->CR1 |= (USART_CR1_RE | USART_CR1_TE);
    }

    if (bufferId == BUFFER_2_ID) {
        BUFFER_2->CR1 |= (USART_CR1_RE | USART_CR1_TE);
    }
}

void comms_close_connection(uint8_t bufferId) {

    if (bufferId == BUFFER_1_ID) {
        BUFFER_1->CR1 &= ~(USART_CR1_RE | USART_CR1_TE);
    }

    if (bufferId == BUFFER_2_ID) {
        BUFFER_2->CR1 &= ~(USART_CR1_RE | USART_CR1_TE);
    }
}