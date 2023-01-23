/**
 * @file main.c
 * @author Gian Barta-Dougall
 * @brief
 *
 * @note #define LIBSERIALPORT_MSBUILD must be specified in libserialport_internal.h
 * file for this to compile. There would be a way to compile everything doing this
 * differently but I have not found that out yet
 *
 * @version 0.1
 * @date 2023-01-16
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <libserialport.h>
#include <stdio.h>

/* C Library Includes */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>   // Use for multi threading
#include <sys/timeb.h> // Used for time functionality

/* Personal Includes */
#include "chars.h"
#include "uart_lib.h"
#include "watchdog_defines.h"
#include "bpacket.h"

#define MAPLE_MAX_ARGS 5

/* Example of how to get a list of serial ports on the system.
 *
 * This example file is released to the public domain. */

int check(enum sp_return result);
void maple_print_bpacket_data(bpacket_t* bpacket);
const char* parity_name(enum sp_parity parity);
void maple_create_and_send_bpacket(struct sp_port* port, uint8_t request, uint8_t numDataBytes,
                                   uint8_t data[BPACKET_MAX_NUM_DATA_BYTES]);
void maple_send_bpacket(struct sp_port* port, bpacket_t* bpacket);

#define RX_BUFFER_SIZE (BPACKET_BUFFER_LENGTH_BYTES * 50)
uint8_t rxBuffer[RX_BUFFER_SIZE];
// uint8_t rxBuffer[BPACKET_BUFFER_LENGTH_BYTES * 12];
// uint16_t bufferIndex = 0;

#define PACKET_BUFFER_SIZE 50
uint8_t packetBufferIndex  = 0;
uint8_t packetPendingIndex = 0;
bpacket_t packetBuffer[PACKET_BUFFER_SIZE];

void maple_increment_packet_buffer_index(void) {
    packetBufferIndex++;

    if (packetBufferIndex == PACKET_BUFFER_SIZE) {
        packetBufferIndex = 0;
    }
}

void maple_increment_packet_pending_index(void) {
    packetPendingIndex++;

    if (packetPendingIndex == PACKET_BUFFER_SIZE) {
        packetPendingIndex = 0;
    }
}

uint8_t maple_configure_port(struct sp_port* port) {

    if (port == NULL) {
        return 0;
    }

    check(sp_set_baudrate(port, 115200));
    check(sp_set_bits(port, 8));
    check(sp_set_parity(port, SP_PARITY_NONE));
    check(sp_set_stopbits(port, 1));
    check(sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));

    return 1;
}

enum sp_return maple_open_port(struct sp_port* port) {

    // Open the port
    enum sp_return result = sp_open(port, SP_MODE_READ_WRITE);
    if (result != SP_OK) {
        return result;
    }

    // Configure the port settings for communication
    check(sp_set_baudrate(port, 115200));
    check(sp_set_bits(port, 8));
    check(sp_set_parity(port, SP_PARITY_NONE));
    check(sp_set_stopbits(port, 1));
    check(sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));

    return SP_OK;
}

void maple_create_and_send_bpacket(struct sp_port* port, uint8_t request, uint8_t numDataBytes,
                                   uint8_t data[BPACKET_MAX_NUM_DATA_BYTES]) {
    bpacket_t bpacket;
    bpacket_create_p(&bpacket, request, numDataBytes, data);
    maple_send_bpacket(port, &bpacket);
}

void maple_send_bpacket(struct sp_port* port, bpacket_t* bpacket) {

    bpacket_buffer_t packetBuffer;
    bpacket_to_buffer(bpacket, &packetBuffer);

    if (sp_blocking_write(port, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
        printf("Failed to send bpacket\n");
    }
}

enum sp_return maple_search_ports(char portName[50]) {
    portName[0] = '\0';

    // Create a struct to hold all the COM ports currently in use
    struct sp_port** port_list;
    enum sp_return result = sp_list_ports(&port_list);

    if (result != SP_OK) {
        return result;
    }

    // Iterate through every port. Ping the port and check if
    // the response matches the given target
    bpacket_t bpacket;
    for (int i = 0; port_list[i] != NULL; i++) {

        struct sp_port* port = port_list[i];

        if (maple_open_port(port) != SP_OK) {
            continue;
        }

        // Ping port
        maple_create_and_send_bpacket(port, BPACKET_GEN_R_PING, 0, NULL);

        // Wait for response from port
        uint8_t response[BPACKET_BUFFER_LENGTH_BYTES];
        response[0] = '\0';
        if (sp_blocking_read(port, response, 6, 1100) < 0) {
            sp_close(port);
            continue;
        }

        bpacket_decode(&bpacket, response);

        if (bpacket.request != BPACKET_R_SUCCESS || bpacket.bytes[0] != 23) {
            sp_close(port);

            continue;
        }

        sprintf(portName, "%s", sp_get_port_name(port));
        sp_close(port);
        break;
    }

    sp_free_port_list(port_list);

    return SP_OK;
}

uint8_t maple_copy_file(struct sp_port* port, char* filePath, char* cpyFileName) {

    FILE* target;
    target = fopen(cpyFileName, "wb"); // Read binary

    if (target == NULL) {
        printf("Could not open file\n");
        fclose(target);
        return FALSE;
    }

    // Confirm file path <= max number of bytes that can be sent
    int filePathNumBytes = chars_get_num_bytes(filePath);
    if (filePathNumBytes > BPACKET_MAX_NUM_DATA_BYTES) {
        printf("File path is too long\n");
        fclose(target);
        return FALSE;
    }

    // Convert file path to binary data
    uint8_t bpacketBuffer[filePathNumBytes];
    for (int i = 0; i < filePathNumBytes; i++) {
        bpacketBuffer[i] = (uint8_t)filePath[i];
    }

    // Send command to copy file. Keeping reading until no more data to send across
    maple_create_and_send_bpacket(port, WATCHDOG_BPK_R_COPY_FILE, filePathNumBytes, bpacketBuffer);

    int packetsFinished = FALSE;
    int count           = 0;
    while (packetsFinished == FALSE) {

        // Wait until the packet is ready
        count = 0;
        while (packetPendingIndex == packetBufferIndex) {

            if (count == 1000) {
                printf("Timeout 1000ms\n");
                return FALSE;
            }

            Sleep(1);
            count++;
        }

        // printf("Request[%i]: %i Num bytes: %i -> ", packetPendingIndex, packetBuffer[packetPendingIndex].request,
        //        packetBuffer[packetPendingIndex].numBytes);
        // Packet ready. Print its contents
        // for (int i = 0; i < packetBuffer[packetPendingIndex].numBytes; i++) {
        //     printf("%c", packetBuffer[packetPendingIndex].bytes[i]);
        // }
        // printf("\n");

        if (packetBuffer[packetPendingIndex].request != BPACKET_R_IN_PROGRESS &&
            packetBuffer[packetPendingIndex].request != BPACKET_R_SUCCESS) {
            printf("PACKET ERROR FOUND. Request %i\n", packetBuffer[packetPendingIndex].request);
            fclose(target);
            return FALSE;
        }

        if (packetBuffer[packetPendingIndex].request == BPACKET_R_SUCCESS) {
            packetsFinished = TRUE;
        }

        for (int i = 0; i < packetBuffer[packetPendingIndex].numBytes; i++) {
            fputc(packetBuffer[packetPendingIndex].bytes[i], target);
        }

        maple_increment_packet_pending_index();
    }

    fclose(target);

    return TRUE;
}

void maple_print_bpacket_data(bpacket_t* bpacket) {

    char data[bpacket->numBytes + 1];

    // Copy all the data across
    for (int i = 0; i < bpacket->numBytes; i++) {
        data[i] = bpacket->bytes[i];
    }

    data[bpacket->numBytes] = '\0';

    printf("%s\n", data);
}

void maple_list_directory(struct sp_port* port, char* filePath) {

    bpacket_t packet;
    bpacket_create_sp(&packet, WATCHDOG_BPK_R_LIST_DIR, filePath);
    maple_send_bpacket(port, &packet);

    int packetsFinished = FALSE;
    while (packetsFinished == FALSE) {

        // Wait until the packet is ready
        while (packetPendingIndex == packetBufferIndex) {}

        // Packet ready. Print its contents
        for (int i = 0; i < packetBuffer[packetPendingIndex].numBytes; i++) {
            printf("%c", packetBuffer[packetPendingIndex].bytes[i]);
        }

        if (packetBuffer[packetPendingIndex].request == BPACKET_R_SUCCESS) {
            packetsFinished = TRUE;
        }

        maple_increment_packet_pending_index();
    }
}

void maple_print_uart_response(void) {

    int packetsFinished = FALSE;
    while (packetsFinished == FALSE) {

        // Wait until the packet is ready
        while (packetPendingIndex == packetBufferIndex) {}

        // Packet ready. Print its contents
        for (int i = 0; i < packetBuffer[packetPendingIndex].numBytes; i++) {
            printf("%c", packetBuffer[packetPendingIndex].bytes[i]);
        }

        if (packetBuffer[packetPendingIndex].request == BPACKET_R_SUCCESS) {
            packetsFinished = TRUE;
        }

        maple_increment_packet_pending_index();
    }
}

uint8_t maple_match_args(char** args, int numArgs, struct sp_port* port) {

    if (numArgs == 1) {

        if (chars_same(args[0], "help\0") == TRUE) {
            maple_create_and_send_bpacket(port, BPACKET_GEN_R_HELP, 0, NULL);
            maple_print_uart_response();
            // sprintf(uartString, "%i,%s,%s", B, "data.txt", "21/02/2023 0900 26.625\r\n");
            return TRUE;
        }

        // if (chars_same(args[0], "rec\0") == TRUE) {
        //     sprintf(uartString, "%i,%s,%s", UART_REQUEST_RECORD_DATA, "data.txt", "21/02/2023 0900 26.625\r\n");
        //     return TRUE;
        // }

        if (chars_same(args[0], "clc\0")) {
            printf(ASCII_CLEAR_SCREEN);
            return TRUE;
        }

        if (chars_same(args[0], "ls\0") == TRUE) {
            maple_list_directory(port, "\0");
            return TRUE;
        }
    }

    if (numArgs == 2) {

        if (chars_same(args[0], "ls\0") == TRUE) {
            maple_list_directory(port, args[1]);
            return TRUE;
        }
    }

    if (numArgs == 3) {

        if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
            chars_same(args[2], "on\0") == TRUE) {
            maple_create_and_send_bpacket(port, WATCHDOG_BPK_R_LED_RED_ON, 0, NULL);
            return TRUE;
        }

        if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
            chars_same(args[2], "off\0") == TRUE) {
            maple_create_and_send_bpacket(port, WATCHDOG_BPK_R_LED_RED_OFF, 0, NULL);
            return TRUE;
        }

        if (chars_same(args[0], "cpy\0") == TRUE) {
            if (maple_copy_file(port, args[1], args[2]) == TRUE) {
                printf(ASCII_COLOR_GREEN "Succesfully transfered file\n" ASCII_COLOR_WHITE);
            } else {
                printf(ASCII_COLOR_RED "Failed to transfer file\n" ASCII_COLOR_WHITE);
            }
            return TRUE;
        }
    }

    return FALSE;
}

DWORD WINAPI maple_listen_rx(void* arg) {

    // Arg is the pointer to the port
    struct sp_port* port = arg;

    uint8_t c[1];
    uint8_t lastChar  = BPACKET_STOP_BYTE;
    int numBytes      = 0;
    packetBufferIndex = 0;

    int bufferIndex = 0;

    int packetLengthFlag  = FALSE;
    int packetCommandFlag = FALSE;
    int packetByteIndex   = 0;
    int startByteIndex    = 0;

    while ((numBytes = sp_blocking_read(port, c, 1, 0)) > 0) {

        if (bufferIndex == RX_BUFFER_SIZE) {
            bufferIndex = 0;
        }

        rxBuffer[bufferIndex++] = c[0];

        if (packetLengthFlag == TRUE) {
            // Subtract 1 because this number includes the request
            packetBuffer[packetBufferIndex].numBytes = (c[0] - 1);
            packetLengthFlag                         = FALSE;
            packetCommandFlag                        = TRUE;
            lastChar                                 = c[0];
            // printf("Packet length flag set\n");
            continue;
        }

        if (packetCommandFlag == TRUE) {
            packetBuffer[packetBufferIndex].request = c[0];
            packetCommandFlag                       = FALSE;
            lastChar                                = c[0];
            // printf("Packet command flag set\n");
            continue;
        }

        if (c[0] == BPACKET_START_BYTE) {

            // Need to determine whether the character recieved is a start byte or part
            // of a message. We can be very confident the character represents a start
            // byte if:
            // 1) The character that was recieved previously was a stop byte, and
            // 2) The start byte agrees with the length of the last message that
            //      was recieved

            if (lastChar == BPACKET_STOP_BYTE) {

                // Check whether the length of the message agrees with the position
                // of this recieved start byte
                // if (bufferIndex == (stopByteIndex + 1)) {
                //     // Very certain that this byte is the start of a new packet
                // }

                // Start of new packet
                packetLengthFlag = TRUE;
                startByteIndex   = bufferIndex - 1; // Minus 1 because one is added at the top
                lastChar         = c[0];

                // printf("Start byte recieved\n");
                // printf("Start byte acted upon\n");
                continue;
            }

            //
        }

        if (c[0] == BPACKET_STOP_BYTE) {
            // printf("Buffer index: %i Start index: %i\n", bufferIndex, startByteIndex);

            // Calculate the
            int stopByteIndex = (bufferIndex == 0 ? RX_BUFFER_SIZE : bufferIndex) - 1;

            // The added 3 is for the start byte and the second byte in the bpacket that holds
            // the number of data bytes the packet should contain and the request byte
            int predictedStopByteIndex =
                (startByteIndex + 3 + packetBuffer[packetBufferIndex].numBytes) % RX_BUFFER_SIZE;

            // Need to determine whether the character recieved was a stop byte or
            // part of the message. If the stop byte is in the predicted spot, then
            // we can be farily certain that this byte is a stop byte
            if (stopByteIndex == predictedStopByteIndex) {

                // Very sure this is the end of a packet
                // printf("Request[%i]: %i Numbytes: %i-> ", packetBufferIndex, packetBuffer[packetBufferIndex].request,
                //        packetBuffer[packetBufferIndex].numBytes);
                // // Packet ready. Print its contents
                // for (int i = 0; i < packetBuffer[packetBufferIndex].numBytes; i++) {
                //     printf("%c", packetBuffer[packetBufferIndex].bytes[i]);
                // }
                // printf("\n");

                // Reset byte index to 0
                packetByteIndex = 0;
                lastChar        = c[0];

                // printf("Stop byte recieved\n");
                maple_increment_packet_buffer_index();
                continue;
            }
        }

        // Byte is a data byte, add to the bpacket
        packetBuffer[packetBufferIndex].bytes[packetByteIndex++] = c[0];
    }

    if (numBytes < 0) {
        printf("Error reading COM port\n");
    }

    return FALSE;
}

int main(int argc, char** argv) {

    char espPortName[50];
    espPortName[0]        = '\0';
    enum sp_return result = maple_search_ports(espPortName);
    if (result != SP_OK) {
        check(result);
        return 0;
    }

    if (espPortName[0] == '\0') {
        printf("ESP port could not be found\n");
        return 0;
    }

    // Open ESP port
    struct sp_port* port;
    if (check(sp_get_port_by_name(espPortName, &port)) != SP_OK) {
        printf("Could not find ESP port\n");
        return 0;
    }

    if (check(maple_open_port(port)) != SP_OK) {
        printf("Could not open ESP port\n");
        return 0;
    }

    HANDLE thread = CreateThread(NULL, 0, maple_listen_rx, port, 0, NULL);

    if (!thread) {
        printf("Thread failed\n");
        return 0;
    }

    printf("ESP32 connected on port %s\n", espPortName);

    char userInput[100];
    while (1) {

        // Get input from user
        gets(userInput);

        // Split the string by spaces
        char* ptr = strtok(userInput, " ");
        char* args[5];
        int numArgs;
        for (numArgs = 0; ptr != NULL; numArgs++) {

            args[numArgs] = malloc(sizeof(char) * strlen(ptr));
            strcpy(args[numArgs], ptr);

            ptr = strtok(NULL, " ");

            if (numArgs == MAPLE_MAX_ARGS) {
                break;
            }
        }

        if (numArgs == 0) {
            continue;
        }

        if (maple_match_args(args, numArgs, port) == FALSE) {
            printf(ASCII_COLOR_RED "Unkown command: '%s'\n" ASCII_COLOR_WHITE, userInput);
        }

        // Free list of args
        for (int i = 0; i < numArgs; i++) {
            free(args[i]);
        }
    }
}

// /* A pointer to a null-terminated array of pointers to
//  * struct sp_port, which will contain the ports found.*/
// struct sp_port** port_list;
// char espPortName[50];
// espPortName[0] = '\0';

// printf("Getting port list.\n");

// /* Call sp_list_ports() to get the ports. The port_list
//  * pointer will be updated to refer to the array created. */
// enum sp_return result = sp_list_ports(&port_list);

// if (result != SP_OK) {
//     printf("sp_list_ports() failed!\n");
//     return -1;
// }

// /* Iterate through the ports. When port_list[i] is NULL
//  * this indicates the end of the list. */
// int i;
// for (i = 0; port_list[i] != NULL; i++) {
//     struct sp_port* port = port_list[i];

//     /* Get the name of the port. */
//     char* port_name = sp_get_port_name(port);

//     if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK) {
//         printf("Could not open %s\n", port_name);
//         continue;
//     }

//     maple_configure_port(port);

//     // To work out correct port, send ping to port and wait for response
//     char ping[50];
//     sprintf(ping, "%i,h,h", UART_REQUEST_STATUS);
//     int result = sp_blocking_write(port, ping, strlen(ping) + 1, 100);
//     if (result < 0) {
//         printf("Could not write bytes to %s\n", port_name);
//     }

//     // Wait for response
//     char response[100];
//     response[0] = '\0';
//     result      = sp_blocking_read(port, response, 100, 1100);
//     if (result < 0) {
//         printf("Error reading from %s\n", port_name);
//     } else {

//         if (response[0] == '\0') {
//             printf("No Response from %s\n", port_name);
//         } else {
//             sprintf(espPortName, "%s", port_name);
//             printf("%s\n", response);
//         }
//     }

//     if (sp_close(port) != SP_OK) {
//         printf("Could not close %s\n", port_name);
//     }
// }

// Connect to the port that has the ESP32 connected to it

// printf("Found %d ports.\n", i);

// Check ESP32 port still available
// if (espPortName[0] != '\0') {
//     struct sp_port* port;

//     if (sp_get_port_by_name(espPortName, &port) != SP_OK) {
//         sprintf("Failed to Open ESP port: %s\n", espPortName);
//     } else {
//         printf("ESP Port found: %s\n", sp_get_port_name(port));
//     }

// } else {
//     printf("ESP port null\n");
// }

// printf("Freeing port list.\n");
/* Free the array created by sp_list_ports(). */
// sp_free_port_list(port_list);

/* Note that this will also free all the sp_port structures
 * it points to. If you want to keep one of them (e.g. to
 * use that port in the rest of your program), take a copy
 * of it first using sp_copy_port(). */

// return 0;
// }

/* Helper function for error handling. */
int check(enum sp_return result) {
    /* For this example we'll just exit on any error by calling abort(). */
    char* error_message;

    switch (result) {
        case SP_ERR_ARG:
            printf("Error: Invalid argument.\n");
            abort();
        case SP_ERR_FAIL:
            error_message = sp_last_error_message();
            printf("Error: Failed: %s\n", error_message);
            sp_free_error_message(error_message);
            abort();
        case SP_ERR_SUPP:
            printf("Error: Not supported.\n");
            abort();
        case SP_ERR_MEM:
            printf("Error: Couldn't allocate memory.\n");
            abort();
        case SP_OK:
        default:
            return result;
    }
}

/* Helper function to give a name for each parity mode. */
const char* parity_name(enum sp_parity parity) {
    switch (parity) {
        case SP_PARITY_INVALID:
            return "(Invalid)";
        case SP_PARITY_NONE:
            return "None";
        case SP_PARITY_ODD:
            return "Odd";
        case SP_PARITY_EVEN:
            return "Even";
        case SP_PARITY_MARK:
            return "Mark";
        case SP_PARITY_SPACE:
            return "Space";
        default:
            return NULL;
    }
}
