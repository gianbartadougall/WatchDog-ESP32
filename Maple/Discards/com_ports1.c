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
#include "utils.h"

// For testng only
#include "datetime.h"
#include "watchdog_defines.h"

/* Private Macros */
#define PORT_NAME_MAX_BYTES 50

/* Private Functions */
uint8_t com_ports_close_connection(void);
uint8_t com_ports_configure_port(struct sp_port* port);
enum sp_return com_ports_open_port(struct sp_port* port);
enum sp_return com_ports_search_ports(char portName[PORT_NAME_MAX_BYTES], uint8_t address,
                                      uint8_t pingResponse);
int com_ports_check(enum sp_return result);
int com_ports_check(enum sp_return result);
uint8_t com_ports_send_bpacket(bpk_t* Bpacket);
void comms_port_test(void);

/* Private Variables */
struct sp_port* activePort;

uint8_t com_ports_open_connection(uint8_t address, uint8_t pingResponse) {

    char espPortName[PORT_NAME_MAX_BYTES];
    espPortName[0]        = '\0';
    enum sp_return result = com_ports_search_ports(espPortName, address, pingResponse);

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

enum sp_return com_ports_search_ports(char portName[PORT_NAME_MAX_BYTES], uint8_t address,
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
    bpk_t Bpacket;
    for (int i = 0; port_list[i] != NULL; i++) {

        struct sp_port* port = port_list[i];

        if (com_ports_open_port(port) != SP_OK) {
            printf("Unable to open port [%i]\n", i);
            continue;
        }

        // Ping port
        bpk_create(&Bpacket, address, BPK_Addr_Send_Maple, BPK_REQUEST_PING, BPACKET_CODE_EXECUTE,
                   0, NULL);
        bpk_buffer_t packetBuffer;
        bpk_to_buffer(&Bpacket, &packetBuffer);
        if (sp_blocking_write(port, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
            printf("Unable to write\n");
            continue;
        }

        uint8_t response[BPACKET_BUFFER_LENGTH_BYTES];
        for (int i = 0; i < BPACKET_BUFFER_LENGTH_BYTES; i++) {
            response[i] = 0;
        }

        uint8_t responseSize;
        if ((responseSize = sp_blocking_read(port, response, 6, 200)) < 0) {
            sp_close(port);
            printf("res < 0\n");
            continue;
        }

        if (responseSize < 0) {
            sp_close(port);
            continue;
        }

        bpk_buffer_decode(&Bpacket, response);

        if ((Bpacket.Request != BPACKET_CODE_SUCCESS) && (Bpacket.Data.bytes[0] != pingResponse)) {
            sp_close(port);
            printf("Request: [%i] with ping: [%i]\n", Bpacket.Request, Bpacket.Data.bytes[0]);
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

uint8_t com_ports_send_bpacket(bpk_t* Bpacket) {
    bpk_buffer_t packetBuffer;
    bpk_to_buffer(Bpacket, &packetBuffer);

    if (sp_blocking_write(activePort, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
        return FALSE;
    }

    return TRUE;
}

int com_ports_read(void* buf, size_t count, unsigned int timeout_ms) {
    return sp_blocking_read(activePort, buf, count, timeout_ms);
}

void comms_port_test(void) {

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
        }

        bpk_t getRTCTime, setRTCTime;
        bpk_buffer_t getPacketBuffer, setPacketBuffer;

        bpk_create(&getRTCTime, BPK_Addr_Send_Stm32, BPK_Addr_Send_Maple,
                   WATCHDOG_BPK_R_GET_DATETIME, BPACKET_CODE_EXECUTE, 0, NULL);
        bpk_to_buffer(&getRTCTime, &getPacketBuffer);

        // printf("packet buffer length: %i %i %i\n", getPacketBuffer.numBytes,
        // getPacketBuffer.request, getRTCTime.code);

        // uint8_t length = 6;
        // uint8_t data[length];
        // data[0] = BPACKET_START_BYTE;
        // data[1] = 3;
        // data[2] = BPK_REQUEST_PING;
        // data[3] = 'G';
        // data[4] = 'H';
        // data[5] = BPACKET_STOP_BYTE;
        printf("Starting\n");
        while (1) {

            if (sp_blocking_write(port, getPacketBuffer.buffer, getPacketBuffer.numBytes, 1000) <
                0) {
                printf("Unable to write\n");
                continue;
            }

            uint8_t response[BPACKET_BUFFER_LENGTH_BYTES];
            if (sp_blocking_read(port, response, 100, 1000) < 0) {
                printf("Unbable to read\n");
                return;
            }

            uint8_t result = bpk_buffer_decode(&getRTCTime, response);

            // if (result != TRUE) {
            //     char errMsg[50];
            //     bpacket_get_error(result, errMsg);
            //     printf(errMsg);
            //     printf("%s\n", response);
            //     continue;
            // }

            if (getRTCTime.code == BPACKET_CODE_SUCCESS) {
                printf("%i:%i:%i %i/%i/%i\n", getRTCTime.Data.bytes[0], getRTCTime.Data.bytes[1],
                       getRTCTime.Data.bytes[2], getRTCTime.Data.bytes[3], getRTCTime.Data.bytes[4],
                       (getRTCTime.Data.bytes[5] << 8) | getRTCTime.Data.bytes[6]);

                if (getRTCTime.Data.bytes[0] >= 15) {

                    // Reset the RTC
                    dt_datetime_t datetime;
                    wd_bpacket_to_datetime(&getRTCTime, &datetime);
                    datetime.time.second = 0;
                    datetime.time.minute = 0;

                    result = wd_datetime_to_bpacket(
                        &setRTCTime, BPK_Addr_Send_Stm32, BPK_Addr_Send_Maple,
                        WATCHDOG_BPK_R_SET_DATETIME, BPACKET_CODE_EXECUTE, &datetime);
                    // if (result != TRUE) {
                    //     char errMsg[50];
                    //     bpacket_get_error(result, errMsg);
                    //     printf(errMsg);
                    //     continue;
                    // }

                    bpk_to_buffer(&setRTCTime, &setPacketBuffer);

                    if (sp_blocking_write(port, setPacketBuffer.buffer, setPacketBuffer.numBytes,
                                          1000) < 0) {
                        printf("Unable to set the date\n");
                        continue;
                    }

                    if (sp_blocking_read(port, response, 100, 1000) < 0) {
                        printf("Unbable to read\n");
                        return;
                    }

                    result = bpk_buffer_decode(&getRTCTime, response);

                    // if (result != TRUE) {
                    //     char errMsg[50];
                    //     bpacket_get_error(result, errMsg);
                    //     printf(errMsg);
                    //     continue;
                    // }

                    printf("Request: [%i] with code: [%i]\n", getRTCTime.request, getRTCTime.code);

                    // for (int i = 0; i < BPACKET_BUFFER_LENGTH_BYTES; i++) {
                    //     printf("%c", response[i]);
                    // }

                    // if (response[0] != '\0') {
                    //     printf("\n");
                    // }

                    // for (int i = 0; i < BPACKET_BUFFER_LENGTH_BYTES; i++) {
                    //     response[i] = '\0';
                    // }
                }
            } else {
                printf("Return code not success %i\n", getRTCTime.code);
            }
        }
    }
}