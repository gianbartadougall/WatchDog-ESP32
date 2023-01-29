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

/* C Library Includes */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>   // Use for multi threading
#include <sys/timeb.h> // Used for time functionality

/* C Library Includes for Display Images */
#include "stb_image.h"

/* Personal Includes */
#include "chars.h"
#include "uart_lib.h"
#include "watchdog_defines.h"
#include "bpacket.h"
#include "com_ports.h"
#include "gui.h"

#define MAPLE_MAX_ARGS 5

/* Example of how to get a list of serial ports on the system.
 *
 * This example file is released to the public domain. */

int check(enum sp_return result);
void maple_print_bpacket_data(bpacket_t* bpacket);
void maple_create_and_send_bpacket(uint8_t request, uint8_t numDataBytes, uint8_t data[BPACKET_MAX_NUM_DATA_BYTES]);
void maple_create_and_send_sbpacket(uint8_t request, char* string);
void maple_print_uart_response(void);

#define RX_BUFFER_SIZE (BPACKET_BUFFER_LENGTH_BYTES * 50)
uint8_t rxBuffer[RX_BUFFER_SIZE];

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

void maple_create_and_send_bpacket(uint8_t request, uint8_t numDataBytes, uint8_t data[BPACKET_MAX_NUM_DATA_BYTES]) {
    bpacket_t bpacket;
    bpacket_create_p(&bpacket, request, numDataBytes, data);
    if (com_ports_send_bpacket(&bpacket) != TRUE) {
        return;
    }
}

void maple_create_and_send_sbpacket(uint8_t request, char* string) {
    bpacket_t bpacket;
    bpacket_create_sp(&bpacket, request, string);
    if (com_ports_send_bpacket(&bpacket) != TRUE) {
        return;
    }
}

uint8_t maple_copy_file(char* filePath, char* cpyFileName) {

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
    maple_create_and_send_bpacket(WATCHDOG_BPK_R_COPY_FILE, filePathNumBytes, bpacketBuffer);

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

uint8_t maple_receive_camera_view(char* fileName) {

    FILE* target;
    target = fopen(fileName, "wb"); // Read binary

    if (target == NULL) {
        printf("Could not open file\n");
        fclose(target);
        return FALSE;
    }

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

uint8_t maple_match_args(char** args, int numArgs) {

    if (numArgs == 1) {

        if (chars_same(args[0], "help\0") == TRUE) {
            maple_create_and_send_bpacket(BPACKET_GEN_R_HELP, 0, NULL);
            maple_print_uart_response();
            return TRUE;
        }

        if (chars_same(args[0], "clc\0")) {
            printf(ASCII_CLEAR_SCREEN);
            return TRUE;
        }

        if (chars_same(args[0], "ls\0") == TRUE) {
            maple_create_and_send_sbpacket(WATCHDOG_BPK_R_LIST_DIR, "\0");
            maple_print_uart_response();
            return TRUE;
        }

        if (chars_same(args[0], "photo\0") == TRUE) {
            maple_create_and_send_bpacket(WATCHDOG_BPK_R_TAKE_PHOTO, 0, NULL);
            maple_print_uart_response();
            return TRUE;
        }
    }

    if (numArgs == 2) {

        if (chars_same(args[0], "ls\0") == TRUE) {
            maple_create_and_send_sbpacket(WATCHDOG_BPK_R_LIST_DIR, args[1]);
            maple_print_uart_response();
            return TRUE;
        }
    }

    if (numArgs == 3) {

        if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
            chars_same(args[2], "on\0") == TRUE) {
            maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_ON, 0, NULL);
            return TRUE;
        }

        if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
            chars_same(args[2], "off\0") == TRUE) {
            maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_OFF, 0, NULL);
            return TRUE;
        }

        if (chars_same(args[0], "cpy\0") == TRUE) {
            if (maple_copy_file(args[1], args[2]) == TRUE) {
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
    // struct sp_port* port = arg;

    uint8_t c[1];
    uint8_t lastChar  = BPACKET_STOP_BYTE;
    int numBytes      = 0;
    packetBufferIndex = 0;

    int bufferIndex = 0;

    int packetLengthFlag  = FALSE;
    int packetCommandFlag = FALSE;
    int packetByteIndex   = 0;
    int startByteIndex    = 0;

    // while ((numBytes = sp_blocking_read(port, c, 1, 0)) > 0) {
    while ((numBytes = com_ports_read(c, 1, 0)) > 0) {

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
            continue;
        }

        if (packetCommandFlag == TRUE) {
            packetBuffer[packetBufferIndex].request = c[0];
            packetCommandFlag                       = FALSE;
            lastChar                                = c[0];
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

                // Very sure this is the end of a packet. Reset byte index to 0
                packetByteIndex = 0;
                lastChar        = c[0];

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

    // if (com_ports_open_connection(WATCHDOG_PING_CODE) != TRUE) {
    //     printf("Unable to connect to Watchdog\n");
    //     return FALSE;
    // }

    // HANDLE thread = CreateThread(NULL, 0, maple_listen_rx, NULL, 0, NULL);

    // if (!thread) {
    //     printf("Thread failed\n");
    //     return 0;
    // }

    // // Watchdog connected. Get information from watchdog to display on the screen
    // maple_create_and_send_bpacket(BPACKET_GET_R_STATUS, 0, NULL);

    // // Wait until the packet is ready
    // while (packetPendingIndex == packetBufferIndex) {}

    // if (packetBuffer[packetPendingIndex].request != BPACKET_R_SUCCESS) {
    //     printf("Error recieving status!\n");
    //     return 0;
    // }

    // watchdog_info_t watchdogInfo;
    // watchdogInfo.id               = packetBuffer[packetPendingIndex].bytes[0];
    // watchdogInfo.cameraResolution = packetBuffer[packetPendingIndex].bytes[1];
    // watchdogInfo.numImages =
    //     (packetBuffer[packetPendingIndex].bytes[2] << 8) | packetBuffer[packetPendingIndex].bytes[3];
    // watchdogInfo.status = (packetBuffer[packetPendingIndex].bytes[4] == 0) ? SYSTEM_STATUS_OK : SYSTEM_STATUS_ERROR;
    // sprintf(watchdogInfo.datetime, "01/03/2022 9:15 AM");

    // packetPendingIndex++;

    uint32_t flags     = 0;
    uint8_t cameraView = FALSE;

    HANDLE thread = CreateThread(NULL, 0, gui, &flags, 0, NULL);

    if (!thread) {
        printf("Thread failed\n");
        return 0;
    }

    // gui_init(&watchdogInfo, &flags);

    while (1) {

        if ((flags & GUI_TURN_RED_LED_ON) != 0) {
            flags &= ~(GUI_TURN_RED_LED_ON);
            maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_ON, 0, NULL);
        }

        if ((flags & GUI_TURN_RED_LED_OFF) != 0) {
            flags &= ~(GUI_TURN_RED_LED_OFF);
            maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_OFF, 0, NULL);
        }

        if ((flags & GUI_CAMERA_VIEW_ON) != 0) {
            flags &= ~(GUI_CAMERA_VIEW_ON);

            // Send request to set resolution to low value so sending images
            // over UART has a quick enough frame rate
            // maple_create_and_send_bpacket(, 0, NULL);
            cameraView = TRUE;
        }

        if ((flags & GUI_CAMERA_VIEW_OFF) != 0) {
            flags &= ~(GUI_CAMERA_VIEW_OFF);
            cameraView = FALSE;
        }

        if (cameraView == TRUE) {

            // Request image from watchdog
            maple_create_and_send_bpacket(WATCHDOG_BPK_R_CAMERA_VIEW, 0, NULL);

            // Wait for watchdog to transmit photo over UART

            if (maple_receive_camera_view(CAMERA_VIEW_FILENAME) == TRUE) {
                flags |= GUI_UPDATE_CAMERA_VIEW;
            }
        }

        if ((flags & GUI_CLOSE) != 0) {
            break;
        }
    }
}

/**
 * @brief This function takes in user input from the command line and processes
 * it. This function is deprecated since the GUI interface has been made
 *
 */
void maple_command_line() {

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

        if (maple_match_args(args, numArgs) == FALSE) {
            printf(ASCII_COLOR_RED "Unkown command: '%s'\n" ASCII_COLOR_WHITE, userInput);
        }

        // Free list of args
        for (int i = 0; i < numArgs; i++) {
            free(args[i]);
        }
    }
}