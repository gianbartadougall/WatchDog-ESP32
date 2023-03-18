/**
 * @file com_ports.c
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-23
 *
 * @copyright Copyright (c) 2023
 *
 */

/* Library Includes */
#include <stdlib.h>
#include <stdio.h>

/* Personal Includes */
#include "com_ports.h"
#include "utilities.h"

// For testng only
#include "datetime.h"
#include "watchdog_defines.h"

/* Private Macros */
#define PORT_NAME_MAX_BYTES 50

/* Private Functions */
uint8_t com_ports_close_connection(void);
uint8_t com_ports_configure_port(struct sp_port* port);
enum sp_return com_ports_open_port(struct sp_port* port);
enum sp_return com_ports_search_ports(char portName[PORT_NAME_MAX_BYTES], const bp_receive_address_t RAddress,
                                      uint8_t pingResponse);
int com_ports_check(enum sp_return result);
int com_ports_check(enum sp_return result);
uint8_t com_ports_send_bpacket(bpacket_t* bpacket);
void comms_port_test(void);

/* Private Variables */
struct sp_port* activePort;

void comms_port_clear_buffer(struct sp_port* port) {
    uint8_t data;
    while (sp_blocking_read(port, &data, 1, 50) > 0) {}
}

uint8_t com_ports_open_connection(const bp_receive_address_t RAddress, uint8_t pingResponse) {

    char espPortName[PORT_NAME_MAX_BYTES];
    espPortName[0]        = '\0';
    enum sp_return result = com_ports_search_ports(espPortName, RAddress, pingResponse);

    if (result != SP_OK) {
        com_ports_check(result);
        return FALSE;
    }

    if (espPortName[0] == '\0') {
        printf("ESP port could not be found\n");
        return FALSE;
    }

    // Open ESP port
    if (com_ports_check(sp_get_port_by_name(espPortName, &activePort)) != SP_OK) {
        printf("Could not find ESP port\n");
        return FALSE;
    }

    if (sp_open(activePort, SP_MODE_READ_WRITE) != SP_OK) {
        printf("Unable to open port port %s\n", espPortName);
    }

    printf("Connection made to port %s\n", sp_get_port_name(activePort));

    return TRUE;
}

uint8_t com_ports_close_connection(void) {

    if (sp_close(activePort) != SP_OK) {
        return FALSE;
    }

    return TRUE;
}

uint8_t com_ports_configure_port(struct sp_port* port) {

    if (port == NULL) {
        return 0;
    }

    com_ports_check(sp_set_baudrate(port, 115200));
    com_ports_check(sp_set_bits(port, 8));
    com_ports_check(sp_set_parity(port, SP_PARITY_NONE));
    com_ports_check(sp_set_stopbits(port, 1));
    com_ports_check(sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));

    return 1;
}

enum sp_return com_ports_open_port(struct sp_port* port) {

    // Open the port
    enum sp_return result = sp_open(port, SP_MODE_READ_WRITE);
    if (result != SP_OK) {
        return result;
    }

    // Configure the port settings for communication
    com_ports_check(sp_set_baudrate(port, 115200));
    com_ports_check(sp_set_bits(port, 8));
    com_ports_check(sp_set_parity(port, SP_PARITY_NONE));
    com_ports_check(sp_set_stopbits(port, 1));
    com_ports_check(sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));

    return SP_OK;
}

enum sp_return com_ports_search_ports(char portName[PORT_NAME_MAX_BYTES], const bp_receive_address_t RAddress,
                                      uint8_t pingResponse) {
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

        if (com_ports_open_port(port) != SP_OK) {
            printf("Unable to open port [%i]\n", i);
            continue;
        }

        comms_port_clear_buffer(port);

        // Ping port
        bp_create_packet(&bpacket, RAddress, BP_ADDRESS_S_MAPLE, BP_GEN_R_PING, BP_CODE_EXECUTE, 0, NULL);
        bpacket_buffer_t packetBuffer;
        bpacket_to_buffer(&bpacket, &packetBuffer);
        if (sp_blocking_write(port, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
            printf("Unable to write\n");
            continue;
        }

        uint8_t response[BPACKET_BUFFER_LENGTH_BYTES];
        for (int i = 0; i < BPACKET_BUFFER_LENGTH_BYTES; i++) {
            response[i] = 0;
        }

        uint8_t responseSize;
        if ((responseSize = sp_blocking_read(port, response, BPACKET_BUFFER_LENGTH_BYTES, 100)) < 0) {
            sp_close(port);
            printf("res < 0\n");
            continue;
        }

        uint8_t result = bpacket_buffer_decode(&bpacket, response);

        if (result != TRUE) {
            char errorMsg[50];
            bpacket_get_error(result, errorMsg);
            printf("%s", errorMsg);
            for (int i = 0; i < responseSize; i++) {
                printf("%c", response[i]);
            }
            printf("\n");
            continue;
        }

        if ((bpacket.code != BPACKET_CODE_SUCCESS) && (bpacket.bytes[0] != pingResponse)) {
            sp_close(port);

            // Print the entire message received
            printf("Sender: %i Receiver: %i Request: %i Code: %i Num bytes: %i\n", bpacket.sender, bpacket.receiver,
                   bpacket.request, bpacket.code, bpacket.numBytes);
            printf("Read %i bytes\n", responseSize);
            for (int i = 0; i < responseSize; i++) {
                printf("%c", response[i]);
            }

            for (int i = 0; i < responseSize; i++) {
                printf("%i ", response[i]);
            }

            printf("\n");

            printf("Code: [%i] with ping: [%i]\n", bpacket.code, bpacket.bytes[0]);
            continue;
        }

        sprintf(portName, "%s", sp_get_port_name(port));
        sp_close(port);
        break;
    }

    sp_free_port_list(port_list);

    return SP_OK;
}

int com_ports_check(enum sp_return result) {
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

uint8_t com_ports_send_bpacket(bpacket_t* bpacket) {
    bpacket_buffer_t packetBuffer;
    bpacket_to_buffer(bpacket, &packetBuffer);

    if (sp_blocking_write(activePort, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
        return FALSE;
    }

    return TRUE;
}

int com_ports_read(void* buf, size_t count, unsigned int timeout_ms) {
    return sp_blocking_read(activePort, buf, count, timeout_ms);
}

void comms_port_test(void) {

    // bpacket_t bpacket;
    // bpacket_buffer_t bp;
    // char m[256];
    // sprintf(m, "Hello The sun the, Bzringing it a new dayz full of opportunities and possiBilitiesY, so "
    //            "it's important to zBstart jeach morning witBh A Yjpojsitive mindset, a grAtefBzjYul heajYrt, and a "
    //            "strong determinAtiojYn to mAke the most out of.akasdfasdfasdfa55454d");
    // bpacket_create_sp(&bpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32, WATCHDOG_BPK_R_WRITE_TO_FILE,
    //                   BPACKET_CODE_SUCCESS, m);
    // bpacket_to_buffer(&bpacket, &bp);

    // printf("Size: %i\n", bpacket.numBytes);

    // for (int i = 0; i < 9; i++) {
    //     printf("%i ", bp.buffer[i]);
    // }
    // printf("\n");
    // while (1) {}

    struct sp_port** port_list;
    enum sp_return result = sp_list_ports(&port_list);

    if (result != SP_OK) {
        return;
    }

    // Iterate through every port. Ping the port and check if
    // the response matches the given target
    for (int i = 0; port_list[i] != NULL; i++) {
        struct sp_port* port = port_list[i];

        if (com_ports_open_port(port) != SP_OK) {
            printf("Unable to open port [%i]\n", i);
            continue;
        } else {
            printf("Opened port %s\n", sp_get_port_name(port));
        }

        bpacket_t getRTCTime;
        bpacket_buffer_t getPacketBuffer;

        // bpacket_create_sp(&getRTCTime, BPACKET_ADDRESS_STM32, BPACKET_ADDRESS_MAPLE, BPACKET_GEN_R_PING,
        //                  BPACKET_CODE_EXECUTE, "img1.jpg\0");
        bpacket_create_p(&getRTCTime, BPACKET_ADDRESS_ESP32, BPACKET_ADDRESS_MAPLE, WATCHDOG_BPK_R_WRITE_TO_FILE,
                         BPACKET_CODE_EXECUTE, 0, NULL);
        bpacket_to_buffer(&getRTCTime, &getPacketBuffer);

        // for (int i = 0; i < getPacketBuffer.numBytes; i++) {
        //     printf("%c", getPacketBuffer.buffer[i]);
        // }

        printf("Starting: %i %i\n", getPacketBuffer.buffer[6], getPacketBuffer.numBytes);

        comms_port_clear_buffer(port);

        // Send request
        if (sp_blocking_write(port, getPacketBuffer.buffer, getPacketBuffer.numBytes, 1000) < 0) {
            printf("Unable to write\n");
            continue;
        }

        uint8_t bytes = 0;
        uint8_t data;

        while ((bytes = sp_blocking_read(port, &data, 1, 3000)) >= 0) {

            if (bytes == 0) {
                continue;
            }

            printf("%c", data);
            // printf("%c (%i)", data, data);
        }

        printf("}\nEnded\n");
        while (1) {}

        // while (1) {

        //     for (int i = 0; i < 1000000000; i++) {}

        //     uint8_t response[BPACKET_BUFFER_LENGTH_BYTES];

        //     uint8_t result = bpacket_buffer_decode(&getRTCTime, response);

        //     if (result != TRUE) {
        //         char errMsg[50];
        //         bpacket_get_error(result, errMsg);
        //         printf(errMsg);
        //         printf("%s\n", response);
        //         continue;
        //     }

        //     if (getRTCTime.code == BPACKET_CODE_SUCCESS) {

        //         printf("Start time: %i:%i End time: %i:%i Interval time: %i:%i Resolution: %i\n",
        //         getRTCTime.bytes[0],
        //                getRTCTime.bytes[1], getRTCTime.bytes[2], getRTCTime.bytes[3], getRTCTime.bytes[4],
        //                getRTCTime.bytes[5], getRTCTime.bytes[6]);

        //         wd_camera_settings_t cameraSettings;
        //         cameraSettings.startTime.second    = 0;
        //         cameraSettings.startTime.minute    = getRTCTime.bytes[0];
        //         cameraSettings.startTime.hour      = getRTCTime.bytes[1];
        //         cameraSettings.endTime.second      = 0;
        //         cameraSettings.endTime.minute      = getRTCTime.bytes[2];
        //         cameraSettings.endTime.hour        = getRTCTime.bytes[3];
        //         cameraSettings.intervalTime.second = 0;
        //         cameraSettings.intervalTime.minute = getRTCTime.bytes[4];
        //         cameraSettings.intervalTime.hour   = getRTCTime.bytes[5];
        //         cameraSettings.resolution          = getRTCTime.bytes[6];

        //         result = wd_bpacket_to_camera_settings(&getRTCTime, &cameraSettings);

        //         if (result == TRUE) {

        //             if (cameraSettings.startTime.hour < 12) {
        //                 cameraSettings.startTime.hour++;
        //             } else {
        //                 cameraSettings.startTime.hour = 3;
        //             }

        //             result = wd_camera_settings_to_bpacket(&getRTCTime, BPACKET_ADDRESS_STM32, BPACKET_ADDRESS_MAPLE,
        //                                                    WATCHDOG_BPK_R_SET_CAMERA_SETTINGS, BPACKET_CODE_EXECUTE,
        //                                                    &cameraSettings);

        //             if (result != TRUE) {
        //                 char errMsg[50];
        //                 wd_get_error(result, errMsg);
        //                 printf(errMsg);
        //                 continue;
        //             }

        //             bpacket_to_buffer(&getRTCTime, &setPacketBuffer);

        //             if (sp_blocking_write(port, setPacketBuffer.buffer, setPacketBuffer.numBytes, 1000) < 0) {
        //                 printf("Unable to set the date\n");
        //                 continue;
        //             }

        //             uint32_t bytesRead = 0;
        //             if ((bytesRead = sp_blocking_read(port, response, 100, 1000)) < 0) {
        //                 printf("Unbable to read\n");
        //                 return;
        //             }

        //             if (bytesRead == 0) {
        //                 continue;
        //             }

        //             result = bpacket_buffer_decode(&getRTCTime, response);

        //             if (result != TRUE) {
        //                 char errMsg[50];
        //                 bpacket_get_error(result, errMsg);
        //                 printf(errMsg);
        //                 continue;
        //             }

        //             printf("Request: [%i] with code: [%i]\n", getRTCTime.request, getRTCTime.code);
        //             // for (int i = 0; i < BPACKET_BUFFER_LENGTH_BYTES; i++) {
        //             //     printf("%c", response[i]);
        //             // }

        //             // if (response[0] != '\0') {
        //             //     printf("\n");
        //             // }

        //             // for (int i = 0; i < BPACKET_BUFFER_LENGTH_BYTES; i++) {
        //             //     response[i] = '\0';
        //             // }
        //         } else {
        //             if (result != TRUE) {
        //                 char errMsg[50];
        //                 wd_get_error(result, errMsg);
        //                 printf(errMsg);
        //                 printf("here\n");
        //                 continue;
        //             }
        //         }

        //         continue;
        //     } else {
        //         printf("Return code not success %i\n", getRTCTime.code);
        //         bpacket_char_array_t msg;
        //         bpacket_data_to_string(&getRTCTime, &msg);
        //         printf(msg.string);
        //         printf("\n");
        //     }
        // }
    }
}