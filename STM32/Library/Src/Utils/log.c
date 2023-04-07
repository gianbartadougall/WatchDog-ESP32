/**
 * @file log.c
 * @author Gian Barta-Dougall
 * @brief Library for logging
 * @version 0.1
 * @date 2022-05-31
 *
 * @copyright Copyright (c) 2022
 *
 */

/* C Library Includes */
#include <stdarg.h>

/* Private Includes */
#include "log.h"
#include "chars.h"

#define LOG_MAX_MESSAGE_SIZE 160

/* Function Prototypes */

int (*transmit_message)(const char* message, ...)    = NULL;
void (*transmit_data)(uint8_t* data, uint8_t length) = NULL;

void log_init(int (*func)(const char* message, ...), void (*bfunc)(uint8_t* data, uint8_t length)) {
    transmit_message = func;
    transmit_data    = bfunc;
}

void log_error(const char* format, ...) {

    if (transmit_message == NULL) {
        return;
    }

    // Limit of 160 characters per print
    char msg[LOG_MAX_MESSAGE_SIZE];

    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    transmit_message(ASCII_COLOR_RED);
    transmit_message(msg);
    transmit_message(ASCII_COLOR_WHITE);
}

void log_message(const char* format, ...) {

    if (transmit_message == NULL) {
        return;
    }

    // Limit of 160 characters per print
    char msg[LOG_MAX_MESSAGE_SIZE];

    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    transmit_message(ASCII_COLOR_WHITE);
    transmit_message(msg);
    transmit_message(ASCII_COLOR_WHITE);
}

void log_warning(const char* format, ...) {

    if (transmit_message == NULL) {
        return;
    }

    // Limit of 160 characters per print
    char msg[LOG_MAX_MESSAGE_SIZE];

    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    transmit_message(ASCII_COLOR_MAGENTA);
    transmit_message(msg);
    transmit_message(ASCII_COLOR_WHITE);
}

void log_success(const char* format, ...) {

    if (transmit_message == NULL) {
        return;
    }

    // Limit of 160 characters per print
    char msg[LOG_MAX_MESSAGE_SIZE];

    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    transmit_message(ASCII_COLOR_GREEN);
    transmit_message(msg);
    transmit_message(ASCII_COLOR_WHITE);
}

void log_data(uint8_t* data, uint8_t length) {
    if (transmit_data == NULL) {
        return;
    }

    transmit_message(ASCII_COLOR_WHITE);
    transmit_data(data, length);
    transmit_message(ASCII_COLOR_WHITE);
}

void log_clear(void) {
    // Prints ANSI escape codes that will clear the terminal screen
    transmit_message(ASCII_CLEAR_SCREEN);
}