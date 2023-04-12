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
#include <time.h>    // Use for ms timer

/* C Library Includes for COM Port Interfacing */
#include <libserialport.h>

/* Personal Includes */
#include "chars.h"
#include "watchdog_defines.h"
#include "Bpacket.h"
#include "gui.h"
#include "datetime.h"
#include "bpacket.h"
#include "log.h"
#include "bpacket_utils.h"
#include
#include "cbuffer.h"

#define LOG_ERROR_CODE(code) (log_error("Error %s line %i. Code %i\r\n", __FILE__, __LINE__, code))
#define LOG_ERROR_MSG(msg)   (log_message("Error %s line %i: \'%s\'\r\n", __FILE__, __LINE__, msg))

#define MAPLE_MAX_ARGS     5
#define PACKET_BUFFER_SIZE 50

bpacket_circular_buffer_t mapleReceiveBuffer;
bpacket_circular_buffer_t mapleTransmitBuffer;

#define MAPLE_RECEIVE_BUFFER()  (mapleReceiveBuffer.buffer[*mapleReceiveBuffer.rIndex])
#define MAPLE_TRANSMIT_BUFFER() (mapleTransmitBuffer.buffer[*mapleTransmitBuffer.wIndex])

/* Example of how to get a list of serial ports on the system.
 *
 * This example file is released to the public domain. */

int check(enum sp_return result);
void maple_print_bpacket_data(bpk_packet_t* Bpacket);
void maple_create_and_send_bpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                   uint8_t numDataBytes, uint8_t* data);
void maple_create_and_send_sbpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                    char* string);
void maple_print_uart_response(void);
uint8_t maple_match_args(char** args, int numArgs);
void maple_command_line(void);
void maple_test(void);

uint8_t guiWriteIndex  = 0;
uint8_t guiReadIndex   = 0;
uint8_t mainWriteIndex = 0;
uint8_t mainReadIndex  = 0;

bpk_packet_t guiToMainBpackets[BPACKET_CIRCULAR_BUFFER_SIZE];
bpk_packet_t mainToGuiBpackets[BPACKET_CIRCULAR_BUFFER_SIZE];

uint32_t packetBufferIndex  = 0;
uint32_t packetPendingIndex = 0;
bpk_packet_t packetBuffer[PACKET_BUFFER_SIZE];

struct sp_port* gbl_activePort = NULL;

void print_data(uint8_t* data, uint8_t length) {
    for (int i = 0; i < length; i++) {
        printf("%c", data[i]);
    }
}

uint8_t maple_send_bpacket(bpk_packet_t* Bpacket) {

    bpk_buffer_t packetBuffer;
    bpacket_to_buffer(Bpacket, &packetBuffer);

    if (sp_blocking_write(gbl_activePort, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
        return FALSE;
    }

    return TRUE;
}

bpk_packet_t* maple_get_next_bpacket_response(void) {

    if (packetPendingIndex == packetBufferIndex) {
        return NULL;
    }

    bpk_packet_t* buffer = &packetBuffer[packetPendingIndex];
    bpacket_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
    return buffer;
}

void maple_create_and_send_bpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                   uint8_t numDataBytes, uint8_t* data) {
    bpk_packet_t Bpacket;
    if (bp_create_packet(&Bpacket, Receiver, BPK_Addr_Send_Maple, Request, BPK_Code_Execute,
                         numDataBytes, data) != TRUE) {
        log_error("Error %s %i. Code %i", __FILE__, __LINE__, Bpacket.ErrorCode.val);
        return;
    }

    if (maple_send_bpacket(&Bpacket) != TRUE) {
        return;
    }
}

void maple_create_and_send_sbpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                    char* string) {
    bpk_packet_t Bpacket;

    if (bp_create_string_packet(&Bpacket, Receiver, BPK_Addr_Send_Maple, Request, BPK_Code_Execute,
                                string) != TRUE) {
        log_error("Error %s %i. Code %i\r\n", __FILE__, __LINE__, Bpacket.ErrorCode.val);
        return;
    }

    if (maple_send_bpacket(&Bpacket) != TRUE) {
        return;
    }
}

void maple_print_bpacket_data(bpk_packet_t* Bpacket) {

    if (Bpacket->Request.val != BPK_Request_Message.val) {
        log_message("Request: %i Len: %i Data: ", Bpacket->Request.val, Bpacket->Data.numBytes);
    }

    for (int i = 0; i < Bpacket->Data.numBytes; i++) {
        log_message("%c", Bpacket->Data.bytes[i]);
    }
}

void maple_print_uart_response(void) {

    int packetsFinished = FALSE;
    while (packetsFinished == FALSE) {

        // Wait until the packet is ready
        while (packetPendingIndex == packetBufferIndex) {}

        bpk_packet_t* Bpacket = maple_get_next_bpacket_response();

        // Packet ready. Print its contents
        for (int i = 0; i < Bpacket->Data.numBytes; i++) {
            log_message("%c", Bpacket->Data.bytes[i]);
        }

        if (Bpacket->Request.val == BPK_Code_Success.val) {
            packetsFinished = TRUE;
        }
    }
}

int maple_read_port(void* buf, size_t count, unsigned int timeout_ms) {

    if (gbl_activePort == NULL) {
        return 0;
    }

    return sp_blocking_read(gbl_activePort, buf, count, timeout_ms);
}

DWORD WINAPI maple_listen_rx(void* arg) {

    if (sp_get_port_by_name("COM12", &gbl_activePort) != SP_OK) {
        log_error("Failed to find port 12\n");
        return FALSE;
    }

    enum sp_return result = sp_open(gbl_activePort, SP_MODE_READ_WRITE);
    if (result != SP_OK) {
        log_error("Failed to connect to port 12\n");
        return FALSE;
    }

    // Configure the port settings for communication
    sp_set_baudrate(gbl_activePort, 115200);
    sp_set_bits(gbl_activePort, 8);
    sp_set_parity(gbl_activePort, SP_PARITY_NONE);
    sp_set_stopbits(gbl_activePort, 1);
    sp_set_flowcontrol(gbl_activePort, SP_FLOWCONTROL_NONE);

    uint8_t byte, expectedByte, numBytes;
    cbuffer_t ByteBuffer;
    uint8_t byteBuffer[8];
    bpk_utils_init_expected_byte_buffer(byteBuffer);
    cbuffer_init(&ByteBuffer, (void*)byteBuffer, sizeof(uint8_t), 8);
    bpk_packet_t Bpacket = {0};
    uint8_t bi           = 0;

    while ((numBytes = maple_read_port(&byte, 1, 0)) >= 0) {

        // Read the current index of the byte buffer and store the result into
        // expected byte
        cbuffer_read_current_byte(&ByteBuffer, (void*)&expectedByte);

        switch (expectedByte) {

            /* Expecting bpacket 'data' bytes */
            case BPK_BYTE_DATA:

                // Add byte to bpacket
                Bpacket.Data.bytes[bi++] = byte;

                // Skip resetting if there are more date bytes to read
                if (bi < Bpacket.Data.numBytes) {
                    continue;
                }

                // No more data bytes to read. Execute request and then exit switch statement
                // to reset
                log_success("Bpacket received. Bytes read %i %i %i\n", Bpacket.Data.numBytes,
                            Bpacket.Data.bytes[0], Bpacket.Data.bytes[Bpacket.Data.numBytes - 1]);

                break;

            /* Expecting bpacket 'length' byte */
            case BPK_BYTE_LENGTH:

                // Store the length and updated the expected byte
                Bpacket.Data.numBytes = byte;
                cbuffer_increment_read_index(&ByteBuffer);

                // Skip reseting if there is data to be read
                if (Bpacket.Data.numBytes > 0) {
                    continue;
                }

                // No data in Bpacket. Execute request and exit switch statement to reset
                log_success("Bpacket received No data. Bytes read %i\n", Bpacket.Data.numBytes);

                break;

            /* Expecting another bpacket byte that is not 'data' or 'length' */
            default:

                // If decoding the byte succeeded, move to the next expected byte by
                // incrementing the circrular buffer holding the expected bytes
                if (bpk_utils_decode_non_data_byte(&Bpacket, expectedByte, byte) == TRUE) {
                    cbuffer_increment_read_index(&ByteBuffer);
                    continue;
                }

                // Decoding failed, print error
                log_error("BPK Read failed. %i %i\n", expectedByte, byte);
        }

        // Reset the expected byte back to the start and bpacket buffer index
        cbuffer_reset_read_index(&ByteBuffer);
        bi = 0;
    }

    log_error("Connection terminated\n");
}

uint8_t maple_connect_to_device(bpk_addr_receive_t receiver, uint8_t pingCode) {

    // Create a struct to hold all the COM ports currently in use
    struct sp_port** port_list;

    if (sp_list_ports(&port_list) != SP_OK) {
        LOG_ERROR_MSG("Failed to list ports");
        return 0;
    }

    // Loop through all the ports
    uint8_t portFound = FALSE;

    for (int i = 0; port_list[i] != NULL; i++) {

        gbl_activePort = port_list[i];

        // Open the port
        enum sp_return result = sp_open(gbl_activePort, SP_MODE_READ_WRITE);
        if (result != SP_OK) {
            return result;
        }

        // Configure the port settings for communication
        sp_set_baudrate(gbl_activePort, 115200);
        sp_set_bits(gbl_activePort, 8);
        sp_set_parity(gbl_activePort, SP_PARITY_NONE);
        sp_set_stopbits(gbl_activePort, 1);
        sp_set_flowcontrol(gbl_activePort, SP_FLOWCONTROL_NONE);

        // Send a ping
        bpk_packet_t Bpacket;
        bpk_buffer_t bpacketBuffer;
        bp_create_packet(&Bpacket, receiver, BPK_Addr_Send_Maple, BPK_Request_Ping,
                         BPK_Code_Execute, 0, NULL);
        bpacket_to_buffer(&Bpacket, &bpacketBuffer);

        if (sp_blocking_write(gbl_activePort, bpacketBuffer.buffer, bpacketBuffer.numBytes, 100) <
            0) {
            printf("Unable to write\n");
            continue;
        }

        // Response may include other incoming messages as well, not just a response to a ping.
        // Create a timeout of 2 seconds and look for response from ping

        clock_t startTime = clock();
        while ((clock() - startTime) < 200) {

            // Wait for Bpacket to be received
            while (packetPendingIndex != packetBufferIndex) {

                // Confirm the request is valid
                if (packetBuffer[packetPendingIndex].Request.val == BPK_Request_Ping.val) {

                    // Confirm the ping code was correct
                    if (packetBuffer[packetPendingIndex].Data.bytes[0] ==
                        WATCHDOG_PING_CODE_STM32) {
                        portFound = TRUE;
                    }
                }

                bpacket_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
            }
        }

        // Close port and check next port if this was not the correct port
        if (portFound != TRUE) {
            sp_close(gbl_activePort);
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
        log_error("Could not open file\n");
        fclose(target);
        return FALSE;
    }

    // Send command to copy file. Keeping reading until no more data to send across
    maple_create_and_send_bpacket(BPK_Req_Stream_Images, BPK_Addr_Receive_Esp32, 0, NULL);

    int packetsFinished = FALSE;
    clock_t startTime;
    while (packetsFinished == FALSE) {

        // Wait until the packet is ready
        startTime = clock();
        while (packetPendingIndex == packetBufferIndex) {

            if ((clock() - startTime) > 6000) {
                log_error("Timeout 1000ms\n");
                return FALSE;
            }
        }

        if (packetBuffer[packetPendingIndex].Request.val == BPK_Request_Message.val) {
            bpacket_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
            continue;
        }

        if (packetBuffer[packetPendingIndex].Request.val != BPK_Code_In_Progress.val &&
            packetBuffer[packetPendingIndex].Request.val != BPK_Code_Success.val) {
            LOG_ERROR_CODE(packetBuffer[packetPendingIndex].Request.val);
            fclose(target);
            return FALSE;
        }

        if (packetBuffer[packetPendingIndex].Request.val == BPK_Code_Success.val) {
            packetsFinished = TRUE;
        }

        for (int i = 0; i < packetBuffer[packetPendingIndex].Data.numBytes; i++) {
            fputc(packetBuffer[packetPendingIndex].Data.bytes[i], target);
        }

        bpacket_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
    }

    fclose(target);

    return TRUE;
}

void maple_test_bpackets(void) {

    if (sp_get_port_by_name("COM12", &gbl_activePort) != SP_OK) {
        log_error("Failed to find port 12\n");
        return;
    }

    enum sp_return result = sp_open(gbl_activePort, SP_MODE_READ_WRITE);
    if (result != SP_OK) {
        log_error("Failed to connect to port 12\n");
        return;
    }

    // Configure the port settings for communication
    sp_set_baudrate(gbl_activePort, 115200);
    sp_set_bits(gbl_activePort, 8);
    sp_set_parity(gbl_activePort, SP_PARITY_NONE);
    sp_set_stopbits(gbl_activePort, 1);
    sp_set_flowcontrol(gbl_activePort, SP_FLOWCONTROL_NONE);

    // Test reading bpackets
    // uint8_t byte, expectedByte, numBytes;
    // cbuffer_t ByteBuffer;
    // uint8_t byteBuffer[8];
    // bpk_utils_init_expected_byte_buffer(byteBuffer);
    // cbuffer_init(&ByteBuffer, byteBuffer, 8);
    // bpk_packet_t Bpacket = {0};
    // uint8_t bi           = 0;

    // while ((numBytes = maple_read_port(&byte, 1, 0)) >= 0) {

    //     // Read the current index of the byte buffer and store the result into
    //     // expected byte
    //     cbuffer_read_current_byte(&ByteBuffer, &expectedByte);

    //     switch (expectedByte) {

    //         /* Expecting bpacket 'data' bytes */
    //         case BPK_BYTE_DATA:

    //             // Add byte to bpacket
    //             Bpacket.Data.bytes[bi++] = byte;

    //             // Skip resetting if there are more date bytes to read
    //             if (bi < Bpacket.Data.numBytes) {
    //                 continue;
    //             }

    //             // No more data bytes to read. Execute request and then exit switch statement
    //             // to reset
    //             log_success("Bpacket received. Bytes read %i %i %i\n", Bpacket.Data.numBytes,
    //                         Bpacket.Data.bytes[0], Bpacket.Data.bytes[Bpacket.Data.numBytes -
    //                         1]);

    //             break;

    //         /* Expecting bpacket 'length' byte */
    //         case BPK_BYTE_LENGTH:

    //             // Store the length and updated the expected byte
    //             Bpacket.Data.numBytes = byte;
    //             cbuffer_increment_read_index(&ByteBuffer);

    //             // Skip reseting if there is data to be read
    //             if (Bpacket.Data.numBytes > 0) {
    //                 continue;
    //             }

    //             // No data in Bpacket. Execute request and exit switch statement to reset
    //             log_success("Bpacket received No data. Bytes read %i\n", Bpacket.Data.numBytes);

    //             break;

    //         /* Expecting another bpacket byte that is not 'data' or 'length' */
    //         default:

    //             // If decoding the byte succeeded, move to the next expected byte by
    //             // incrementing the circrular buffer holding the expected bytes
    //             if (bpk_utils_decode_non_data_byte(&Bpacket, expectedByte, byte) == TRUE) {
    //                 cbuffer_increment_read_index(&ByteBuffer);
    //                 continue;
    //             }

    //             // Decoding failed, print error
    //             log_error("BPK Read failed. %i %i\n", expectedByte, byte);
    //     }

    //     // Reset the expected byte back to the start and bpacket buffer index
    //     cbuffer_reset_read_index(&ByteBuffer);
    //     bi = 0;
    // }

    // log_error("Connection terminated\n");
    // // Send a ping
    // bpk_packet_t Bpacket;
    // bpk_buffer_t bpacketBuffer;
    // bp_create_packet(&Bpacket, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Request_Ping,
    //                  BPK_Code_Execute, 0, NULL);
    // bpacket_to_buffer(&Bpacket, &bpacketBuffer);

    // if (sp_blocking_write(gbl_activePort, bpacketBuffer.buffer, bpacketBuffer.numBytes, 100) < 0)
    // {
    //     log_error("Unable to write\n");
    //     return;
    // }

    // log_success("Written message\r\n");
}

int main(int argc, char** argv) {

    // Initialise logging
    log_init(printf, print_data);

    // maple_test_bpackets();

    // while (1) {}

    HANDLE thread = CreateThread(NULL, 0, maple_listen_rx, NULL, 0, NULL);

    if (!thread) {
        log_error("Thread failed\n");
        return 0;
    }

    while (1) {}

    // Try connect to the device
    if (maple_connect_to_device(BPK_Addr_Receive_Stm32, WATCHDOG_PING_CODE_STM32) != TRUE) {
        log_error("Unable to connect to device. Check the COM port is not already in use\n");
        TerminateThread(thread, 0);
        return FALSE;
    }

    log_success("Connected to port %s\n", sp_get_port_name(gbl_activePort));

    maple_test();

    bpacket_circular_buffer_t mapleReceiveBuffer;
    bpacket_create_circular_buffer(&mapleReceiveBuffer, &guiWriteIndex, &mainReadIndex,
                                   &guiToMainBpackets[0]);

    bpacket_circular_buffer_t mapleTransmitBuffer;
    bpacket_create_circular_buffer(&mapleTransmitBuffer, &mainWriteIndex, &guiReadIndex,
                                   &mainToGuiBpackets[0]);

    watchdog_info_t watchdogInfo;
    watchdogInfo.id               = packetBuffer[packetPendingIndex].Data.bytes[0];
    watchdogInfo.cameraResolution = packetBuffer[packetPendingIndex].Data.bytes[1];
    watchdogInfo.numImages        = (packetBuffer[packetPendingIndex].Data.bytes[2] << 8) |
                             packetBuffer[packetPendingIndex].Data.bytes[3];
    watchdogInfo.status = (packetBuffer[packetPendingIndex].Data.bytes[4] == 0)
                              ? SYSTEM_STATUS_OK
                              : SYSTEM_STATUS_ERROR;
    sprintf(watchdogInfo.datetime, "01/03/2022 9:15 AM");

    // packetPendingIndex++;

    uint32_t flags = 0;
    // uint8_t cameraView = FALSE;

    gui_initalisation_t guiInit;
    guiInit.watchdog  = &watchdogInfo;
    guiInit.flags     = &flags;
    guiInit.guiToMain = &mapleReceiveBuffer;
    guiInit.mainToGui = &mapleTransmitBuffer;

    guiInit.guiToMain = &mapleReceiveBuffer;
    guiInit.mainToGui = &mapleTransmitBuffer;

    HANDLE guiThread = CreateThread(NULL, 0, gui, &guiInit, 0, NULL);

    uint8_t startOfImgData = TRUE;
    FILE* streamImage      = NULL;

    if (!guiThread) {
        printf("Thread failed\n");
        return 0;
    }

    while (1) {
        // If a Bpacket is recieved from the Gui, deal with it in here
        if (*mapleReceiveBuffer.rIndex != *mapleReceiveBuffer.wIndex) {
            uint8_t sendStatus = maple_send_bpacket(MAPLE_RECEIVE_BUFFER());
            if (sendStatus != TRUE) {
                printf("Error %s %i\r\n", __FILE__, __LINE__);
            }

            bpacket_increment_circular_buffer_index(mapleReceiveBuffer.rIndex);
        }

        // Close thread and exit program if GUI closes
        if ((flags & GUI_CLOSE) != 0) {
            TerminateThread(thread, 0);
            break;
        }

        // If their is a Bpacket put into the packet buffer, it gets put in the main-to-gui circular
        // buffer

        if (packetBufferIndex != packetPendingIndex) {

            bpk_packet_t* receivedBpacket = maple_get_next_bpacket_response();

            if (receivedBpacket->Request.val == BPK_Request_Message.val) {
                continue;
            }

            if (receivedBpacket->Request.val == BPK_Req_Stream_Images.val) {
                if (startOfImgData == TRUE) {
                    startOfImgData = FALSE;

                    if ((streamImage = fopen(CAMERA_VIEW_FILENAME, "wb")) == NULL) {
                        // TODO: make this do something proper
                        printf("Couldnt open stream file");
                    }
                    for (int i = 0; i < receivedBpacket->Data.numBytes; i++) {
                        fputc(receivedBpacket->Data.bytes[i], streamImage);
                    }
                } else if (receivedBpacket->Code.val == BPK_Code_Success.val) {
                    startOfImgData = TRUE;
                    for (int i = 0; i < receivedBpacket->Data.numBytes; i++) {
                        fputc(receivedBpacket->Data.bytes[i], streamImage);
                    }
                    fclose(streamImage);
                    // Create Bpacket to update the thing
                    bp_create_packet(MAPLE_TRANSMIT_BUFFER(), BPK_Addr_Receive_Maple,
                                     BPK_Addr_Send_Maple, BPK_Req_Stream_Images, BPK_Code_Execute,
                                     0, NULL);
                    bpacket_increment_circular_buffer_index(mapleTransmitBuffer.wIndex);
                } else if (receivedBpacket->Code.val == BPK_Code_In_Progress.val) {
                    for (int i = 0; i < receivedBpacket->Data.numBytes; i++) {
                        fputc(receivedBpacket->Data.bytes[i], streamImage);
                    }
                }
                continue;
            }

            MAPLE_TRANSMIT_BUFFER() = receivedBpacket;

            // Increae write index of the main to gui circular buffer so that it can be parsed to
            // the GUI
            bpacket_increment_circular_buffer_index(mapleTransmitBuffer.wIndex);
        }
    }

    return 0;
}

uint8_t maple_response_is_valid(bpk_request_t ExpectedRequest, uint16_t timeout) {

    // Print response
    clock_t startTime = clock();
    while ((clock() - startTime) < timeout) {

        bpk_packet_t* Bpacket = maple_get_next_bpacket_response();

        // If there is next response yet then skip
        if (Bpacket == NULL) {
            continue;
        }

        // Skip if the Bpacket is just a message
        if (Bpacket->Request.val == BPK_Request_Message.val) {
            continue;
        }

        // Confirm the received Bpacket matches the correct request and code
        if (Bpacket->Request.val != ExpectedRequest.val) {
            // printf("Rec: %i Snd: %i Req: %i Code: %i num bytes: %i\r\n", Bpacket->Receiver.val,
            // Bpacket->sender,
            //        Bpacket->Request, Bpacket->Code, Bpacket->Data.numBytes);
            return FALSE;
        }

        if (Bpacket->Code.val != BPK_Code_Success.val) {
            printf("Expected code %i but got %i\n", BPK_Code_Success.val, Bpacket->Request.val);
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
                maple_create_and_send_bpacket(BPK_Request_Help, BPK_Addr_Receive_Stm32, 0, NULL);
                return TRUE;
            }

            if (chars_same(args[0], "ping\0") == TRUE) {
                uint8_t ping = WATCHDOG_PING_CODE_STM32;
                maple_create_and_send_bpacket(BPK_Request_Ping, BPK_Addr_Receive_Esp32, 1, &ping);
                if (maple_response_is_valid(BPK_Request_Ping, 1000) == TRUE) {
                    printf("%sCommand executed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
                } else {
                    printf("%sCommand failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
                }
                return TRUE;
            }

            if (chars_same(args[0], "clc\0")) {
                printf(ASCII_CLEAR_SCREEN);
                return TRUE;
            }

            if (chars_same(args[0], "ls\0") == TRUE) {
                maple_create_and_send_sbpacket(BPK_Req_List_Dir, BPK_Addr_Receive_Esp32, "\0");
                return TRUE;
            }

            if (chars_same(args[0], "photo\0") == TRUE) {
                maple_create_and_send_bpacket(BPK_Req_Take_Photo, BPK_Addr_Receive_Stm32, 0, NULL);
                if (maple_response_is_valid(BPK_Req_Take_Photo, 10000) == TRUE) {
                    printf("%sCommand executed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
                } else {
                    printf("%sCommand failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
                }

                return TRUE;
            }

            if (chars_same(args[0], "settings\0") == TRUE) {
                maple_create_and_send_bpacket(BPK_Req_Get_Camera_Capture_Times,
                                              BPK_Addr_Receive_Stm32, 0, NULL);
                if (maple_response_is_valid(BPK_Req_Get_Camera_Capture_Times, 3000) == TRUE) {
                    printf("%sCommand executed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
                } else {
                    printf("%sCommand failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
                }

                return TRUE;
            }

            break;

        case 2:

            if (chars_same(args[0], "ls\0") == TRUE) {
                maple_create_and_send_sbpacket(BPK_Req_List_Dir, BPK_Addr_Receive_Esp32, args[1]);
                return TRUE;
            }

            if (chars_same(args[0], "cpy\0") == TRUE) {
                maple_create_and_send_sbpacket(BPK_Req_Copy_File, BPK_Addr_Receive_Esp32, args[1]);
                return TRUE;
            }

            break;

        case 3:

            if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
                chars_same(args[2], "on\0") == TRUE) {
                maple_create_and_send_bpacket(BPK_Req_Led_Red_On, BPK_Addr_Receive_Esp32, 0, NULL);
                if (maple_response_is_valid(BPK_Req_Led_Red_On, 2000) == TRUE) {
                    printf("%sCommand executed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
                } else {
                    printf("%sCommand failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
                }

                return TRUE;
            }

            if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
                chars_same(args[2], "off\0") == TRUE) {
                maple_create_and_send_bpacket(BPK_Req_Led_Red_Off, BPK_Addr_Receive_Esp32, 0, NULL);
                if (maple_response_is_valid(BPK_Req_Led_Red_Off, 2000) == TRUE) {
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

uint8_t maple_get_response(bpk_packet_t** Bpacket, bpk_request_t Request, uint16_t timeout) {

    // Print response
    clock_t startTime = clock();
    while ((clock() - startTime) < timeout) {

        *Bpacket = maple_get_next_bpacket_response();

        // If there is next response yet then skip
        if ((*Bpacket != NULL) && ((*Bpacket)->Request.val == Request.val)) {
            return TRUE;
        }

        if ((*Bpacket) != NULL) {
            char info[80];
            bpacket_get_info(*Bpacket, info);
            printf(info);
        }
    }

    return FALSE;
}

uint8_t maple_restart_esp32(void) {

    // Turn the ESP32 off. Wait for 200ms. Turn the ESP32 back on. Wait for 1000ms. This ensures
    // that when we get the capture time settings from the esp32 we can know whether the capture
    // time settings set above were saved to the SD card or not
    bpk_packet_t* stateBpacket;
    maple_create_and_send_bpacket(BPK_Req_Turn_Off, BPK_Addr_Receive_Stm32, 0, NULL);
    if (maple_get_response(&stateBpacket, BPK_Req_Turn_Off, 1000) != TRUE) {
        printf("%sTurning the ESP32 off failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        return FALSE;
    }

    clock_t time = clock();
    while ((clock() - time) < 200) {}

    // Delay of 1500ms because the STM32 will automatically delay for 1s when turning the esp32
    // so it has enough time to boot up
    maple_create_and_send_bpacket(BPK_Req_Turn_On, BPK_Addr_Receive_Stm32, 0, NULL);
    if (maple_get_response(&stateBpacket, BPK_Req_Turn_On, 2000) != TRUE) {
        printf("%sTurning the ESP32 on failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        return FALSE;
    }

    return TRUE;
}

void maple_test(void) {

    bpk_packet_t Bpacket;
    uint8_t failed = FALSE;
    char msg[100];

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    if (FALSE) {
        /****** START CODE BLOCK ******/
        // Description: Testing turning on and off the red LED

        /* Turn the red LED on */
        maple_create_and_send_bpacket(BPK_Req_Led_Red_On, BPK_Addr_Receive_Stm32, 0, NULL);
        if (maple_response_is_valid(BPK_Req_Led_Red_On, 2000) == FALSE) {
            LOG_ERROR_MSG("Failed to turn red LED on");
            failed = TRUE;
        }

        /* Turn the red LED off */
        maple_create_and_send_bpacket(BPK_Req_Led_Red_Off, BPK_Addr_Receive_Stm32, 0, NULL);
        if (maple_response_is_valid(BPK_Req_Led_Red_Off, 2000) == FALSE) {
            LOG_ERROR_MSG("Failed to turn red LED off");
            failed = TRUE;
        }

        /* Turn the red LED on directly */
        maple_create_and_send_bpacket(BPK_Req_Led_Red_On, BPK_Addr_Receive_Esp32, 0, NULL);
        if (maple_response_is_valid(BPK_Req_Led_Red_On, 2000) == FALSE) {
            LOG_ERROR_MSG("Failed to turn red LED on");
            failed = TRUE;
        }

        /* Turn the red LED off directly */
        maple_create_and_send_bpacket(BPK_Req_Led_Red_Off, BPK_Addr_Receive_Esp32, 0, NULL);
        if (maple_response_is_valid(BPK_Req_Led_Red_Off, 2000) == FALSE) {
            LOG_ERROR_MSG("Failed to turn red LED off");
            failed = TRUE;
        }

        if (failed == FALSE) {
            log_success("LED tests passed\n");
        }
        /****** END CODE BLOCK ******/
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    if (FALSE) {
        /****** START CODE BLOCK ******/
        // Description:

        /* Set the datetime on the STM32 */
        dt_datetime_t NewDatetime;
        dt_time_init(&NewDatetime.Time, 30, 15, 8);
        dt_date_init(&NewDatetime.Date, 1, 2, 2023);
        wd_datetime_to_bpacket(&Bpacket, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple,
                               BPK_Req_Set_Datetime, BPK_Code_Execute, &NewDatetime);
        maple_send_bpacket(&Bpacket);

        bpk_packet_t* DateTimeBpacket;
        if (maple_get_response(&DateTimeBpacket, BPK_Req_Set_Datetime, 1000) != TRUE) {
            LOG_ERROR_MSG("Setting the datetime from the STM32 failed\n");
            failed = TRUE;
        }

        if (bpk_utils_confirm_params(DateTimeBpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32,
                                     BPK_Req_Set_Datetime, BPK_Code_Success, 0) != TRUE) {
            LOG_ERROR_MSG("Setting the datetime on the STM failed");
            failed = TRUE;
        }

        /* Get the datetime from the STM32 */
        maple_create_and_send_bpacket(BPK_Req_Get_Datetime, BPK_Addr_Receive_Stm32, 0, NULL);
        if (maple_get_response(&DateTimeBpacket, BPK_Req_Get_Datetime, 1000) != TRUE) {
            LOG_ERROR_MSG("Getting the datetime from the STM32 failed\n");
            failed = TRUE;
        }

        if (bpk_utils_confirm_params(DateTimeBpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32,
                                     BPK_Req_Get_Datetime, BPK_Code_Success, 7) != TRUE) {
            LOG_ERROR_MSG("Getting the datetime from the STM32 failed\n");
            failed = TRUE;
        } else {

            dt_datetime_t Datetime;
            if (wd_bpacket_to_datetime(DateTimeBpacket, &Datetime) != TRUE) {
                LOG_ERROR_CODE(DateTimeBpacket->ErrorCode.val);
                failed = TRUE;
            } else {
                uint8_t result = 0;
                result |= (Datetime.Time.hour != NewDatetime.Time.hour);
                result |= (Datetime.Date.day != NewDatetime.Date.day);
                result |= (Datetime.Date.month != NewDatetime.Date.month);
                result |= (Datetime.Date.year != NewDatetime.Date.year);

                if (result != 0) {
                    LOG_ERROR_MSG("Incorrect datetime retrieved\n");
                    failed = TRUE;
                }
            }
        }

        if (failed == FALSE) {
            log_success("Setting and getting datetime passed\n");
        }

        /****** END CODE BLOCK ******/
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Setting the Camera Settings */

    if (TRUE) {
        /****** START CODE BLOCK ******/
        // Description: Testing reading and writing camera settings

        cdt_u8_t NewCameraSettings = {.value = WD_CAM_RES_320x240};
        if (wd_camera_settings_to_bpk(&Bpacket, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple,
                                      BPK_Req_Set_Camera_Settings, BPK_Code_Execute,
                                      &NewCameraSettings) != TRUE) {
            LOG_ERROR_CODE(Bpacket.ErrorCode.val);
            failed = TRUE;
        } else {
            maple_send_bpacket(&Bpacket);
        }

        log_message("Camera settings %i\r\n", Bpacket.Data.bytes[0]);
        // while (1) {}

        bpk_packet_t* Response;
        if (maple_get_response(&Response, BPK_Req_Set_Camera_Settings, 1000) != TRUE) {
            log_error("Updating camera settings failed");
            failed = TRUE;
        }

        // Read the camera settings and confirm its the same as the one that was set
        if (bpk_utils_confirm_params(Response, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32,
                                     BPK_Req_Set_Camera_Settings, BPK_Code_Success, 0) != TRUE) {
            LOG_ERROR_CODE(Response->ErrorCode.val);
            LOG_ERROR_MSG(Response->Data.bytes);
            failed = TRUE;
        }

        // Restart the esp32. This will allow maple to determine whether the settings were properly
        // saved onto the SD card or not
        if (maple_restart_esp32() != TRUE) {
            log_error("Maple failed to restart ESP32\n");
            failed = TRUE;
        }

        bpk_reset(&Bpacket);

        maple_create_and_send_bpacket(BPK_Req_Get_Camera_Settings, BPK_Addr_Receive_Esp32, 0, NULL);
        if (maple_get_response(&Response, BPK_Req_Get_Camera_Settings, 1000) != TRUE) {
            printf("%sGetting the camera settings failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
            failed = TRUE;
        }

        // Confirm the received Bpacket has the expected values
        if (bpk_utils_confirm_params(Response, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32,
                                     BPK_Req_Get_Camera_Settings, BPK_Code_Success, 1) != TRUE) {
            printf("%sUnexpected response when updating camera settings. %s%s\n", ASCII_COLOR_RED,
                   msg, ASCII_COLOR_WHITE);
            failed = TRUE;
        }

        // Convert Bpacket back to camera settings
        cdt_u8_t CameraSettings;
        if (wd_bpk_to_camera_settings(Response, &CameraSettings) != TRUE) {
            LOG_ERROR_MSG("Bpacket to camera settings failed");
            failed = TRUE;
        } else {

            // Compare the received camera settings with the camera settings that were set
            // previosuly
            if (CameraSettings.value != NewCameraSettings.value) {
                log_error("Incorrect camera settings. Found %i but expected %i\n",
                          CameraSettings.value, NewCameraSettings.value);
                failed = TRUE;
            }
        }

        if (failed == FALSE) {
            log_success("%sCamera settings tests passed\n%s", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
        }
        /****** END CODE BLOCK ******/
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

    // result = wd_capture_time_settings_to_bpacket(
    //     &Bpacket, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Req_Set_Camera_Capture_Times,
    //     BPK_Code_Execute, &newCaptureTime);
    // if (result != TRUE) {
    //     wd_get_error(result, msg);
    //     printf("%sFailed to convert capture time settings to Bpacket. %s\n%s", ASCII_COLOR_RED,
    //     msg,
    //            ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // } else {
    //     maple_send_bpacket(&Bpacket);
    // }

    // bpk_packet_t* newCaptureTimeSettingsBpacket;
    // // Internally the way the capture time settings is updated on the STM32, you are actually
    // // expecting a BPK_Req_Get_Camera_Capture_Times request back instead of a
    // // BPK_Req_Set_Camera_Capture_Times
    // if (maple_get_response(&newCaptureTimeSettingsBpacket, BPK_Req_Set_Camera_Capture_Times,
    //                        1000) != TRUE) {
    //     printf("%sSetting the capture time failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // // Confirm the received Bpacket has the expected values
    // // Internally the way the capture time settings is updated on the STM32, you are actually
    // // expecting a BPK_Req_Get_Camera_Capture_Times request back instead of a
    // // BPK_Req_Set_Camera_Capture_Times
    // if (bpk_utils_confirm_params(newCaptureTimeSettingsBpacket, BPK_Addr_Receive_Maple,
    //                        BPK_Addr_Send_Stm32, BPK_Req_Set_Camera_Capture_Times,
    //                        BPK_Code_Success, 0, msg) != TRUE) {
    //     printf("%sUnexpected response when updating capture time settings. %s%s\n",
    //     ASCII_COLOR_RED,
    //            msg, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // if (maple_restart_esp32() != TRUE) {
    //     printf("%sMaple failed to restart ESP32%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // /* Test Getting the Watchdog Capture Time from the STM32 */
    // bpk_packet_t* captureTimeSettingsBpacket;
    // maple_create_and_send_bpacket(BPK_Req_Get_Camera_Capture_Times, BPK_Addr_Receive_Stm32, 0,
    //                               NULL);
    // if (maple_get_response(&captureTimeSettingsBpacket, BPK_Req_Get_Camera_Capture_Times, 1000)
    // !=
    //     TRUE) {
    //     printf("%sGetting the capture time settings from the STM32 failed%s\n", ASCII_COLOR_RED,
    //            ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // if (bpk_utils_confirm_params(captureTimeSettingsBpacket, BPK_Addr_Receive_Maple,
    // BPK_Addr_Send_Stm32,
    //                        BPK_Req_Get_Camera_Capture_Times, BPK_Code_Success, 6, msg) != TRUE) {
    //     printf("%sUnexpected response when getting the capture time settings from STM32. %s%s\n",
    //            ASCII_COLOR_RED, msg, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // /* Test Getting the Watchdog Capture Time from the ESP32 */
    // maple_create_and_send_bpacket(BPK_Req_Get_Camera_Capture_Times, BPK_Addr_Receive_Esp32, 0,
    //                               NULL);
    // if (maple_get_response(&captureTimeSettingsBpacket, BPK_Req_Get_Camera_Capture_Times, 1000)
    // !=
    //     TRUE) {
    //     printf("%sGetting the capture time settings from ESP32 failed%s\n", ASCII_COLOR_RED,
    //            ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // if (bpk_utils_confirm_params(captureTimeSettingsBpacket, BPK_Addr_Receive_Maple,
    // BPK_Addr_Send_Esp32,
    //                        BPK_Req_Get_Camera_Capture_Times, BPK_Code_Success, 6, msg) != TRUE) {
    //     printf("%sUnexpected response when getting capture time settings from ESP32. %s%s\n",
    //            ASCII_COLOR_RED, msg, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // if (failed == FALSE) {
    //     printf("%sCapture time settings tests passed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Taking a Photo */
    // bpk_packet_t* photoBpacket;
    // maple_create_and_send_bpacket(BPK_Req_Take_Photo, BPK_Addr_Receive_Stm32, 0, NULL);
    // if (maple_get_response(&photoBpacket, WATCHDOG_BPK_R_TAKE_PHOTO, 5000) != TRUE) {
    //     printf("%sTaking a photo failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // }

    // if (bpk_utils_confirm_params(photoBpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32,
    // WATCHDOG_BPK_R_TAKE_PHOTO,
    //                            BPK_Code_Success, 0, msg) != TRUE) {
    //     printf("%sUnexpected response when taking a photo. %s%s\n", ASCII_COLOR_RED, msg,
    //     ASCII_COLOR_WHITE); failed = TRUE;
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // while (1) {}
    /* Test listing the directory */
    // bpk_packet_t* dirBpacket;
    // bpacket_create_sp(&Bpacket, BPK_Addr_Send_Esp32, BPK_Addr_Send_Maple,
    // WATCHDOG_BPK_R_LIST_DIR,
    //                   BPK_Code_Execute, DATA_FOLDER_PATH);
    // maple_send_bpacket(&Bpacket);
    // if (maple_get_response(&dirBpacket, WATCHDOG_BPK_R_LIST_DIR, 3000) != TRUE) {
    //     printf("%sListing directory failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
    //     failed = TRUE;
    // } else {
    //     for (int i = 0; i < dirBpacket->Data.numBytes; i++) {
    //         printf("%c", dirBpacket->bytes[i]);
    //     }
    //     printf("\n");
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test downloading photo that was just taken */

    // Extract the first image name from the Bpacket data
    // char imageFileName[30];

    // int i            = 0;
    // imageFileName[0] = '\0';
    // while (chars_contains(imageFileName, ".jpg") != TRUE) {

    //     int j;
    //     for (j = 0; dirBpacket->bytes[i] != '\n'; j++) {

    //         if (dirBpacket->bytes[i] != '\r') {
    //             imageFileName[j] = dirBpacket->bytes[i];
    //         }

    //         i++;
    //     }

    //     imageFileName[j] = '\0';
    //     i++;

    //     if (i >= dirBpacket->Data.numBytes) {
    //         break;
    //     }
    // }

    // printf("Image to copy: %s\n", imageFileName);

    // if (chars_contains(imageFileName, ".jpg") != TRUE) {
    //     printf("Failed\r\n");
    //     failed = TRUE;
    // } else {

    //     /* Test Copying a File by downloading the image that was just taken */

    //     FILE* testImage;
    //     if ((testImage = fopen("testImage.jpg", "wb")) != NULL) {

    //         // Send request to ESP32 to copy file

    //         // Append the data folder to the image
    //         char fileName[50];
    //         sprintf(fileName, "%s/%s", DATA_FOLDER_PATH, imageFileName);
    //         maple_create_and_send_sbpacket(BPK_Req_Copy_File, BPK_Addr_Receive_Esp32,
    //         fileName);

    //         uint8_t fileTransfered = FALSE;
    //         clock_t time           = clock();
    //         while (fileTransfered != TRUE) {

    //             if ((clock() - time) > 2000) {
    //                 fclose(testImage);
    //                 printf("%sTime out when receiving image data%s\n", ASCII_COLOR_RED,
    //                 ASCII_COLOR_WHITE); break;
    //             }

    //             // Wait for data
    //             bpk_packet_t* packet = maple_get_next_bpacket_response();

    //             if (packet == NULL) {
    //                 continue;
    //             }

    //             if (packet->Request != WATCHDOG_BPK_R_COPY_FILE) {
    //                 printf("Skipping packet with request %i\r\n", packet->Request);
    //                 continue;
    //             }

    //             time = clock();

    //             // Store data
    //             for (int i = 0; i < packet->Data.numBytes; i++) {
    //                 fputc(packet->bytes[i], testImage);
    //             }

    //             // Check whether the packet is the end of the data stream
    //             if (packet->Code.val == BPK_Code_Success.val) {

    //                 // Close the file
    //                 fclose(testImage);

    //                 fileTransfered = TRUE;
    //             }
    //         }

    //         if (fileTransfered != TRUE) {
    //             failed = TRUE;
    //         }

    //     } else {
    //         printf("%sSkipping file download. fopen() returned NULL%s\n", ASCII_COLOR_RED,
    //         ASCII_COLOR_WHITE); failed = TRUE;
    //     }
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Streaming */

    // FILE* streamImage;
    // if ((streamImage = fopen("streamImage.jpg", "wb")) != NULL) {

    //     maple_create_and_send_bpacket(BPK_Req_Stream_Images, BPK_Addr_Receive_Esp32, 0,
    //     NULL);

    //     uint8_t fileTransfered = FALSE;
    //     clock_t time           = clock();
    //     while (fileTransfered != TRUE) {

    //         if ((clock() - time) > 6000) {
    //             fclose(streamImage);
    //             printf("%sTime out when receiving image data%s\n", ASCII_COLOR_RED,
    //             ASCII_COLOR_WHITE); break;
    //         }

    //         // Wait for data
    //         bpk_packet_t* packet = maple_get_next_bpacket_response();

    //         if ((packet == NULL) || (packet->Request != WATCHDOG_BPK_R_STREAM_IMAGE)) {
    //             continue;
    //         }

    //         time = clock();

    //         // Store data
    //         for (int i = 0; i < packet->Data.numBytes; i++) {
    //             fputc(packet->bytes[i], streamImage);
    //         }

    //         // Check whether the packet is the end of the data stream
    //         if (packet->Code.val == BPK_Code_Success.val) {

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
    //     printf("%sSkipping file download. fopen() returned NULL%s\n", ASCII_COLOR_RED,
    //     ASCII_COLOR_WHITE); failed = TRUE;
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Recording Data */

    /* Test Listing all the files on the SD card */

    /* Test Getting the Status */

    /* Test that updating the resolution on the camera actually changes the resolution of the photos
     * taken */

    if (failed == FALSE) {
        printf("%sAll tests passed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
    }
}