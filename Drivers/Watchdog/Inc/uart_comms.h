#ifndef UART_COMMS_H
#define UART_COMMS_H

/* Public Marcos for common actions */
#define UART_DELIMETER      '_'
#define UART_DATA_DELIMETER ','

// Starting at 2 so no code is 0 or 1 because these codes
// are reserved for WD_ERROR and WD_SUCCESS
#define START                                 2
#define UART_ERROR_INVALID_REQUEST            (START + 1)
#define UART_ERROR_INVALID_INSTRUCTION        (START + 2)
#define UART_ERROR_INVALID_DATA               (START + 3)
#define UART_ERROR_REQUEST_FAILED             (START + 4)
#define UART_ERROR_REQUEST_UNKNOWN            (START + 6)
#define UART_ERROR_REQUEST_TRANSLATION_FAILED (START + 7)
#define UART_REQUEST_SUCCEEDED                (START + 5)
#define UART_REQUEST_LED_ON                   (START + 8)
#define UART_REQUEST_LED_OFF                  (START + 9)
#define UART_REQUEST_DATA_READ                (START + 10)

#define PACKET_COMMAND_NUM_CHARS     10
#define PACKET_INSTRUCTION_NUM_CHARS 100
#define PACKET_DATA_NUM_CHARS        100

static const int RX_BUF_SIZE = (PACKET_COMMAND_NUM_CHARS + PACKET_DATA_NUM_CHARS + PACKET_INSTRUCTION_NUM_CHARS + 5);

typedef struct packet_t {
    int request;
    char instruction[PACKET_INSTRUCTION_NUM_CHARS];
    char data[PACKET_DATA_NUM_CHARS];
} packet_t;

int packet_to_string(packet_t* packet, char data[RX_BUF_SIZE]);

int string_to_packet(packet_t* packet, char data[RX_BUF_SIZE]);

#endif // UART_COMMS_H