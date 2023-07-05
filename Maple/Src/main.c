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
#include "log.h"
#include "bpacket_utils.h"
#include "cbuffer.h"

#define LOG_ERROR_CODE(code) (log_error("Error %s line %i. Code %i\r\n", __FILE__, __LINE__, code))
#define LOG_ERROR_MSG(msg)   (log_message("Error %s line %i: \'%s\'\r\n", __FILE__, __LINE__, msg))

// #define MAPLE_MAX_ARGS     5
#define PACKET_BUFFER_SIZE 50

cbuffer_t RxCbuffer;
cbuffer_t TxCbuffer;

#define MAPLE_BUFFER_NUM_ELEMENTS 10
#define MAPLE_BUFFER_NUM_BYTES    (sizeof(bpk_t) * MAPLE_BUFFER_NUM_ELEMENTS)
uint8_t rxCbufferBytes[MAPLE_BUFFER_NUM_BYTES];
uint8_t txCbufferBytes[MAPLE_BUFFER_NUM_BYTES];

void maple_print_bpacket_data(bpk_t* Bpacket);
void maple_create_and_send_bpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                   uint8_t numDataBytes, uint8_t* data);
void maple_print_uart_response(void);
void maple_test(void);
void maple_gui_init(void);
void maple_handle_watchdog_response(bpk_t* Bpacket);

uint32_t packetBufferIndex  = 0;
uint32_t packetPendingIndex = 0;
bpk_t packetBuffer[PACKET_BUFFER_SIZE];

struct sp_port* gbl_activePort = NULL;

void print_data(uint8_t* data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        printf("%c", data[i]);
    }
    printf("\r\n");
}

uint8_t maple_send_bpacket(bpk_t* Bpacket) {

    bpk_buffer_t packetBuffer;
    bpk_to_buffer(Bpacket, &packetBuffer);

    if (sp_blocking_write(gbl_activePort, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
        return FALSE;
    }

    return TRUE;
}

void maple_create_and_send_bpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                   uint8_t numDataBytes, uint8_t* data) {
    bpk_t Bpacket;
    if (bpk_create(&Bpacket, Receiver, BPK_Addr_Send_Maple, Request, BPK_Code_Execute, numDataBytes,
                   data) != TRUE) {
        log_error("Error %s %i. Code %i", __FILE__, __LINE__, Bpacket.ErrorCode.val);
        return;
    }

    if (maple_send_bpacket(&Bpacket) != TRUE) {
        log_error("Failed to send bpacket\n");
        return;
    }
}

void maple_print_bpacket_data(bpk_t* Bpacket) {

    if (Bpacket->Request.val != BPK_Request_Message.val) {
        log_message("Request: %i Len: %i Data: ", Bpacket->Request.val, Bpacket->Data.numBytes);
    }

    for (int i = 0; i < Bpacket->Data.numBytes; i++) {
        log_message("%c", Bpacket->Data.bytes[i]);
    }
}

void maple_print_uart_response(void) {

    int packetsFinished = FALSE;
    bpk_t Bpacket;
    while (packetsFinished == FALSE) {

        // Wait until the packet is ready
        while (cbuffer_read_next_element(&RxCbuffer, (void*)&Bpacket) != TRUE) {}

        // Packet ready. Print its contents
        for (int i = 0; i < Bpacket.Data.numBytes; i++) {
            log_message("%c", Bpacket.Data.bytes[i]);
        }

        if (Bpacket.Request.val == BPK_Code_Success.val) {
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

    uint8_t byte, expectedByte;
    int numBytes;
    cbuffer_t ByteBuffer;
    uint8_t byteBuffer[8];
    bpk_utils_init_expected_byte_buffer(byteBuffer);
    cbuffer_init(&ByteBuffer, (void*)byteBuffer, sizeof(uint8_t), 8);
    bpk_t Bpacket = {0};
    uint8_t bi    = 0;

    while ((numBytes = maple_read_port(&byte, 1, 0)) >= 0) {

        // Read the current index of the byte buffer and store the result into
        // expected byte
        cbuffer_read_current_element(&ByteBuffer, (void*)&expectedByte);

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
                if (Bpacket.Request.val == BPK_REQUEST_MESSAGE) {
                    switch (Bpacket.Code.val) {
                        case BPK_CODE_ERROR:
                            printf(ASCII_COLOR_RED);
                            break;

                        case BPK_CODE_SUCCESS:
                            printf(ASCII_COLOR_GREEN);
                            break;

                        case BPK_CODE_DEBUG:
                            printf(ASCII_COLOR_MAGENTA);
                            break;

                        default:
                            printf(ASCII_COLOR_BLUE);
                    }
                    print_data(Bpacket.Data.bytes, Bpacket.Data.numBytes);
                    printf(ASCII_COLOR_WHITE);
                } else {

                    // Write bpacket to circular buffer
                    cbuffer_append_element(&RxCbuffer, (void*)&Bpacket);
                }

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

                // Write bpacket to circular buffer
                cbuffer_append_element(&RxCbuffer, (void*)&Bpacket);

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
                // log_error("BPK Read failed. %i %i. Num bytes %i\n", expectedByte, byte,
                // numBytes);
        }

        // Reset the expected byte back to the start and bpacket buffer index
        cbuffer_reset_read_index(&ByteBuffer);
        bi = 0;
    }

    log_error("Connection terminated with code %i\n", numBytes);

    return FALSE;
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

        // Creating thread inside this function because the thread needs to be active after a port
        // is open but before it closes. Anytime you try listen when a port isn't open the sp read
        // function will return error code and so the thread will exit
        HANDLE thread = CreateThread(NULL, 0, maple_listen_rx, NULL, 0, NULL);

        if (!thread) {
            log_error("Thread failed\n");
            return 0;
        }

        // Configure the port settings for communication
        sp_set_baudrate(gbl_activePort, 115200);
        sp_set_bits(gbl_activePort, 8);
        sp_set_parity(gbl_activePort, SP_PARITY_NONE);
        sp_set_stopbits(gbl_activePort, 1);
        sp_set_flowcontrol(gbl_activePort, SP_FLOWCONTROL_NONE);

        // Send a ping
        bpk_t Bpacket;
        bpk_buffer_t bpacketBuffer;
        bpk_create(&Bpacket, receiver, BPK_Addr_Send_Maple, BPK_Request_Ping, BPK_Code_Execute, 0,
                   NULL);
        bpk_to_buffer(&Bpacket, &bpacketBuffer);

        if (sp_blocking_write(gbl_activePort, bpacketBuffer.buffer, bpacketBuffer.numBytes, 100) <
            0) {
            printf("Unable to write\n");
            continue;
        }

        // Response may include other incoming messages as well, not just a response to a ping.
        // Create a timeout of 2 seconds and look for response from ping

        clock_t startTime = clock();
        bpk_t PingBpacket;
        while ((clock() - startTime) < 200) {

            // Wait for Bpacket to be received
            if (cbuffer_read_next_element(&RxCbuffer, (void*)&PingBpacket) == TRUE) {

                // Check if the bpacket matches the ping code
                if (PingBpacket.Request.val != BPK_REQUEST_PING) {
                    continue;
                }

                if (PingBpacket.Data.bytes[0] != WATCHDOG_PING_CODE_STM32) {
                    continue;
                }

                portFound = TRUE;
                break;
            }
        }

        // Close port and check next port if this was not the correct port
        if (portFound != TRUE) {

            // Terminate the thread
            TerminateThread(thread, FALSE);

            // Close the port
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
            bpk_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
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

        bpk_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
    }

    fclose(target);

    return TRUE;
}

int main(int argc, char** argv) {

    // Initialise logging
    log_init(printf, print_data);

    // TODO: Check the folder to see if the watchdog folder exists or not. If

    // Initialise the GUI
    maple_gui_init();

    cbuffer_init(&RxCbuffer, rxCbufferBytes, sizeof(bpk_t), MAPLE_BUFFER_NUM_ELEMENTS);

    // Main loop
    uint8_t watchdogConnected = FALSE;
    bpk_t BpkWatchdogResponse;
    while (1) {

        if (watchdogConnected != TRUE) {

            if (maple_connect_to_device(BPK_Addr_Receive_Stm32, WATCHDOG_PING_CODE_STM32) != TRUE) {

                // TODO: Delay 500ms

                // TODO: Display error on GUI

                continue;
            }

            // TODO: Display connection status on GUI
            char* portName = sp_get_port_name(gbl_activePort);
            log_success("Watchdog connected (%s)\r\n", portName);
            free(portName);
        }

        if (watchdogConnected != TRUE) {
            continue;
        }

        // Process any bpackets received from a Watchdog
        if (cbuffer_read_next_element(&RxCbuffer, &BpkWatchdogResponse) == TRUE) {
            maple_handle_watchdog_response(&BpkWatchdogResponse);
        }
    }
}

void maple_handle_watchdog_response(bpk_t* Bpacket) {
    // TODO: Implement
}

uint8_t maple_response_is_valid(bpk_request_t ExpectedRequest, uint16_t timeout) {

    // Print response
    clock_t startTime = clock();
    bpk_t Bpacket;
    while ((clock() - startTime) < timeout) {

        // Wait for next response
        while (cbuffer_read_next_element(&RxCbuffer, (void*)&Bpacket) != TRUE) {}

        // Skip if the Bpacket is just a message
        if (Bpacket.Request.val == BPK_Request_Message.val) {
            continue;
        }

        // Confirm the received Bpacket matches the correct request and code
        if (Bpacket.Request.val != ExpectedRequest.val) {
            return FALSE;
        }

        if (Bpacket.Code.val != BPK_Code_Success.val) {
            printf("Expected code %i but got %i\n", BPK_Code_Success.val, Bpacket.Request.val);
            return FALSE;
        }

        return TRUE;
    }

    printf("Failed to get response\n");
    return FALSE;
}

uint8_t maple_get_response(bpk_t* Bpacket, bpk_request_t Request, uint16_t timeout) {

    // Print response
    clock_t startTime = clock();
    while ((clock() - startTime) < timeout) {

        while (cbuffer_read_next_element(&RxCbuffer, (void*)Bpacket) != TRUE) {}

        // If there is next response yet then skip
        if (Bpacket->Request.val == Request.val) {
            return TRUE;
        }

        char info[80];
        bpk_get_info(Bpacket, info);
        printf(info);
    }

    return FALSE;
}

uint8_t maple_restart_esp32(void) {

    // Turn the ESP32 off. Wait for 200ms. Turn the ESP32 back on. Wait for 1000ms. This ensures
    // that when we get the capture time settings from the esp32 we can know whether the capture
    // time settings set above were saved to the SD card or not
    bpk_t stateBpacket;
    maple_create_and_send_bpacket(BPK_Req_Esp32_Off, BPK_Addr_Receive_Stm32, 0, NULL);
    if (maple_get_response(&stateBpacket, BPK_Req_Esp32_Off, 1000) != TRUE) {
        printf("%sTurning the ESP32 off failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        return FALSE;
    }

    clock_t time = clock();
    while ((clock() - time) < 200) {}

    // Delay of 1500ms because the STM32 will automatically delay for 1s when turning the esp32
    // so it has enough time to boot up
    maple_create_and_send_bpacket(BPK_Req_Esp32_On, BPK_Addr_Receive_Stm32, 0, NULL);
    if (maple_get_response(&stateBpacket, BPK_Req_Esp32_On, 2000) != TRUE) {
        printf("%sTurning the ESP32 on failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        return FALSE;
    }

    return TRUE;
}

void maple_gui_init(void) {
    // TODO: Implement
}

void maple_test(void) {

    // bpk_t Bpacket;
    // uint8_t failed = FALSE;
    // char msg[100];

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
    // bpk_t ImageBpacket;
    // bpk_create_sp(&ImageBpacket, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple,
    //                 BPK_Req_Take_Photo, BPK_Code_Execute,
    //                 fileName);=

    //         uint8_t fileTransfered = FALSE;
    //         clock_t time           = clock();
    //         while (fileTransfered != TRUE) {

    //             if ((clock() - time) > 2000) {
    //                 fclose(testImage);
    //                 printf("%sTime out when receiving image data%s\n", ASCII_COLOR_RED,
    //                 ASCII_COLOR_WHITE); break;
    //             }

    //             // Wait for data
    //             bpk_t* packet = maple_get_next_bpacket_response();

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
    //         bpk_t* packet = maple_get_next_bpacket_response();

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

    // if (failed == FALSE) {
    //     printf("%sAll tests passed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
    // }
}

/* GARBAGE CODE */
// // Initialise rx and tx buffers
// cbuffer_init(&RxCbuffer, rxCbufferBytes, sizeof(bpk_t), MAPLE_BUFFER_NUM_ELEMENTS);
// cbuffer_init(&TxCbuffer, txCbufferBytes, sizeof(bpk_t), MAPLE_BUFFER_NUM_ELEMENTS);
// while (1) {}

// int i = 0;
// while (1) {

//     if (i == 0) {
//         maple_create_and_send_bpacket(BPK_Req_Led_Red_On, BPK_Addr_Receive_Esp32, 0, NULL);
//         i = 1;
//     } else {
//         maple_create_and_send_bpacket(BPK_Req_Led_Red_Off, BPK_Addr_Receive_Esp32, 0, NULL);
//         i = 0;
//     }

//     Sleep(2000);
// }

// maple_test();

// /* This code currently does nothing as there is an infinite while loop in there! */
// HANDLE guiThread = CreateThread(NULL, 0, gui, NULL, 0, NULL);

// if (!guiThread) {
//     printf("Thread failed\n");
//     return 0;
// }

// while (1) {}

// return 0;