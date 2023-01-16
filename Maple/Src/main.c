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

/* Personal Includes */
#include "uart_comms.h"
#include "chars.h"
#include "uart_lib.h"
#include "watchdog_defines.h"

#define MAPLE_MAX_ARGS 5

/* Example of how to get a list of serial ports on the system.
 *
 * This example file is released to the public domain. */

int check(enum sp_return result);
const char* parity_name(enum sp_parity parity);

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
    for (int i = 0; port_list[i] != NULL; i++) {
        struct sp_port* port = port_list[i];

        if (maple_open_port(port) != SP_OK) {
            continue;
        }

        // Ping port
        char ping[50];
        sprintf(ping, "%i,h,h", UART_REQUEST_PING);
        if (sp_blocking_write(port, ping, strlen(ping) + 1, 1100) < 0) {
            sp_close(port);
            continue;
        }

        // Wait for response from port
        char response[100];
        response[0] = '\0';
        if (sp_blocking_read(port, response, 100, 1100) < 0) {
            sp_close(port);
            continue;
        }

        // Confirm response matches target
        if (chars_contains(response, "ESP32 Watchdog") == TRUE) {
            strcpy(portName, sp_get_port_name(port));
            sp_close(port);
            break;
        }
    }

    sp_free_port_list(port_list);

    return SP_OK;
}

uint8_t maple_match_args(char** args, int numArgs, char uartString[100]) {

    if (numArgs == 1) {

        if (chars_same(args[0], "rec\0") == TRUE) {
            sprintf(uartString, "%i,%s,%s", UART_REQUEST_RECORD_DATA, "data.txt", "21/02/2023 0900 26.625\r\n");
            return TRUE;
        }

        if (chars_same(args[0], "ls\0") == TRUE) {
            sprintf(uartString, "%i,%s,%s", UART_REQUEST_LIST_DIRECTORY, "", "");
            return TRUE;
        }
    }

    if (numArgs == 2) {

        if (chars_same(args[0], "ls\0") == TRUE) {
            sprintf(uartString, "%i,%s,%s", UART_REQUEST_LIST_DIRECTORY, args[1], "");
            return TRUE;
        }
    }

    if (numArgs == 3) {

        if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
            chars_same(args[2], "on\0") == TRUE) {
            sprintf(uartString, "%i,%s,%s", UART_REQUEST_LED_RED_ON, "", "");
            return TRUE;
        }

        if (chars_same(args[0], "led\0") == TRUE && chars_same(args[1], "red\0") == TRUE &&
            chars_same(args[2], "off\0") == TRUE) {
            sprintf(uartString, "%i,%s,%s", UART_REQUEST_LED_RED_OFF, "", "");
            return TRUE;
        }
    }

    return FALSE;
}

int main(int argc, char** argv) {

    char espPortName[50];
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

        char uartString[100];
        if (maple_match_args(args, numArgs, uartString) == TRUE) {
            if (sp_blocking_write(port, uartString, strlen(uartString) + 1, 200) < 0) {
                printf("Failed to send '%s'\n", uartString);
            }

            char response[100];
            response[0] = '\0';
            if (sp_blocking_read(port, response, 100, 2000) >= 0) {
                printf("%s\n", response);
            }

        } else {
            printf("Invalid arguments\n");
        }

        // Free list of args
        for (int i = 0; i < numArgs; i++) {
            free(args[i]);
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

    return 0;
}

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
