/**
 * @file uart_lib.h
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef UART_LIB_H
#define UART_LIB_H

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

#endif // UART_LIB_H