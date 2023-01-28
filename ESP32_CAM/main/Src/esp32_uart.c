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

    bpacket_buffer_t packetBuffer;
    bpacket_to_buffer(bpacket, &packetBuffer);
    uart_write_bytes(UART_NUM, packetBuffer.buffer, packetBuffer.numBytes);
}

int esp32_uart_send_data(const char* data) {

    // Because this data is being sent to a master MCU, the NULL character
    // needs to be appended on if it is not to ensure the master MCU knows
    // when the end of the sent data is. Thus add 1 to the length to ensure
    // the null character is also sent
    const int len     = strlen(data) + 1;
    const int txBytes = uart_write_bytes(UART_NUM, data, len);

    return txBytes;
}

void esp32_uart_send_string(char* string) {

    bpacket_t bpacket;
    bpacket.request  = BPACKET_R_IN_PROGRESS;
    bpacket.numBytes = BPACKET_MAX_NUM_DATA_BYTES;
    int pi           = 0;
    int numBytes     = chars_get_num_bytes(string);

    for (uint32_t i = 0; i < numBytes; i++) {

        bpacket.bytes[pi++] = string[i];

        if (pi < BPACKET_MAX_NUM_DATA_BYTES && (i + 1) != numBytes) {
            continue;
        }

        if ((i + 1) == numBytes) {
            bpacket.request  = BPACKET_R_SUCCESS;
            bpacket.numBytes = pi--;
        }

        esp32_uart_send_bpacket(&bpacket);
        pi = 0;
    }
}

int esp32_uart_read_bpacket(uint8_t bpacketBuffer[BPACKET_BUFFER_LENGTH_BYTES]) {
    return uart_read_bytes(UART_NUM, bpacketBuffer, BPACKET_BUFFER_LENGTH_BYTES, 50 / portTICK_RATE_MS);
}