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

void esp32_uart_send_bpacket(bpacket_t* bpacket) {

    bpacket_buffer_t bpacketBuffer;
    bpacket_to_buffer(bpacket, &bpacketBuffer);
    uart_write_bytes(UART_NUM, bpacketBuffer.buffer, bpacketBuffer.numBytes);

    // bpacket_t res;
    // char msg[20];
    // sprintf(msg, "[bytes: %i]", bpacketBuffer.numBytes);
    // bpacket_create_sp(&res, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32, 0, BPACKET_CODE_SUCCESS, msg);
    // bpacket_to_buffer(&res, &bp1);
    // uart_write_bytes(UART_NUM, bp1.buffer, bp1.numBytes);
}

void esp32_uart_send_data(uint8_t* data, uint16_t numBytes) {
    uart_write_bytes(UART_NUM, data, numBytes);
}

void esp32_uart_send_string(char* string) {

    bpacket_t bpacket;
    bpacket.request  = BPACKET_CODE_IN_PROGRESS;
    bpacket.numBytes = BPACKET_MAX_NUM_DATA_BYTES;
    int pi           = 0;
    int numBytes     = chars_get_num_bytes(string);

    for (uint32_t i = 0; i < numBytes; i++) {

        bpacket.bytes[pi++] = string[i];

        if (pi < BPACKET_MAX_NUM_DATA_BYTES && (i + 1) != numBytes) {
            continue;
        }

        if ((i + 1) == numBytes) {
            bpacket.request  = BPACKET_CODE_SUCCESS;
            bpacket.numBytes = pi--;
        }

        esp32_uart_send_bpacket(&bpacket);
        pi = 0;
    }
}

int esp32_uart_read_bpacket(uint8_t bpacketBuffer[BPACKET_BUFFER_LENGTH_BYTES]) {
    return uart_read_bytes(UART_NUM, bpacketBuffer, BPACKET_BUFFER_LENGTH_BYTES, 50 / portTICK_RATE_MS);
}