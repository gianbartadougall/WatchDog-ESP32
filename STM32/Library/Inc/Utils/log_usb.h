/**
 * @file log_usb_log.c
 * @author Gian Barta-Dougall
 * @brief Library for logging
 * @version 0.1
 * @date 2022-05-31
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef LOG_USB_H
#define LOG_USB_H

/* Public Includes */
#include <stdint.h>
#include <stdio.h>

/* STM32 Includes */

/* Public Macros */
#define ASCII_COLOR_BLACK   "\x1b[30m"
#define ASCII_COLOR_RED     "\x1b[31m"
#define ASCII_COLOR_GREEN   "\x1b[32m"
#define ASCII_COLOR_YELLOW  "\x1b[33m"
#define ASCII_COLOR_BLUE    "\x1b[34m"
#define ASCII_COLOR_MAGENTA "\x1b[35m"
#define ASCII_COLOR_CYAN    "\x1b[36m"
#define ASCII_COLOR_WHITE   "\x1b[37m"
#define ASCII_COLOR_RESET   "\x1b[0m"

#define ASCII_KEY_ENTER 0x0D
#define ASCII_KEY_ESC   0x1C
#define ASCII_KEY_SPACE 0x20

#define CHAR_CARRIGE_RETURN  '\r'
#define CHAR_NEW_LINE        '\n'
#define CHAR_NULL_TERMINATOR '\0'

#define ASCII_CLEAR_SCREEN "\033[2J\033[H"

#define LOG_USB_ERROR() (log_usb_error("Err %s line %i\n", __FILE__, __LINE__))

/**
 * @brief Writes data to console in red
 *
 * @param msg The message to write to the console
 */
void log_usb_error(const char* format, ...);

/**
 * @brief Writes data to console in green
 *
 * @param msg The message to write to the console
 */
void log_usb_success(const char* format, ...);

/**
 * @brief Writes data to console in magenta
 *
 * @param msg The message to write to the console
 */
void log_usb_warning(const char* format, ...);

/**
 * @brief Writes data to console in white
 *
 * @param msg The message to write to the console
 */
void log_usb_message(const char* format, ...);

void log_usb_bytes(uint8_t* data, uint16_t length);

/**
 * @brief Clears the console
 */
void log_usb_clear(void);

#endif // LOG_USB_H