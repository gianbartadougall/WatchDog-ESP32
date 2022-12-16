
/* Public Includes */
#include <driver/uart.h>

/* Private Includes */
#include "uart_comms.h"
#include "hardware_config.h"
#include "wd_utils.h"

#define MASTER 0
#define SLAVE  1
#define TYPE   SLAVE

#define UART_NUM HC_UART_COMMS_UART_NUM

static const int RX_BUF_SIZE = 1024;

#if (TYPE == MASTER)
void uart_comms_transmit_message(int commandCode, char* message) {

    int attemptsLeft = 3;
    char msg[100];
    while (1) {

        // Create message to transmit over UART
        sprintf(msg, "%d_%s", commandCode, message);
        const int txBytes = uart_write_bytes(UART_NUM, msg, strlen(msg));

        // Wait for a response from slave to ensure the message was recieved correctly
        char response[50];
        if ((uart_comms_read(response, 5000) > 0) || (attemptsLeft == 0)) {
            break;
        }

        attemptsLeft -= 1;
    }
}
#elif (TYPE == SLAVE)

void uart_comms_receive_command(int* action, char* message) {

    char data[100];
    uart_comms_read(data, 5000);

    // Extract the command
    int command = wd_utils_extract_number(message, 0, '_');
    if (command == 0) {
        action = UC_ACTION_NONE;
    } else {
        action = command;
    }

    // Copy the message from the data string
    int i = 0;
    char c;
    while (c = data[i]) {

        message[i] = c;

        if (c == '\0') {
            break;
        }
    }
}

#endif

void uart_comms_read(char* data, int timeout) {

    const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, timeout / portTICK_RATE_MS);

    if (rxBytes > 0) {
        data[rxBytes] = 0;
        return rxBytes;
    }

    return rxBytes;
}