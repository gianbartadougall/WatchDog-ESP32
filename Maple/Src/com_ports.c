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

/* Private Macros */
#define PORT_NAME_MAX_BYTES 50

/* Private Functions */
uint8_t com_ports_close_connection(void);
uint8_t com_ports_configure_port(struct sp_port* port);
enum sp_return com_ports_open_port(struct sp_port* port);
enum sp_return com_ports_search_ports(char portName[PORT_NAME_MAX_BYTES], uint8_t pingResponse);
int com_ports_check(enum sp_return result);
int com_ports_check(enum sp_return result);
uint8_t com_ports_send_bpacket(bpacket_t* bpacket);

/* Private Variables */
struct sp_port* activePort;

uint8_t com_ports_open_connection(uint8_t pingResponse) {

    char espPortName[PORT_NAME_MAX_BYTES];
    espPortName[0]        = '\0';
    enum sp_return result = com_ports_search_ports(espPortName, pingResponse);

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

    if (com_ports_check(com_ports_open_port(activePort)) != SP_OK) {
        printf("Could not open ESP port\n");
        return FALSE;
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

enum sp_return com_ports_search_ports(char portName[PORT_NAME_MAX_BYTES], uint8_t pingResponse) {
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
            continue;
        }

        // Ping port
        bpacket_create_p(&bpacket, BPACKET_GEN_R_PING, 0, NULL);
        bpacket_buffer_t packetBuffer;
        bpacket_to_buffer(&bpacket, &packetBuffer);
        if (sp_blocking_write(port, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
            continue;
        }

        // printf("Here 1\n");
        uint8_t response[BPACKET_BUFFER_LENGTH_BYTES];
        if (sp_blocking_read(port, response, 6, 1500) < 0) {
            sp_close(port);
            continue;
        }
        // printf("Here 2\n");
        /* TEST CODE */
        // uint8_t response[BPACKET_BUFFER_LENGTH_BYTES];
        // bpacket_create_p(&bpacket, BPACKET_GEN_R_PING, 0, NULL);
        // bpacket_buffer_t packetBuffer;
        // bpacket_to_buffer(&bpacket, &packetBuffer);
        // uint8_t length = 6;
        // uint8_t data[length];
        // data[0] = BPACKET_START_BYTE;
        // data[1] = 3;
        // data[2] = 19;
        // data[3] = 'G';
        // data[4] = 'H';
        // data[5] = BPACKET_STOP_BYTE;

        // while (1) {
            
        //     if (sp_blocking_write(port, data, length, 1000) < 0) {
        //         continue;
        //     }

        //     if (sp_blocking_read(port, response, 100, 1000) < 0) {
        //         return 0;
        //     }

        //     for (int i = 0; i < BPACKET_BUFFER_LENGTH_BYTES; i++) {
        //         printf("%c", response[i]);
        //     }

        //     if (response[0] != '\0') {
        //         printf("\n");
        //     }
            
        //     for (int i = 0; i < BPACKET_BUFFER_LENGTH_BYTES; i++) {
        //         response[i] = '\0';
        //     }
            
            
        // } 
        /* TEST CODE */

        bpacket_decode(&bpacket, response);
        // printf("Here 3\n");
        if (bpacket.request != BPACKET_R_SUCCESS || bpacket.bytes[0] != pingResponse) {
            sp_close(port);
            continue;
        }
        // printf("Here 4\n");
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