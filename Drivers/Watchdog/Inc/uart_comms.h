#ifndef UART_COMMS_H
#define UART_COMMS_H

/* Public Marcos for common actions */
#define UART_DELIMETER '_'
#define UART_DATA_DELIMETER ','
#define UART_REQUEST_PARSE_ERROR 1
#define UART_REQUEST_ACKNOWLEDGED 2
#define UART_REQUEST_LED_ON 3
#define UART_REQUEST_LED_OFF 4

#define PACKET_COMMAND_NUM_CHARS 10
#define PACKET_INSTRUCTION_NUM_CHARS 100
#define PACKET_DATA_NUM_CHARS 100

static const int RX_BUF_SIZE =
    (PACKET_COMMAND_NUM_CHARS + PACKET_DATA_NUM_CHARS + PACKET_INSTRUCTION_NUM_CHARS + 5);

typedef struct packet_t {
    int request;
    char instruction[PACKET_INSTRUCTION_NUM_CHARS];
    char data[PACKET_DATA_NUM_CHARS];
} packet_t;

int packet_to_string(packet_t *packet, char data[RX_BUF_SIZE]);

int string_to_packet(packet_t *packet, char data[RX_BUF_SIZE]);

#endif // UART_COMMS_H