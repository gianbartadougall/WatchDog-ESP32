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
#include "datetime.h"

#define MAPLE_MAX_ARGS     5
#define PACKET_BUFFER_SIZE 50

/* Example of how to get a list of serial ports on the system.
 *
 * This example file is released to the public domain. */

int check(enum sp_return result);
void maple_print_bpacket_data(bpacket_t* bpacket);
void maple_create_and_send_bpacket(uint8_t request, uint8_t receiver, uint8_t numDataBytes,
                                   uint8_t data[BPACKET_MAX_NUM_DATA_BYTES]);
void maple_create_and_send_sbpacket(uint8_t receiver, uint8_t request, char* string);
void maple_print_uart_response(void);
uint8_t maple_match_args(char** args, int numArgs);

#define RX_BUFFER_SIZE (BPACKET_BUFFER_LENGTH_BYTES * 50)
uint8_t rxBuffer[RX_BUFFER_SIZE];

bpacket_t packetBuffer[PACKET_BUFFER_SIZE];
uint8_t guiWriteIndex  = 0;
uint8_t guiReadIndex   = 0;
uint8_t mainWriteIndex = 0;
uint8_t mainReadIndex  = 0;

bpacket_t guiToMainBpackets[BPACKET_CIRCULAR_BUFFER_SIZE];
bpacket_t mainToGuiBpackets[BPACKET_CIRCULAR_BUFFER_SIZE];

uint8_t packetBufferIndex  = 0;
uint8_t packetPendingIndex = 0;
bpacket_t packetBuffer[PACKET_BUFFER_SIZE];

uint8_t currentByte = 0;
uint8_t pastByte1   = BPACKET_STOP_BYTE_UPPER;
uint8_t pastByte2   = 0;
uint8_t pastByte3   = 0;

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

void maple_create_and_send_bpacket(uint8_t request, uint8_t receiver, uint8_t numDataBytes,
                                   uint8_t data[BPACKET_MAX_NUM_DATA_BYTES]) {
    bpacket_t bpacket;
    bpacket_create_p(&bpacket, receiver, BPACKET_ADDRESS_MAPLE, BPACKET_CODE_EXECUTE, request, numDataBytes, data);
    if (com_ports_send_bpacket(&bpacket) != TRUE) {
        return;
    }
}

void maple_create_and_send_sbpacket(uint8_t receiver, uint8_t request, char* string) {
    bpacket_t bpacket;
    bpacket_create_sp(&bpacket, receiver, BPACKET_ADDRESS_MAPLE, BPACKET_CODE_EXECUTE, request, string);
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
    maple_create_and_send_bpacket(BPACKET_ADDRESS_ESP32, WATCHDOG_BPK_R_COPY_FILE, filePathNumBytes, bpacketBuffer);

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

        if (packetBuffer[packetPendingIndex].request != BPACKET_CODE_IN_PROGRESS &&
            packetBuffer[packetPendingIndex].request != BPACKET_CODE_SUCCESS) {
            printf("PACKET ERROR FOUND. Request %i\n", packetBuffer[packetPendingIndex].request);
            fclose(target);
            return FALSE;
        }

        if (packetBuffer[packetPendingIndex].request == BPACKET_CODE_SUCCESS) {
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

        if (packetBuffer[packetPendingIndex].request != BPACKET_CODE_IN_PROGRESS &&
            packetBuffer[packetPendingIndex].request != BPACKET_CODE_SUCCESS) {
            printf("PACKET ERROR FOUND. Request %i\n", packetBuffer[packetPendingIndex].request);
            fclose(target);
            return FALSE;
        }

        if (packetBuffer[packetPendingIndex].request == BPACKET_CODE_SUCCESS) {
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

    printf("Request: %i\n", bpacket->request);
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

        if (packetBuffer[packetPendingIndex].request == BPACKET_CODE_SUCCESS) {
            packetsFinished = TRUE;
        }

        maple_increment_packet_pending_index();
    }
}

uint8_t maple_get_uart_single_response(bpacket_t* bpacket) {

    // Wait until the packet is ready
    while (packetPendingIndex == packetBufferIndex) {}

    // Packet ready. Copy contents to given bpacket
    bpacket->request  = packetBuffer[packetPendingIndex].request;
    bpacket->numBytes = packetBuffer[packetPendingIndex].numBytes;

    // Packet ready. Print its contents
    for (int i = 0; i < packetBuffer[packetPendingIndex].numBytes; i++) {
        bpacket->bytes[i] = packetBuffer[packetPendingIndex].bytes[i];
    }

    maple_increment_packet_pending_index();

    if (bpacket->request != BPACKET_CODE_SUCCESS) {
        return FALSE;
    }

    return TRUE;
}

void update_past_bytes(void) {
    pastByte3 = pastByte2;
    pastByte2 = pastByte1;
    pastByte1 = currentByte;
}

DWORD WINAPI maple_listen_rx(void* arg) {

    uint16_t numBytes            = 0;
    uint8_t expectedByteId       = BPACKET_START_BYTE_UPPER_ID;
    uint8_t numDataBytesReceived = 0;
    uint8_t numDataBytesExpected = 0;
    uint8_t bpacketByteIndex     = 0;
    uint8_t byte;

    while ((numBytes = com_ports_read(&byte, 1, 0)) >= 0) {

        // printf("%c", byte);

        if (expectedByteId == BPACKET_DATA_BYTE_ID) {

            // Increment the number of data bytes received
            numDataBytesReceived++;

            // Set the expected byte to stop byte if this is the last data byte expected
            if (numDataBytesReceived == numDataBytesExpected) {
                expectedByteId = BPACKET_STOP_BYTE_UPPER_ID;
            }

            // char msg[40];
            // sprintf(msg, "DATA BYTE[%i]: [%i] [%i]", bufferId, numDataBytesReceived[bufferId],
            //         numDataBytesExpected[bufferId]);
            // printf(msg, sizeof(msg));

            // Add byte to bpacket
            packetBuffer[packetBufferIndex].bytes[bpacketByteIndex++] = byte;
            continue;
        }

        if ((expectedByteId == BPACKET_STOP_BYTE_LOWER_ID) && (byte == BPACKET_STOP_BYTE_LOWER)) {

            // printf(" STP BYTE LW ");

            // End of packet reached. Reset the system
            expectedByteId = BPACKET_START_BYTE_UPPER_ID;

            // Increment the packet buffer. Reset the buffer index
            maple_increment_packet_buffer_index();
            bpacketByteIndex = 0;
            continue;
        }

        if ((expectedByteId == BPACKET_STOP_BYTE_UPPER_ID) && (byte == BPACKET_STOP_BYTE_UPPER)) {

            // printf(" STP BYTE UP ");
            expectedByteId = BPACKET_STOP_BYTE_LOWER_ID;
            continue;
        }

        if (expectedByteId == BPACKET_NUM_BYTES_BYTE_ID) {
            // printf("[LEN BYTE]");

            // Set the number of bytes in the bpacket
            packetBuffer[packetBufferIndex].numBytes = byte;

            // Set the number of data bytes expected
            numDataBytesExpected = byte;
            numDataBytesReceived = 0;

            // Update the expected byte to data bytes
            if (byte == 0) {
                expectedByteId = BPACKET_STOP_BYTE_UPPER_ID;
            } else {
                expectedByteId = BPACKET_DATA_BYTE_ID;
            }

            continue;
        }

        if (expectedByteId == BPACKET_CODE_BYTE_ID) {
            // printf(" COD BYTE  ");

            packetBuffer[packetBufferIndex].code = byte;
            expectedByteId                       = BPACKET_NUM_BYTES_BYTE_ID;
            continue;
        }

        if (expectedByteId == BPACKET_REQUEST_BYTE_ID) {
            // printf(" REQ BYTE  ");

            packetBuffer[packetBufferIndex].request = byte;

            expectedByteId = BPACKET_CODE_BYTE_ID;
            continue;
        }

        if (expectedByteId == BPACKET_SENDER_BYTE_ID) {
            // printf(" SND BYTE  ");

            packetBuffer[packetBufferIndex].sender = byte;

            expectedByteId = BPACKET_REQUEST_BYTE_ID;
            continue;
        }

        if (expectedByteId == BPACKET_RECEIVER_BYTE_ID) {

            // printf(" REC BYTE  ");

            packetBuffer[packetBufferIndex].receiver = byte;
            expectedByteId                           = BPACKET_SENDER_BYTE_ID;
            continue;
        }

        if ((expectedByteId == BPACKET_START_BYTE_LOWER_ID) && (byte == BPACKET_START_BYTE_LOWER)) {
            // printf(" START LOWER ");
            expectedByteId = BPACKET_RECEIVER_BYTE_ID;
            continue;
        }

        if ((expectedByteId == BPACKET_START_BYTE_UPPER_ID) && (byte == BPACKET_START_BYTE_UPPER)) {
            expectedByteId = BPACKET_START_BYTE_LOWER_ID;
            // printf(" START UPPER ");
            // Reset the byte index
            bpacketByteIndex = 0;

            continue;
        }

        printf("%c", byte);
        // printf(" FAILED [%i] exp [%i] ", byte, expectedByteId);
        // Erraneous byte. Reset the system
        expectedByteId = BPACKET_START_BYTE_UPPER_ID;
    }

    if (numBytes < 0) {
        printf("Error reading COM port\n");
    }

    return FALSE;
}

void copy_file_test_function(void) {

    if (com_ports_open_connection(BPACKET_ADDRESS_STM32, WATCHDOG_PING_CODE_STM32) != TRUE) {
        printf("Unable to connect to Watchdog\n");
        return;
    }

    HANDLE thread = CreateThread(NULL, 0, maple_listen_rx, NULL, 0, NULL);

    if (!thread) {
        printf("Thread failed\n");
        return;
    }

    // Create file to copy data into
    FILE* target;
    target = fopen("testFile1.jpg", "wb"); // Read binary

    if (target == NULL) {
        printf("Could not open file\n");
        fclose(target);
        return;
    }

    // Create bpacket message to request file to be copied
    bpacket_t bpacket;
    char imageName[] = "img0.jpg";
    bpacket_create_sp(&bpacket, BPACKET_ADDRESS_ESP32, BPACKET_ADDRESS_MAPLE, WATCHDOG_BPK_R_COPY_FILE,
                      BPACKET_CODE_EXECUTE, imageName);
    com_ports_send_bpacket(&bpacket);

    printf("Sent\n");
    int i = 0;
    bpacket_t* packet;
    while (1) {

        // printf("Packets received\n");

        while (packetPendingIndex == packetBufferIndex) {}

        packet = &packetBuffer[packetPendingIndex];

        // for (int i = 0; i < packet->numBytes; i++) {
        //     printf("%c", packet->bytes[i]);
        // }
        // printf("\n");
        // maple_increment_packet_pending_index();

        // Skip the packet if the reciever data is not for the copy file
        // printf("here\n");
        if (packetBuffer[packetPendingIndex].request != WATCHDOG_BPK_R_COPY_FILE) {
            maple_increment_packet_pending_index();
            printf("Skipping\n");
            continue;
        }
        // printf(" donkey \n");
        // printf("Sender: %i Receiver: %i Request: %i Code: %i Num bytes: %i\n", packet->sender, packet->receiver,
        //        packet->request, packet->code, packet->numBytes);

        if (packet->numBytes != 255) {
            for (int i = 0; i < packet->numBytes; i++) {
                printf("%c", packet->bytes[i]);
            }
            printf("\n");
        }

        i++;
        // Write the contents to the file
        for (int i = 0; i < packetBuffer[packetPendingIndex].numBytes; i++) {
            fputc(packetBuffer[packetPendingIndex].bytes[i], target);
        }

        printf("Packet [%i] written\n", i);

        if (packetBuffer[packetPendingIndex].code == BPACKET_CODE_SUCCESS) {
            maple_increment_packet_pending_index();
            break;
        }
        maple_increment_packet_pending_index();
    }

    printf("Finished\n");
    fclose(target);
}

int main(int argc, char** argv) {

    copy_file_test_function();
    // comms_port_test();
    while (1) {};

    if (com_ports_open_connection(BPACKET_ADDRESS_STM32, WATCHDOG_PING_CODE_STM32) != TRUE) {
        printf("Unable to connect to Watchdog\n");
        return FALSE;
    }

    HANDLE thread = CreateThread(NULL, 0, maple_listen_rx, NULL, 0, NULL);

    if (!thread) {
        printf("Thread failed\n");
        return 0;
    }

    bpacket_circular_buffer_t guiToMainCircularBuffer;
    bpacket_create_circular_buffer(&guiToMainCircularBuffer, &guiWriteIndex, &mainReadIndex, &guiToMainBpackets[0]);

    bpacket_circular_buffer_t mainToGuiCircularBuffer;
    bpacket_create_circular_buffer(&mainToGuiCircularBuffer, &mainWriteIndex, &guiReadIndex, &mainToGuiBpackets[0]);

    watchdog_info_t watchdogInfo;
    watchdogInfo.id               = packetBuffer[packetPendingIndex].bytes[0];
    watchdogInfo.cameraResolution = packetBuffer[packetPendingIndex].bytes[1];
    watchdogInfo.numImages =
        (packetBuffer[packetPendingIndex].bytes[2] << 8) | packetBuffer[packetPendingIndex].bytes[3];
    watchdogInfo.status = (packetBuffer[packetPendingIndex].bytes[4] == 0) ? SYSTEM_STATUS_OK : SYSTEM_STATUS_ERROR;
    sprintf(watchdogInfo.datetime, "01/03/2022 9:15 AM");

    // packetPendingIndex++;

    uint32_t flags = 0;
    // uint8_t cameraView = FALSE;

    gui_initalisation_t guiInit;
    guiInit.watchdog  = &watchdogInfo;
    guiInit.flags     = &flags;
    guiInit.guiToMain = &guiToMainCircularBuffer;
    guiInit.mainToGui = &mainToGuiCircularBuffer;

    HANDLE guiThread = CreateThread(NULL, 0, gui, &guiInit, 0, NULL);

    if (!guiThread) {
        printf("Thread failed\n");
        return 0;
    }

    while (1) {
        // If a bpacket is recieved from the Gui, deal with it in here
        if (*guiToMainCircularBuffer.readIndex != *guiToMainCircularBuffer.writeIndex) {

            bpacket_increment_circular_buffer_index(guiToMainCircularBuffer.readIndex);
        }

        // If their is a bpacket put into the packet buffer, it gets put in the main-to-gui circular buffer
        if (packetBufferIndex != packetPendingIndex) {

            *mainToGuiCircularBuffer.circularBuffer[*guiToMainCircularBuffer.writeIndex] =
                packetBuffer[packetPendingIndex];

            // Increase packet pending index so code it is know that the incoming data was dealt with
            maple_increment_packet_pending_index();
            // Increae write index of the main to gui circular buffer so that it can be parsed to
            // the GUI
            bpacket_increment_circular_buffer_index(mainToGuiCircularBuffer.writeIndex);
        }
    }
    while (1) {

        // if ((flags & GUI_TURN_RED_LED_OFF) != 0) {
        //     flags &= ~(GUI_TURN_RED_LED_OFF);
        //     maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_OFF, 0, NULL);
        // }

        bpacket_t* bpacket = guiToMainCircularBuffer.circularBuffer[*guiToMainCircularBuffer.readIndex];
        bpacket_increment_circular_buffer_index(guiToMainCircularBuffer.readIndex);

        switch (bpacket->request) {
            default:
                printf("Request: %i\n", bpacket->request);
                com_ports_send_bpacket(bpacket);
        }
    }

    return 0;
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

/**
 * @brief This function takes in a list of string arguments and matches them
 * to the correct uart messages to be transmitted
 *
 */
uint8_t maple_match_args(char** args, int numArgs) {

    if (numArgs == 1) {

        if (chars_same(args[0], "help\0") == TRUE) {
            maple_create_and_send_bpacket(BPACKET_ADDRESS_STM32, BPACKET_GEN_R_HELP, 0, NULL);
            maple_print_uart_response();
            return TRUE;
        }

        if (chars_same(args[0], "clc\0")) {
            printf(ASCII_CLEAR_SCREEN);
            return TRUE;
        }

        if (chars_same(args[0], "ls\0") == TRUE) {
            maple_create_and_send_sbpacket(BPACKET_ADDRESS_ESP32, WATCHDOG_BPK_R_LIST_DIR, "\0");
            maple_print_uart_response();
            return TRUE;
        }

        if (chars_same(args[0], "photo\0") == TRUE) {
            maple_create_and_send_bpacket(BPACKET_ADDRESS_STM32, WATCHDOG_BPK_R_TAKE_PHOTO, 0, NULL);
            maple_print_uart_response();
            return TRUE;
        }
    }

    if (numArgs == 2) {

        if (chars_same(args[0], "ls\0") == TRUE) {
            maple_create_and_send_sbpacket(BPACKET_ADDRESS_ESP32, WATCHDOG_BPK_R_LIST_DIR, args[1]);
            maple_print_uart_response();
            return TRUE;
        }
    }

    if (numArgs == 3) {

        if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
            chars_same(args[2], "on\0") == TRUE) {
            maple_create_and_send_bpacket(BPACKET_ADDRESS_ESP32, WATCHDOG_BPK_R_LED_RED_ON, 0, NULL);
            return TRUE;
        }

        if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
            chars_same(args[2], "off\0") == TRUE) {
            maple_create_and_send_bpacket(BPACKET_ADDRESS_ESP32, WATCHDOG_BPK_R_LED_RED_OFF, 0, NULL);
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