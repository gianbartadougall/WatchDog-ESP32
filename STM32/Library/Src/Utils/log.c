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

char msg[160];

/* Function Prototypes */

int (*func_write_chars)(const char* message, ...)     = NULL;
void (*func_write_u8)(uint8_t* data, uint16_t length) = NULL;

void log_init(int (*char_func)(const char* message, ...), void (*u8_func)(uint8_t* data, uint16_t length)) {
    func_write_chars = char_func;
    func_write_u8    = u8_func;
}

void log_error(const char* format, ...) {

    if (func_write_chars == NULL) {
        return;
    }

    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    func_write_chars(ASCII_COLOR_RED);
    func_write_chars(msg);
    func_write_chars(ASCII_COLOR_WHITE);
}

void log_message(const char* format, ...) {

    if (func_write_chars == NULL) {
        return;
    }

    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    func_write_chars(ASCII_COLOR_WHITE);
    func_write_chars(msg);
    func_write_chars(ASCII_COLOR_WHITE);
}

void log_warning(const char* format, ...) {

    if (func_write_chars == NULL) {
        return;
    }

    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    func_write_chars(ASCII_COLOR_MAGENTA);
    func_write_chars(msg);
    func_write_chars(ASCII_COLOR_WHITE);
}

void log_success(const char* format, ...) {

    if (func_write_chars == NULL) {
        return;
    }

    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    func_write_chars(ASCII_COLOR_GREEN);
    func_write_chars(msg);
    func_write_chars(ASCII_COLOR_WHITE);
}

void log_bytes(uint8_t* data, uint16_t length) {

    if (func_write_u8 == NULL) {
        return;
    }

    func_write_chars(ASCII_COLOR_WHITE);
    func_write_u8(data, length);
}

void log_clear(void) {
    // Prints ANSI escape codes that will clear the terminal screen
    func_write_chars(ASCII_CLEAR_SCREEN);
}