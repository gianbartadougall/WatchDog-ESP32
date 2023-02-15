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
#include <stdio.h>
#include <windows.h> // Use for multi threading
#include "time.h"    // Use for ms timer

/* C Library Includes for COM Port Interfacing */
#include <libserialport.h>

/* Personal Includes */
#include "chars.h"
#include "uart_lib.h"
#include "watchdog_defines.h"
#include "bpacket.h"
#include "gui.h"
#include "datetime.h"
#include "uart_lib.h"
#include "bpacket.h"

#define MAPLE_MAX_ARGS     5
#define PACKET_BUFFER_SIZE 50

bpacket_circular_buffer_t guiToMainCircularBuffer1;
bpacket_circular_buffer_t mainToGuiCircularBuffer1;

#define GTM_CB_CURRENT_BPACKET (guiToMainCircularBuffer1.circularBuffer[*guiToMainCircularBuffer1.readIndex])

/* Example of how to get a list of serial ports on the system.
 *
 * This example file is released to the public domain. */

int check(enum sp_return result);
void maple_print_bpacket_data(bpacket_t* bpacket);
void maple_create_and_send_bpacket(uint8_t request, uint8_t receiver, uint8_t numDataBytes,
                                   uint8_t data[BPACKET_MAX_NUM_DATA_BYTES]);
void maple_create_and_send_sbpacket(uint8_t request, uint8_t receiver, char* string);
void maple_print_uart_response(void);
uint8_t maple_match_args(char** args, int numArgs);
void maple_command_line(void);
void maple_test(void);

uint8_t guiWriteIndex  = 0;
uint8_t guiReadIndex   = 0;
uint8_t mainWriteIndex = 0;
uint8_t mainReadIndex  = 0;

bpacket_t guiToMainBpackets[BPACKET_CIRCULAR_BUFFER_SIZE];
bpacket_t mainToGuiBpackets[BPACKET_CIRCULAR_BUFFER_SIZE];

uint32_t packetBufferIndex  = 0;
uint32_t packetPendingIndex = 0;
bpacket_t packetBuffer[PACKET_BUFFER_SIZE];

struct sp_port* activePort = NULL;

uint8_t maple_send_bpacket(bpacket_t* bpacket) {

    bpacket_buffer_t packetBuffer;
    bpacket_to_buffer(bpacket, &packetBuffer);

    if (sp_blocking_write(activePort, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
        return FALSE;
    }

    return TRUE;
}

bpacket_t* maple_get_next_bpacket_response(void) {

    if (packetPendingIndex == packetBufferIndex) {
        return NULL;
    }

    bpacket_t* buffer = &packetBuffer[packetPendingIndex];
    bpacket_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
    return buffer;
}

void maple_create_and_send_bpacket(uint8_t request, uint8_t receiver, uint8_t numDataBytes,
                                   uint8_t data[BPACKET_MAX_NUM_DATA_BYTES]) {
    bpacket_t bpacket;
    uint8_t result =
        bpacket_create_p(&bpacket, receiver, BPACKET_ADDRESS_MAPLE, request, BPACKET_CODE_EXECUTE, numDataBytes, data);

    if (result != TRUE) {
        char errMsg[50];
        bpacket_get_error(result, errMsg);
        printf("Failed to create bpacket with the error: %s %i\r\n", errMsg, BPACKET_CODE_EXECUTE);
        return;
    }

    if (maple_send_bpacket(&bpacket) != TRUE) {
        return;
    }
}

void maple_create_and_send_sbpacket(uint8_t request, uint8_t receiver, char* string) {
    bpacket_t bpacket;

    uint8_t result =
        bpacket_create_sp(&bpacket, receiver, BPACKET_ADDRESS_MAPLE, request, BPACKET_CODE_EXECUTE, string);

    if (result != TRUE) {
        char errMsg[50];
        bpacket_get_error(result, errMsg);
        printf("Failed to create string bpacket with error: %s", errMsg);
        return;
    }

    if (maple_send_bpacket(&bpacket) != TRUE) {
        return;
    }
}

void maple_print_bpacket_data(bpacket_t* bpacket) {

    if (bpacket->request != BPACKET_GEN_R_MESSAGE) {
        printf("Request: %i Len: %i Data: ", bpacket->request, bpacket->numBytes);
    }

    for (int i = 0; i < bpacket->numBytes; i++) {
        printf("%c", bpacket->bytes[i]);
    }
}

void maple_print_uart_response(void) {

    int packetsFinished = FALSE;
    while (packetsFinished == FALSE) {

        // Wait until the packet is ready
        while (packetPendingIndex == packetBufferIndex) {}

        bpacket_t* bpacket = maple_get_next_bpacket_response();

        // Packet ready. Print its contents
        for (int i = 0; i < bpacket->numBytes; i++) {
            printf("%c", bpacket->bytes[i]);
        }

        if (bpacket->request == BPACKET_CODE_SUCCESS) {
            packetsFinished = TRUE;
        }
    }
}

int maple_read_port(void* buf, size_t count, unsigned int timeout_ms) {

    if (activePort == NULL) {
        return 0;
    }

    return sp_blocking_read(activePort, buf, count, timeout_ms);
}

DWORD WINAPI maple_listen_rx(void* arg) {

    uint16_t numBytes            = 0;
    uint8_t expectedByteId       = BPACKET_START_BYTE_UPPER_ID;
    uint8_t numDataBytesReceived = 0;
    uint8_t numDataBytesExpected = 0;
    uint8_t bpacketByteIndex     = 0;
    uint8_t byte;

    while ((numBytes = maple_read_port(&byte, 1, 0)) >= 0) {

        if (expectedByteId == BPACKET_DATA_BYTE_ID) {

            // Increment the number of data bytes received
            numDataBytesReceived++;

            // Set the expected byte to stop byte if this is the last data byte expected
            if (numDataBytesReceived == numDataBytesExpected) {
                expectedByteId = BPACKET_STOP_BYTE_UPPER_ID;
            }

            // Add byte to bpacket
            packetBuffer[packetBufferIndex].bytes[bpacketByteIndex++] = byte;
            continue;
        }

        if ((expectedByteId == BPACKET_STOP_BYTE_LOWER_ID) && (byte == BPACKET_STOP_BYTE_LOWER)) {

            // End of packet reached. Reset the system
            expectedByteId = BPACKET_START_BYTE_UPPER_ID;

            // Print out the data of the bpacket if the bpacket was a message
            if ((packetBuffer[packetBufferIndex].request == BPACKET_GEN_R_MESSAGE) ||
                (packetBuffer[packetBufferIndex].code == BPACKET_CODE_ERROR)) {

                switch (packetBuffer[packetBufferIndex].code) {

                    case BPACKET_CODE_SUCCESS:
                        printf(ASCII_COLOR_GREEN);
                        break;

                    case BPACKET_CODE_DEBUG:
                        printf(ASCII_COLOR_BLUE);
                        break;

                    case BPACKET_CODE_TODO:
                        printf(ASCII_COLOR_MAGENTA);
                        break;

                    case BPACKET_CODE_ERROR:
                        printf(ASCII_COLOR_RED);
                        break;

                    default:
                        break;
                }

                for (int i = 0; i < packetBuffer[packetBufferIndex].numBytes; i++) {
                    printf("%c", packetBuffer[packetBufferIndex].bytes[i]);
                }
                printf(ASCII_COLOR_WHITE);
            }

            // Increment the packet buffer. Reset the buffer index
            bpacket_increment_circ_buff_index(&packetBufferIndex, PACKET_BUFFER_SIZE);

            bpacketByteIndex = 0;
            continue;
        }

        if ((expectedByteId == BPACKET_STOP_BYTE_UPPER_ID) && (byte == BPACKET_STOP_BYTE_UPPER)) {
            expectedByteId = BPACKET_STOP_BYTE_LOWER_ID;
            continue;
        }

        if (expectedByteId == BPACKET_NUM_BYTES_BYTE_ID) {

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
            packetBuffer[packetBufferIndex].code = byte;

            expectedByteId = BPACKET_NUM_BYTES_BYTE_ID;
            continue;
        }

        if (expectedByteId == BPACKET_REQUEST_BYTE_ID) {
            packetBuffer[packetBufferIndex].request = byte;

            expectedByteId = BPACKET_CODE_BYTE_ID;
            continue;
        }

        if (expectedByteId == BPACKET_SENDER_BYTE_ID) {
            packetBuffer[packetBufferIndex].sender = byte;

            expectedByteId = BPACKET_REQUEST_BYTE_ID;
            continue;
        }

        if (expectedByteId == BPACKET_RECEIVER_BYTE_ID) {

            packetBuffer[packetBufferIndex].receiver = byte;
            expectedByteId                           = BPACKET_SENDER_BYTE_ID;
            continue;
        }

        if ((expectedByteId == BPACKET_START_BYTE_LOWER_ID) && (byte == BPACKET_START_BYTE_LOWER)) {

            expectedByteId = BPACKET_RECEIVER_BYTE_ID;
            continue;
        }

        if ((expectedByteId == BPACKET_START_BYTE_UPPER_ID) && (byte == BPACKET_START_BYTE_UPPER)) {
            expectedByteId = BPACKET_START_BYTE_LOWER_ID;

            // Reset the byte index
            bpacketByteIndex = 0;

            continue;
        }

        printf("%c", byte);

        // Erraneous byte. Reset the system
        expectedByteId = BPACKET_START_BYTE_UPPER_ID;
    }

    if (numBytes < 0) {
        printf("Error reading COM port\n");
    }

    return FALSE;
}

uint8_t maple_connect_to_device(uint8_t address, uint8_t pingCode) {

    // Create a struct to hold all the COM ports currently in use
    struct sp_port** port_list;

    if (sp_list_ports(&port_list) != SP_OK) {
        printf("Failed to list ports\r\n");
        return 0;
    }

    // Loop through all the ports
    uint8_t portFound = FALSE;

    for (int i = 0; port_list[i] != NULL; i++) {

        activePort = port_list[i];

        // Open the port
        enum sp_return result = sp_open(activePort, SP_MODE_READ_WRITE);
        if (result != SP_OK) {
            return result;
        }

        // Configure the port settings for communication
        sp_set_baudrate(activePort, 115200);
        sp_set_bits(activePort, 8);
        sp_set_parity(activePort, SP_PARITY_NONE);
        sp_set_stopbits(activePort, 1);
        sp_set_flowcontrol(activePort, SP_FLOWCONTROL_NONE);

        // Send a ping
        bpacket_t bpacket;
        bpacket_buffer_t bpacketBuffer;
        bpacket_create_p(&bpacket, BPACKET_ADDRESS_STM32, BPACKET_ADDRESS_MAPLE, BPACKET_GEN_R_PING,
                         BPACKET_CODE_EXECUTE, 0, NULL);
        bpacket_to_buffer(&bpacket, &bpacketBuffer);

        if (sp_blocking_write(activePort, bpacketBuffer.buffer, bpacketBuffer.numBytes, 100) < 0) {
            printf("Unable to write\n");
            continue;
        }

        // Response may include other incoming messages as well, not just a response to a ping.
        // Create a timeout of 2 seconds and look for response from ping

        clock_t startTime = clock();
        while ((clock() - startTime) < 200) {

            // Wait for bpacket to be received
            while (packetPendingIndex != packetBufferIndex) {

                // Confirm the request is valid
                if (packetBuffer[packetPendingIndex].request == BPACKET_GEN_R_PING) {

                    // Confirm the ping code was correct
                    if (packetBuffer[packetPendingIndex].bytes[0] == WATCHDOG_PING_CODE_STM32) {
                        portFound = TRUE;
                    }
                }

                bpacket_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
            }
        }

        // Close port and check next port if this was not the correct port
        if (portFound != TRUE) {
            sp_close(activePort);
            continue;
        }

        break;
    }

    if (portFound != TRUE) {
        return FALSE;
    }

    return TRUE;
}

uint8_t maple_stream(char* cpyFileName) {

    FILE* target;
    target = fopen(cpyFileName, "wb"); // Read binary

    if (target == NULL) {
        printf("Could not open file\n");
        fclose(target);
        return FALSE;
    }

    // Send command to copy file. Keeping reading until no more data to send across
    maple_create_and_send_bpacket(WATCHDOG_BPK_R_STREAM_IMAGE, BPACKET_ADDRESS_ESP32, 0, NULL);

    int packetsFinished = FALSE;
    clock_t startTime;
    while (packetsFinished == FALSE) {

        // Wait until the packet is ready
        startTime = clock();
        while (packetPendingIndex == packetBufferIndex) {

            if ((clock() - startTime) > 6000) {
                printf("Timeout 1000ms\n");
                return FALSE;
            }
        }

        if (packetBuffer[packetPendingIndex].request == BPACKET_GEN_R_MESSAGE) {
            bpacket_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
            continue;
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

        printf("Storing data\n");
        for (int i = 0; i < packetBuffer[packetPendingIndex].numBytes; i++) {
            fputc(packetBuffer[packetPendingIndex].bytes[i], target);
        }

        bpacket_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
    }

    fclose(target);

    return TRUE;
}

int main(int argc, char** argv) {

    HANDLE thread = CreateThread(NULL, 0, maple_listen_rx, NULL, 0, NULL);

    if (!thread) {
        printf("Thread failed\n");
        return 0;
    }

    // Try connect to the device
    if (maple_connect_to_device(BPACKET_ADDRESS_STM32, WATCHDOG_PING_CODE_STM32) != TRUE) {
        printf("Unable to connect to device\n");
        TerminateThread(thread, 0);
        return FALSE;
    }

    printf("Connected to port %s\n", sp_get_port_name(activePort));

    maple_test();

    // maple_stream("testImage.jpg");
    // printf("File saved\n");

    maple_command_line();

    bpacket_circular_buffer_t guiToMainCircularBuffer1;
    bpacket_create_circular_buffer(&guiToMainCircularBuffer1, &guiWriteIndex, &mainReadIndex, &guiToMainBpackets[0]);

    bpacket_circular_buffer_t mainToGuiCircularBuffer1;
    bpacket_create_circular_buffer(&mainToGuiCircularBuffer1, &mainWriteIndex, &guiReadIndex, &mainToGuiBpackets[0]);

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
    guiInit.guiToMain = &guiToMainCircularBuffer1;
    guiInit.mainToGui = &mainToGuiCircularBuffer1;

    guiInit.guiToMain = &guiToMainCircularBuffer1;
    guiInit.mainToGui = &mainToGuiCircularBuffer1;

    HANDLE guiThread = CreateThread(NULL, 0, gui, &guiInit, 0, NULL);

    uint8_t startOfImgData = TRUE;
    FILE* streamImage      = NULL;

    if (!guiThread) {
        printf("Thread failed\n");
        return 0;
    }

    while (1) {
        // If a bpacket is recieved from the Gui, deal with it in here
        if (*guiToMainCircularBuffer1.readIndex != *guiToMainCircularBuffer1.writeIndex) {
            uint8_t sendStatus = maple_send_bpacket(GTM_CB_CURRENT_BPACKET);
            if (sendStatus != TRUE) {
                char* sendBpErrorMsg = "HOPEFULLY THIS WILL BE OVERRIDEN IF THERE IS AN ERROR\n";
                bpacket_get_error(sendStatus, sendBpErrorMsg);
                printf("%s\n", sendBpErrorMsg);
            }

            // printf("Sending Request %i\n", GTM_CB_CURRENT_BPACKET->request);

            bpacket_increment_circular_buffer_index(guiToMainCircularBuffer1.readIndex);
        }

        // If their is a bpacket put into the packet buffer, it gets put in the main-to-gui circular
        // buffer

        if (packetBufferIndex != packetPendingIndex) {

            bpacket_t* receivedBpacket = maple_get_next_bpacket_response();

            if (receivedBpacket->request == BPACKET_GEN_R_MESSAGE) {
                continue;
            }

            if (receivedBpacket->request == WATCHDOG_BPK_R_STREAM_IMAGE) {
                if (startOfImgData == TRUE) {
                    startOfImgData = FALSE;

                    if ((streamImage = fopen(CAMERA_VIEW_FILENAME, "wb")) == NULL) {
                        // TODO: make this do something proper
                        printf("Couldnt open stream file");
                    }
                    for (int i = 0; i < receivedBpacket->numBytes; i++) {
                        fputc(receivedBpacket->bytes[i], streamImage);
                    }
                } else if (receivedBpacket->code == BPACKET_CODE_SUCCESS) {
                    startOfImgData = TRUE;
                    for (int i = 0; i < receivedBpacket->numBytes; i++) {
                        fputc(receivedBpacket->bytes[i], streamImage);
                    }
                    fclose(streamImage);
                    // Create Bpacket to update the thing
                    bpacket_create_p(mainToGuiCircularBuffer1.circularBuffer[*mainToGuiCircularBuffer1.writeIndex],
                                     BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_MAPLE, BPACKET_CODE_EXECUTE,
                                     GUI_BPK_R_UPDATE_STREAM_IMAGE, 0, NULL);
                    bpacket_increment_circular_buffer_index(mainToGuiCircularBuffer1.writeIndex);
                } else if (receivedBpacket->code == BPACKET_CODE_IN_PROGRESS) {
                    for (int i = 0; i < receivedBpacket->numBytes; i++) {
                        fputc(receivedBpacket->bytes[i], streamImage);
                    }
                }
                continue;
            }

            mainToGuiCircularBuffer1.circularBuffer[*mainToGuiCircularBuffer1.writeIndex] = receivedBpacket;

            // Increae write index of the main to gui circular buffer so that it can be parsed to
            // the GUI
            bpacket_increment_circular_buffer_index(mainToGuiCircularBuffer1.writeIndex);
        }
    }

    return 0;
}

uint8_t maple_response_is_valid(uint8_t expectedRequest, uint16_t timeout) {

    // Print response
    clock_t startTime = clock();
    while ((clock() - startTime) < timeout) {

        bpacket_t* bpacket = maple_get_next_bpacket_response();

        // If there is next response yet then skip
        if (bpacket == NULL) {
            continue;
        }

        // Skip if the bpacket is just a message
        if (bpacket->request == BPACKET_GEN_R_MESSAGE) {
            continue;
        }

        // Confirm the received bpacket matches the correct request and code
        if (bpacket->request != expectedRequest) {
            printf("Rec: %i Snd: %i Req: %i Code: %i num bytes: %i\r\n", bpacket->receiver, bpacket->sender,
                   bpacket->request, bpacket->code, bpacket->numBytes);
            // printf("Expected request %i but got %i\n", expectedRequest, bpacket->request);
            return FALSE;
        }

        if (bpacket->code != BPACKET_CODE_SUCCESS) {
            printf("Expected code %i but got %i\n", BPACKET_CODE_SUCCESS, bpacket->request);
            return FALSE;
        }

        return TRUE;
    }

    printf("Failed to get response\n");
    return FALSE;
}

/**
 * @brief This function takes in user input from the command line and processes
 * it. This function is deprecated since the GUI interface has been made
 *
 */
void maple_command_line(void) {

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

    switch (numArgs) {

        case 1:

            if (chars_same(args[0], "help\0") == TRUE) {
                maple_create_and_send_bpacket(BPACKET_GEN_R_HELP, BPACKET_ADDRESS_STM32, 0, NULL);
                return TRUE;
            }

            if (chars_same(args[0], "clc\0")) {
                printf(ASCII_CLEAR_SCREEN);
                return TRUE;
            }

            if (chars_same(args[0], "ls\0") == TRUE) {
                maple_create_and_send_sbpacket(WATCHDOG_BPK_R_LIST_DIR, BPACKET_ADDRESS_ESP32, "\0");
                return TRUE;
            }

            if (chars_same(args[0], "photo\0") == TRUE) {
                maple_create_and_send_bpacket(WATCHDOG_BPK_R_TAKE_PHOTO, BPACKET_ADDRESS_STM32, 0, NULL);
                if (maple_response_is_valid(WATCHDOG_BPK_R_TAKE_PHOTO, 10000) == TRUE) {
                    printf("%sCommand executed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
                } else {
                    printf("%sCommand failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
                }

                return TRUE;
            }

            if (chars_same(args[0], "settings\0") == TRUE) {
                maple_create_and_send_bpacket(WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, BPACKET_ADDRESS_STM32, 0, NULL);
                if (maple_response_is_valid(WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, 3000) == TRUE) {
                    printf("%sCommand executed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
                } else {
                    printf("%sCommand failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
                }

                return TRUE;
            }

            break;

        case 2:

            if (chars_same(args[0], "ls\0") == TRUE) {
                maple_create_and_send_sbpacket(WATCHDOG_BPK_R_LIST_DIR, BPACKET_ADDRESS_ESP32, args[1]);
                return TRUE;
            }

            if (chars_same(args[0], "cpy\0") == TRUE) {
                maple_create_and_send_sbpacket(WATCHDOG_BPK_R_COPY_FILE, BPACKET_ADDRESS_ESP32, args[1]);
                return TRUE;
            }

            break;

        case 3:

            if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
                chars_same(args[2], "on\0") == TRUE) {
                maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_ON, BPACKET_ADDRESS_ESP32, 0, NULL);
                if (maple_response_is_valid(WATCHDOG_BPK_R_LED_RED_ON, 2000) == TRUE) {
                    printf("%sCommand executed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
                } else {
                    printf("%sCommand failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
                }

                return TRUE;
            }

            if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
                chars_same(args[2], "off\0") == TRUE) {
                maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_OFF, BPACKET_ADDRESS_ESP32, 0, NULL);
                if (maple_response_is_valid(WATCHDOG_BPK_R_LED_RED_OFF, 2000) == TRUE) {
                    printf("%sCommand executed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
                } else {
                    printf("%sCommand failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
                }

                return TRUE;
            }

            break;

        default:
            break;
    }

    return FALSE;
}

uint8_t maple_get_response(bpacket_t** bpacket, uint8_t request, uint16_t timeout) {

    // Print response
    clock_t startTime = clock();
    while ((clock() - startTime) < timeout) {

        *bpacket = maple_get_next_bpacket_response();

        // If there is next response yet then skip
        if ((*bpacket != NULL) && ((*bpacket)->request == request)) {
            return TRUE;
        }

        if ((*bpacket) != NULL) {
            printf("Skipping\n");
        }
    }

    return FALSE;
}

uint8_t maple_restart_esp32(void) {

    // Turn the ESP32 off. Wait for 200ms. Turn the ESP32 back on. Wait for 1000ms. This ensures that when we
    // get the capture time settings from the esp32 we can know whether the capture time settings set above
    // were saved to the SD card or not
    bpacket_t* stateBpacket;
    maple_create_and_send_bpacket(WATCHDOG_BPK_R_TURN_OFF, BPACKET_ADDRESS_STM32, 0, NULL);
    if (maple_get_response(&stateBpacket, WATCHDOG_BPK_R_TURN_OFF, 1000) != TRUE) {
        printf("%sTurning the ESP32 off failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        return FALSE;
    }

    clock_t time = clock();
    while ((clock() - time) < 200) {}

    // Delay of 1500ms because the STM32 will automatically delay for 1s when turning the esp32
    // so it has enough time to boot up
    maple_create_and_send_bpacket(WATCHDOG_BPK_R_TURN_ON, BPACKET_ADDRESS_STM32, 0, NULL);
    if (maple_get_response(&stateBpacket, WATCHDOG_BPK_R_TURN_ON, 2000) != TRUE) {
        printf("%sTurning the ESP32 on failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        return FALSE;
    }

    return TRUE;
}

void maple_test(void) {

    bpacket_t bpacket;
    uint8_t result = 0;
    uint8_t failed = FALSE;
    char msg[100];

    /* Turn the red LED on */
    // maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_ON, BPACKET_ADDRESS_STM32, 0, NULL);
    // if (maple_response_is_valid(WATCHDOG_BPK_R_LED_RED_ON, 2000) == FALSE) {
    //     failed = TRUE;
    // }

    // /* Turn the red LED off */
    // maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_OFF, BPACKET_ADDRESS_STM32, 0, NULL);
    // if (maple_response_is_valid(WATCHDOG_BPK_R_LED_RED_OFF, 2000) == FALSE) {
    //     printf("%sTurning the red LED off via STM32 failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // /* Turn the red LED on directly */
    // maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_ON, BPACKET_ADDRESS_ESP32, 0, NULL);
    // if (maple_response_is_valid(WATCHDOG_BPK_R_LED_RED_ON, 2000) == FALSE) {
    //     printf("%sTurning the red LED on directly failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // /* Turn the red LED off directly */
    // maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_OFF, BPACKET_ADDRESS_ESP32, 0, NULL);
    // if (maple_response_is_valid(WATCHDOG_BPK_R_LED_RED_OFF, 2000) == FALSE) {
    //     printf("%sTurning the red LED off directly failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // /* Set the datetime on the STM32 */
    // dt_datetime_t newDatetime;
    // dt_time_init(&newDatetime.time, 30, 15, 8);
    // dt_date_init(&newDatetime.date, 1, 2, 2023);
    // wd_datetime_to_bpacket(&bpacket, BPACKET_ADDRESS_STM32, BPACKET_ADDRESS_MAPLE, WATCHDOG_BPK_R_SET_DATETIME,
    //                        BPACKET_CODE_EXECUTE, &newDatetime);
    // maple_send_bpacket(&bpacket);

    // bpacket_t* datetimeBpacket;
    // if (maple_get_response(&datetimeBpacket, WATCHDOG_BPK_R_SET_DATETIME, 1000) != TRUE) {
    //     printf("%sSetting the datetime from the STM32 failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // if (bpacket_confirm_values(datetimeBpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32,
    //                            WATCHDOG_BPK_R_SET_DATETIME, BPACKET_CODE_SUCCESS, 0, msg) != TRUE) {
    //     printf("%sSetting the datetime on the STM failed. %s%s", ASCII_COLOR_RED, msg, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // /* Get the datetime from the STM32 */
    // maple_create_and_send_bpacket(WATCHDOG_BPK_R_GET_DATETIME, BPACKET_ADDRESS_STM32, 0, NULL);
    // if (maple_get_response(&datetimeBpacket, WATCHDOG_BPK_R_GET_DATETIME, 1000) != TRUE) {
    //     printf("%sGetting the datetime from the STM32 failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // if (bpacket_confirm_values(datetimeBpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32,
    //                            WATCHDOG_BPK_R_GET_DATETIME, BPACKET_CODE_SUCCESS, 7, msg) != TRUE) {
    //     printf("%sGetting the datetime from the STM32 failed. %s%s\n", ASCII_COLOR_RED, msg, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // dt_datetime_t datetime;
    // result = wd_bpacket_to_datetime(datetimeBpacket, &datetime);
    // if (result != TRUE) {
    //     wd_get_error(result, msg);
    //     printf("%sFailed to convert datetime to bpacket. %s%s", ASCII_COLOR_RED, msg, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // } else {
    //     result = 0;
    //     result |= (datetime.time.hour != newDatetime.time.hour);
    //     result |= (datetime.date.day != newDatetime.date.day);
    //     result |= (datetime.date.month != newDatetime.date.month);
    //     result |= (datetime.date.year != newDatetime.date.year);

    //     if (result != 0) {
    //         printf("%sIncorrect datetime retrieved%s", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //         failed = TRUE;
    //     }
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Setting the Camera Settings */
    while (1) {}
    // Create camera settings
    wd_camera_settings_t newCameraSettings = {
        .resolution = WD_CAM_RES_1280x1024,
    };

    result =
        wd_camera_settings_to_bpacket(&bpacket, BPACKET_ADDRESS_ESP32, BPACKET_ADDRESS_MAPLE,
                                      WATCHDOG_BPK_R_SET_CAMERA_SETTINGS, BPACKET_CODE_EXECUTE, &newCameraSettings);
    if (result != TRUE) {
        wd_get_error(result, msg);
        printf("%sFailed to convert camera settings to bpacket. %s\n%s", ASCII_COLOR_RED, msg, ASCII_COLOR_WHITE);
        failed = TRUE;
    } else {
        maple_send_bpacket(&bpacket);
    }

    bpacket_t* cameraSettingsBpacket;
    if (maple_get_response(&cameraSettingsBpacket, WATCHDOG_BPK_R_SET_CAMERA_SETTINGS, 1000) != TRUE) {
        printf("%sSetting the camera settings failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        char info[50];
        bpacket_get_info(cameraSettingsBpacket, info);
        printf(info);
        for (int i = 0; i < cameraSettingsBpacket->numBytes; i++) {
            printf("%c", cameraSettingsBpacket->bytes[i]);
        }
        printf("\n");

        failed = TRUE;
    }

    // Confirm the received bpacket has the expected values
    if (bpacket_confirm_values(cameraSettingsBpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32,
                               WATCHDOG_BPK_R_SET_CAMERA_SETTINGS, BPACKET_CODE_SUCCESS, 0, msg) != TRUE) {
        printf("%sUnexpected response when updating camera settings. %s%s\n", ASCII_COLOR_RED, msg, ASCII_COLOR_WHITE);

        char info[100];
        bpacket_get_info(cameraSettingsBpacket, info);
        printf(info);
        failed = TRUE;
    }

    // Restart the esp32. This will allow maple to determine whether the settings were properly saved
    // onto the SD card or not
    if (maple_restart_esp32() != TRUE) {
        printf("%sMaple failed to restart ESP32%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        failed = TRUE;
    }

    /* Test Getting the Camera Settings */

    maple_create_and_send_bpacket(WATCHDOG_BPK_R_GET_CAMERA_SETTINGS, BPACKET_ADDRESS_ESP32, 0, NULL);
    if (maple_get_response(&cameraSettingsBpacket, WATCHDOG_BPK_R_GET_CAMERA_SETTINGS, 1000) != TRUE) {
        printf("%sGetting the camera settings failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        failed = TRUE;
    }

    // Confirm the received bpacket has the expected values
    if (bpacket_confirm_values(cameraSettingsBpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32,
                               WATCHDOG_BPK_R_GET_CAMERA_SETTINGS, BPACKET_CODE_SUCCESS, 1, msg) != TRUE) {
        printf("%sUnexpected response when updating camera settings. %s%s\n", ASCII_COLOR_RED, msg, ASCII_COLOR_WHITE);
        failed = TRUE;
    }

    // Convert bpacket back to camera settings
    wd_camera_settings_t cameraSettings;
    result = wd_bpacket_to_camera_settings(cameraSettingsBpacket, &cameraSettings);

    if (result != TRUE) {
        wd_get_error(result, msg);
        printf("%sFailed to convert bpacket to camera settings. %s\n%s", ASCII_COLOR_RED, msg, ASCII_COLOR_WHITE);
        failed = TRUE;
    } else {

        // Compare the received camera settings with the camera settings that were set previosuly
        if (cameraSettings.resolution != newCameraSettings.resolution) {
            printf("%sFailed to get correct camera settings. Found %i but expected %i%s\n", ASCII_COLOR_RED,
                   cameraSettings.resolution, newCameraSettings.resolution, ASCII_COLOR_WHITE);
            failed = TRUE;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Setting the Watchdog Capture Time */
    // wd_camera_capture_time_settings_t newCaptureTime = {
    //     .startTime.second    = 30,
    //     .startTime.minute    = 45,
    //     .startTime.hour      = 10,
    //     .endTime.second      = 20,
    //     .endTime.minute      = 15,
    //     .endTime.hour        = 17,
    //     .intervalTime.minute = 35,
    //     .intervalTime.hour   = 2,
    // };

    // result = wd_capture_time_settings_to_bpacket(&bpacket, BPACKET_ADDRESS_STM32, BPACKET_ADDRESS_MAPLE,
    //                                              WATCHDOG_BPK_R_SET_CAPTURE_TIME_SETTINGS, BPACKET_CODE_EXECUTE,
    //                                              &newCaptureTime);
    // if (result != TRUE) {
    //     wd_get_error(result, msg);
    //     printf("%sFailed to convert capture time settings to bpacket. %s\n%s", ASCII_COLOR_RED, msg,
    //     ASCII_COLOR_WHITE); failed = TRUE;
    // } else {
    //     maple_send_bpacket(&bpacket);
    // }

    // bpacket_t* newCaptureTimeSettingsBpacket;
    // // Internally the way the capture time settings is updated on the STM32, you are actually expecting a
    // // WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS request back instead of a WATCHDOG_BPK_R_SET_CAPTURE_TIME_SETTINGS
    // if (maple_get_response(&newCaptureTimeSettingsBpacket, WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, 1000) != TRUE) {
    //     printf("%sSetting the capture time failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // // Confirm the received bpacket has the expected values
    // // Internally the way the capture time settings is updated on the STM32, you are actually expecting a
    // // WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS request back instead of a WATCHDOG_BPK_R_SET_CAPTURE_TIME_SETTINGS
    // if (bpacket_confirm_values(newCaptureTimeSettingsBpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32,
    //                            WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, BPACKET_CODE_SUCCESS, 0, msg) != TRUE) {
    //     printf("%sUnexpected response when updating capture time settings. %s%s\n", ASCII_COLOR_RED, msg,
    //            ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // if (maple_restart_esp32() != TRUE) {
    //     printf("%sMaple failed to restart ESP32%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // /* Test Getting the Watchdog Capture Time from the STM32 */
    // bpacket_t* captureTimeSettingsBpacket;
    // maple_create_and_send_bpacket(WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, BPACKET_ADDRESS_STM32, 0, NULL);
    // if (maple_get_response(&captureTimeSettingsBpacket, WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, 1000) != TRUE) {
    //     printf("%sGetting the capture time settings from the STM32 failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // if (bpacket_confirm_values(captureTimeSettingsBpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32,
    //                            WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, BPACKET_CODE_SUCCESS, 6, msg) != TRUE) {
    //     printf("%sUnexpected response when getting the capture time settings from STM32. %s%s\n", ASCII_COLOR_RED,
    //     msg,
    //            ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // /* Test Getting the Watchdog Capture Time from the ESP32 */
    // maple_create_and_send_bpacket(WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, BPACKET_ADDRESS_ESP32, 0, NULL);
    // if (maple_get_response(&captureTimeSettingsBpacket, WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, 1000) != TRUE) {
    //     printf("%sGetting the capture time settings from ESP32 failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // if (bpacket_confirm_values(captureTimeSettingsBpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32,
    //                            WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, BPACKET_CODE_SUCCESS, 6, msg) != TRUE) {
    //     printf("%sUnexpected response when getting capture time settings from ESP32. %s%s\n", ASCII_COLOR_RED, msg,
    //            ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Taking a Photo */
    // bpacket_t* photoBpacket;
    // maple_create_and_send_bpacket(WATCHDOG_BPK_R_TAKE_PHOTO, BPACKET_ADDRESS_STM32, 0, NULL);
    // if (maple_get_response(&photoBpacket, WATCHDOG_BPK_R_TAKE_PHOTO, 5000) != TRUE) {
    //     printf("%sTaking a photo failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // if (bpacket_confirm_values(photoBpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32, WATCHDOG_BPK_R_TAKE_PHOTO,
    //                            BPACKET_CODE_SUCCESS, 0, msg) != TRUE) {
    //     printf("%sUnexpected response when taking a photo. %s%s\n", ASCII_COLOR_RED, msg, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test listing the directory */
    // bpacket_t* dirBpacket;
    // maple_create_and_send_bpacket(WATCHDOG_BPK_R_LIST_DIR, BPACKET_ADDRESS_ESP32, 0, NULL);
    // if (maple_get_response(&dirBpacket, WATCHDOG_BPK_R_LIST_DIR, 3000) != TRUE) {
    //     printf("%sListing directory failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // } else {
    //     for (int i = 0; i < dirBpacket->numBytes; i++) {
    //         printf("%c", dirBpacket->bytes[i]);
    //     }
    //     printf("\n");
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Copying a File by downloading the image that was just taken */

    // FILE* testImage;
    // if ((testImage = fopen("testImage.jpg", "wb")) != NULL) {

    //     // Send request to ESP32 to copy file
    //     char* fileName = "IMG0.JPG\0";
    //     maple_create_and_send_sbpacket(WATCHDOG_BPK_R_COPY_FILE, BPACKET_ADDRESS_ESP32, fileName);

    //     uint8_t fileTransfered = FALSE;
    //     clock_t time           = clock();
    //     while (fileTransfered != TRUE) {

    //         if ((clock() - time) > 2000) {
    //             fclose(testImage);
    //             printf("%sTime out when receiving image data%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //             break;
    //         }

    //         // Wait for data
    //         bpacket_t* packet = maple_get_next_bpacket_response();

    //         if (packet == NULL) {
    //             continue;
    //         }

    //         if (packet->request != WATCHDOG_BPK_R_COPY_FILE) {
    //             printf("Skipping packet with request %i\r\n", packet->request);
    //             continue;
    //         }

    //         time = clock();

    //         // Store data
    //         for (int i = 0; i < packet->numBytes; i++) {
    //             fputc(packet->bytes[i], testImage);
    //         }

    //         // Check whether the packet is the end of the data stream
    //         if (packet->code == BPACKET_CODE_SUCCESS) {

    //             // Close the file
    //             fclose(testImage);

    //             fileTransfered = TRUE;
    //         }
    //     }

    //     if (fileTransfered != TRUE) {
    //         failed = TRUE;
    //     }

    // } else {
    //     printf("%sSkipping file download. fopen() returned NULL%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Streaming */

    // FILE* streamImage;
    // if ((streamImage = fopen("streamImage.jpg", "wb")) != NULL) {

    //     maple_create_and_send_bpacket(WATCHDOG_BPK_R_STREAM_IMAGE, BPACKET_ADDRESS_ESP32, 0, NULL);

    //     uint8_t fileTransfered = FALSE;
    //     clock_t time           = clock();
    //     while (fileTransfered != TRUE) {

    //         if ((clock() - time) > 6000) {
    //             fclose(streamImage);
    //             printf("%sTime out when receiving image data%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //             break;
    //         }

    //         // Wait for data
    //         bpacket_t* packet = maple_get_next_bpacket_response();

    //         if ((packet == NULL) || (packet->request != WATCHDOG_BPK_R_STREAM_IMAGE)) {
    //             continue;
    //         }

    //         time = clock();

    //         // Store data
    //         for (int i = 0; i < packet->numBytes; i++) {
    //             fputc(packet->bytes[i], streamImage);
    //         }

    //         // Check whether the packet is the end of the data stream
    //         if (packet->code == BPACKET_CODE_SUCCESS) {

    //             // Close the file
    //             fclose(streamImage);

    //             fileTransfered = TRUE;
    //         }
    //     }

    //     if (fileTransfered != TRUE) {
    //         printf("FAiled\n");
    //         failed = TRUE;
    //     }

    // } else {
    //     printf("%sSkipping file download. fopen() returned NULL%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Recording Data */

    /* Test Listing all the files on the SD card */

    /* Test Getting the Status */

    /* Test that updating the resolution on the camera actually changes the resolution of the photos taken */

    if (failed == FALSE) {
        printf("%sAll tests passed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
    }
}