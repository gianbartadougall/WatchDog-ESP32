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
enum sp_return com_ports_search_ports(char portName[PORT_NAME_MAX_BYTES],
                                      const bpk_addr_receive_t RAddress, uint8_t pingResponse);
int com_ports_check(enum sp_return result);
int com_ports_check(enum sp_return result);
uint8_t com_ports_send_bpacket(bpk_t* Bpacket);
void comms_port_test(void);

/* Private Variables */
struct sp_port* activePort;

void comms_port_clear_buffer(struct sp_port* port) {
    uint8_t data;
    while (sp_blocking_read(port, &data, 1, 50) > 0) {}
}

uint8_t com_ports_open_connection(const bpk_addr_receive_t RAddress, uint8_t pingResponse) {

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

enum sp_return com_ports_search_ports(char portName[PORT_NAME_MAX_BYTES],
                                      const bpk_addr_receive_t RAddress, uint8_t pingResponse) {
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

        comms_port_clear_buffer(port);

        // Ping port
        bpk_create(&Bpacket, RAddress, BPK_Addr_Send_Maple, BPK_Request_Ping, BPK_Code_Execute, 0,
                   NULL);
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
        if ((responseSize = sp_blocking_read(port, response, BPACKET_BUFFER_LENGTH_BYTES, 100)) <
            0) {
            sp_close(port);
            printf("res < 0\n");
            continue;
        }

        if (bpk_buffer_decode(&Bpacket, response) != TRUE) {
            printf("Error %s %i. Code %i\r\n", __FILE__, __LINE__, Bpacket.ErrorCode.val);
        }

        if ((Bpacket.Code != BPACKET_CODE_SUCCESS) && (Bpacket.Data.bytes[0] != pingResponse)) {
            sp_close(port);

            // Print the entire message received
            printf("Sender: %i Receiver: %i Request: %i Code: %i Num bytes: %i\n",
                   Bpacket.Sender.val, Bpacket.Receiver, Bpacket.Request, Bpacket.Code,
                   Bpacket.Data.numBytes);
            printf("Read %i bytes\n", responseSize);
            for (int i = 0; i < responseSize; i++) {
                printf("%c", response[i]);
            }

            for (int i = 0; i < responseSize; i++) {
                printf("%i ", response[i]);
            }

            printf("\n");

            printf("Code: [%i] with ping: [%i]\n", Bpacket.Code, Bpacket.Data.bytes[0]);
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

    // bpk_t Bpacket;
    // bpk_buffer_t bp;
    // char m[256];
    // sprintf(m, "Hello The sun the, Bzringing it a new dayz full of opportunities and
    // possiBilitiesY, so "
    //            "it's important to zBstart jeach morning witBh A Yjpojsitive mindset, a
    //            grAtefBzjYul heajYrt, and a " "strong determinAtiojYn to mAke the most out
    //            of.akasdfasdfasdfa55454d");
    // bpk_create_sp(&Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32,
    // WATCHDOG_BPK_R_WRITE_TO_FILE,
    //                   BPACKET_CODE_SUCCESS, m);
    // bpk_to_buffer(&Bpacket, &bp);

    // printf("Size: %i\n", Bpacket.Data.numBytes);

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

        bpk_t getRTCTime;
        bpk_buffer_t getPacketBuffer;

        bpk_create(&getRTCTime, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple,
                   WATCHDOG_BPK_R_WRITE_TO_FILE, BPACKET_CODE_EXECUTE, 0, NULL);
        bpk_to_buffer(&getRTCTime, &getPacketBuffer);

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
    }
}