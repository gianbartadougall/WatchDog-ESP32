// /**
//  * @file uart_comms.c
//  * @author Gian Barta-Dougall
//  * @brief
//  * @version 0.1
//  * @date 2022-12-19
//  *
//  * @copyright Copyright (c) 2022
//  *
//  */

// /* Public Includes */
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// /* Private Includes */
// #include "hardware_config.h"
// #include "uart_comms.h"
// #include "wd_utils.h"
// #include "chars.h"

// /* Private Macros */
// #define UART_NUM HC_UART_COMMS_UART_NUM

// /* Private Variable Declarations */

// /* Function Declarations */
// int uart_comms_read(char* data, int timeout);

// void uart_comms_create_packet(packet_t* packet, bpk_request_t Request, char* instruction, char* data) {
//     packet->request = request;
//     sprintf(packet->instruction, "%s", instruction);
//     sprintf(packet->data, "%s", data);
// }

// int packet_to_string(packet_t* packet, char data[RX_BUF_SIZE]) {
//     sprintf(data, "%i,%s,%s", packet->request, packet->instruction, packet->data);

//     return WD_SUCCESS;
// }

// // Packet is of the form request,instruction,data
// int string_to_packet(packet_t* packet, char data[RX_BUF_SIZE]) {

//     char info[3][RX_BUF_SIZE];
//     wd_utils_split_string(data, info, 0, ',');

//     // Validate the command
//     int request;
//     if (wd_utils_extract_number(data, &request, 0, ',') != WD_SUCCESS) {
//         return UART_ERROR_INVALID_REQUEST;
//     }

//     // Validate request
//     // switch (request) {
//     //     case UART_REQUEST_LED_ON:
//     //     case UART_REQUEST_LED_OFF:
//     //     case UART_REQUEST_DATA_READ:
//     //         break;
//     //     default:
//     //         return UART_ERROR_INVALID_REQUEST;
//     // }

//     packet->request = request;
//     sprintf(packet->instruction, info[1]);
//     sprintf(packet->data, info[2]);

//     return WD_SUCCESS;
// }

// // int uart_comms_transmit_message(int commandCode, char* instruction, char* message) {

// //     // Create message to transmit over UART
// //     char msg[100];
// //     sprintf(msg, "%d%c%s%c%s\r\n", commandCode, UC_DELIMETER, instruction, UC_DELIMETER,
// //     message); const int txBytes = uart_write_bytes(UART_NUM, msg, strlen(msg));

// //     return txBytes;
// // }

// // void uart_comms_receive_command(int* action, char instruction[100], char message[100]) {

// //     char data[200];
// //     uart_comms_read(data, 200);

// //     // Extract data from message read
// //     char list[3][100];
// //     wd_utils_split_string(data, list, 0, UC_DELIMETER);

// //     // Extract the command
// //     int command = wd_utils_extract_number(list[0], 0, UC_DELIMETER);

// //     // Ensure the command is valid
// //     switch (command) {
// //         case 1:
// //         case 2:
// //             break;
// //         default:
// //             *action        = 0;
// //             instruction[0] = '\0';
// //             message[0]     = '\0';
// //             return;
// //     }

// //     *action = command;

// //     int i;
// //     for (i = 0; list[1][i] != '\0'; i++) {
// //         instruction[i] = list[1][i];
// //     }

// //     instruction[i] = '\0';

// //     for (i = 0; list[2][i] != '\0'; i++) {
// //         message[i] = list[2][i];
// //     }

// //     message[i] = '\0';
// // }

// // int uart_comms_read(char* data, int timeout) {

// //     const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, timeout / portTICK_RATE_MS);

// //     if (rxBytes > 0) {
// //         data[rxBytes] = 0;
// //         return rxBytes;
// //     }

// //     return rxBytes;
// // }