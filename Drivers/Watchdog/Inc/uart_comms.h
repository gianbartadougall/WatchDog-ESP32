// /**
//  * @file uart_comms.h
//  * @author Gian Barta-Dougall
//  * @brief
//  * @version 0.1
//  * @date 2023-01-14
//  *
//  * @copyright Copyright (c) 2023
//  *
//  */
// #ifndef UART_COMMS_H
// #define UART_COMMS_H

// /* Public Includes */
// #include <stdint.h>

// /* Public Marcos for common actions */
// #define UART_DELIMETER      '_'
// #define UART_DATA_DELIMETER ','

// // Starting at 2 so no code is 0 or 1 because these codes
// // are reserved for WD_ERROR and WD_SUCCESS
// #define START                                 2
// #define UART_ERROR_INVALID_REQUEST            (START + 1)
// #define UART_ERROR_INVALID_INSTRUCTION        (START + 2)
// #define UART_ERROR_INVALID_DATA               (START + 3)
// #define UART_ERROR_REQUEST_FAILED             (START + 4)
// #define UART_REQUEST_SUCCEEDED                (START + 5)
// #define UART_ERROR_REQUEST_UNKNOWN            (START + 6)
// #define UART_ERROR_REQUEST_TRANSLATION_FAILED (START + 7)
// #define UART_ERROR_INIT_ERROR                 (START + 8)
// #define UART_REQUEST_LED_RED_ON               (START + 9)
// #define UART_REQUEST_LED_RED_OFF              (START + 10)
// #define UART_REQUEST_LED_COB_ON               (START + 11)
// #define UART_REQUEST_LED_COB_OFF              (START + 12)
// #define UART_REQUEST_LIST_DIRECTORY           (START + 13)
// #define UART_REQUEST_COPY_FILE                (START + 14)
// #define UART_REQUEST_TAKE_PHOTO               (START + 15)
// #define UART_REQUEST_WRITE_TO_FILE            (START + 16)
// #define UART_REQUEST_IN_PROCESS               (START + 17)
// #define UART_REQUEST_CREATE_PATH              (START + 18)
// #define UART_REQUEST_STATUS                   (START + 19)
// #define UART_REQUEST_PING                     (START + 20)
// #define UART_REQUEST_RECORD_DATA              (START + 21)

// #define PACKET_COMMAND_NUM_CHARS     10
// #define PACKET_INSTRUCTION_NUM_CHARS 100
// #define PACKET_DATA_NUM_CHARS        100

// static const int RX_BUF_SIZE = (PACKET_COMMAND_NUM_CHARS + PACKET_DATA_NUM_CHARS + PACKET_INSTRUCTION_NUM_CHARS + 5);

// typedef struct packet_t {
//     int request;
//     char instruction[PACKET_INSTRUCTION_NUM_CHARS];
//     char data[PACKET_DATA_NUM_CHARS];
// } packet_t;

// int packet_to_string(packet_t* packet, char data[RX_BUF_SIZE]);

// int string_to_packet(packet_t* packet, char data[RX_BUF_SIZE]);

// void uart_comms_create_packet(packet_t* packet, uint8_t request, char* instruction, char* data);

// #endif // UART_COMMS_H