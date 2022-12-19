#ifndef UART_COMMS_H
#define UART_COMMS_H

/* Public Marcos for common actions */
#define UC_DELIMETER             '_'
#define UC_COMMAND_NONE          0
#define UC_COMMAND_CAPTURE_IMAGE 1
#define UC_COMMAND_BLINK_LED     2

void uart_comms_transmit_message(int commandCode, char* data, char* formattedDateTime);

void uart_comms_receive_command(int* action, char* message);

#endif // UART_COMMS_H