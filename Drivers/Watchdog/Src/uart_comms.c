/**
 * @file uart_comms.c
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2022-12-19
 *
 * @copyright Copyright (c) 2022
 *
 */

/* Public Includes */
#include <driver/uart.h>
#include <string.h>

/* Private Includes */
#include "uart_comms.h"
#include "hardware_config.h"
#include "wd_utils.h"

/* Private Macros */
#define UART_NUM HC_UART_COMMS_UART_NUM

/* Private Variable Declarations */

/* Function Declarations */
int uart_comms_read(char* data, int timeout);

void uart_comms_transmit_message(int commandCode, char* data, char* formattedDateTime) {

    int attemptsLeft = 3;
    char msg[100];

    // while (1) {

    // Create message to transmit over UART
    sprintf(msg, "%d%c%s%c%s\r\n", commandCode, UC_DELIMETER, data, UC_DELIMETER, formattedDateTime);
    const int txBytes = uart_write_bytes(UART_NUM, msg, strlen(msg));

    // Wait for a response from slave to ensure the message was recieved correctly
    // char response[50];
    // if ((uart_comms_read(response, 5000) > 0) || (attemptsLeft == 0)) {
    //     break;
    // }

    // attemptsLeft -= 1;
    // }
}

void uart_comms_receive_command(int* action, char* message) {

    char data[100];
    uart_comms_read(data, 200);

    // Extract the command
    int command = wd_utils_extract_number(message, 0, '_');

    // Ensure the command is valid
    switch (command) {
        case UC_COMMAND_BLINK_LED:
        case UC_COMMAND_CAPTURE_IMAGE:
            break;
        default:
            command  = UC_COMMAND_NONE;
            *message = '\0';
            return;
    }

    *action = command;

    // Copy the message from the data string
    int i = 0;
    char c;
    while (data[i] != '\0') {
        message[i] = data[i];
        i++;
    }

    message[i] = '\0';
}

int uart_comms_read(char* data, int timeout) {

    const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, timeout / portTICK_RATE_MS);

    if (rxBytes > 0) {
        data[rxBytes] = 0;
        return rxBytes;
    }

    return rxBytes;
}