/**
 * @file esp32_uart.c
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-17
 *
 * @copyright Copyright (c) 2023
 *
 */

/* Personal Includes */
#include <string.h>

/* Personal Includes */
#include "esp32_uart.h"
#include "hardware_config.h"
#include "chars.h"

#define UART_NUM HC_UART_COMMS_UART_NUM

// support IDF 5.x
#ifndef portTICK_RATE_MS
    #define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

void esp32_uart_send_bpacket(bpk_packet_t* Bpacket) {

    bpk_buffer_t bpacketBuffer;
    bpacket_to_buffer(Bpacket, &bpacketBuffer);
    uart_write_bytes(UART_NUM, bpacketBuffer.buffer, bpacketBuffer.numBytes);
}

void esp32_uart_send_data(uint8_t* data, uint16_t numBytes) {
    uart_write_bytes(UART_NUM, data, numBytes);
}

void esp32_uart_send_string(char* string) {

    bpk_packet_t Bpacket;
    Bpacket.Request       = BPK_Request_Message;
    Bpacket.Code          = BPK_Code_In_Progress;
    Bpacket.Data.numBytes = BPACKET_MAX_NUM_DATA_BYTES;
    int pi                = 0;
    int numBytes          = chars_get_num_bytes(string);

    for (uint32_t i = 0; i < numBytes; i++) {

        Bpacket.Data.bytes[pi++] = string[i];

        if (pi < BPACKET_MAX_NUM_DATA_BYTES && (i + 1) != numBytes) {
            continue;
        }

        if ((i + 1) == numBytes) {
            Bpacket.Code          = BPK_Code_Success;
            Bpacket.Data.numBytes = pi--;
        }

        esp32_uart_send_bpacket(&Bpacket);
        pi = 0;
    }
}

int esp32_uart_read_bpacket(uint8_t bpacketBuffer[BPACKET_BUFFER_LENGTH_BYTES]) {
    return uart_read_bytes(UART_NUM, bpacketBuffer, BPACKET_BUFFER_LENGTH_BYTES, 50 / portTICK_RATE_MS);
}