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

/* Private Includes */
#include "log.h"

#define LOG_COLOR_BLACK   "\x1b[30m"
#define LOG_COLOR_RED     "\x1b[31m"
#define LOG_COLOR_GREEN   "\x1b[32m"
#define LOG_COLOR_YELLOW  "\x1b[33m"
#define LOG_COLOR_BLUE    "\x1b[34m"
#define LOG_COLOR_MAGENTA "\x1b[35m"
#define LOG_COLOR_CYAN    "\x1b[36m"
#define LOG_COLOR_WHITE   "\x1b[37m"
#define LOG_COLOR_RESET   "\x1b[0m"

uint16_t get_length(char* msg) {

    uint16_t length = 0;
    while (msg[length] != '\0') {
        length++;
    }

    return length;
}

void log_error(char* msg) {

    // Add additonal space to length for colour codes
    uint16_t length = get_length(msg) + 18;

    char cmsg[length];
    sprintf(cmsg, "%s%s%s", LOG_COLOR_RED, msg, LOG_COLOR_WHITE);
    log_prints(cmsg);
}

void log_message(char* msg) {

    // Add additonal space to length for colour codes
    uint16_t length = get_length(msg) + 18;

    char cmsg[length];
    sprintf(cmsg, "%s%s%s", LOG_COLOR_CYAN, msg, LOG_COLOR_WHITE);
    log_prints(cmsg);
}

void log_warning(char* msg) {

    // Add additonal space to length for colour codes
    uint16_t length = get_length(msg) + 18;

    char cmsg[length];
    sprintf(cmsg, "%s%s%s", LOG_COLOR_YELLOW, msg, LOG_COLOR_WHITE);
    log_prints(cmsg);
}

void log_success(char* msg) {

    // Add additonal space to length for colour codes
    uint16_t length = get_length(msg) + 18;

    char cmsg[length];
    sprintf(cmsg, "%s%s%s", LOG_COLOR_GREEN, msg, LOG_COLOR_WHITE);
    log_prints(cmsg);
}

void log_prints(char* msg) {
    uint16_t i = 0;

    // Transmit until end of message reached
    while (msg[i] != '\0') {
        while ((USART2->ISR & USART_ISR_TXE) == 0) {};

        USART2->TDR = msg[i];
        i++;
    }
}

void log_print_const(const char* msg) {
    uint16_t i = 0;

    // Transmit until end of message reached
    while (msg[i] != '\0') {
        while ((USART2->ISR & USART_ISR_TXE) == 0) {};

        USART2->TDR = msg[i];
        i++;
    }
}

char log_getc(void) {

    while (!(USART2->ISR & USART_ISR_RXNE)) {};
    return USART2->RDR;
}

void log_clear(void) {
    // Prints ANSI escape codes that will clear the terminal screen
    log_prints("\033[2J\033[H");
}

void log_send_data(char* data, uint8_t length) {

    // Transmit until end of message reached
    for (int i = 0; i < length; i++) {
        while ((USART2->ISR & USART_ISR_TXE) == 0) {};
        USART2->TDR = data[i];
    }
}

void log_send_bdata(uint8_t* data, uint8_t length) {

    // Transmit until end of message reached
    for (int i = 0; i < length; i++) {
        while ((USART2->ISR & USART_ISR_TXE) == 0) {};
        USART2->TDR = data[i];
    }
}

/**
 * @brief This is a test function. Code can be used for an actual uart communication
 * peripheral file where something like a desktop computer can talk to the STM32.
 *
 * This function reads serial input from a program like putty
 * and puts it into a string and then echos the message back to the
 * terminal after the return key is pressed (0x0D)
 *
 */
void serial_communicate(void) {

    log_clear();
    char msg[100];
    uint8_t i = 0;
    while (1) {

        char c = log_getc();
        // sprintf(msg, "%x\r\n", c);
        // log_prints(msg);
        if (c == 0x0D) {
            msg[i++] = '\r';
            msg[i++] = '\n';
            msg[i++] = '\0';
            log_prints(msg);
            i = 0;
        } else {
            msg[i] = c;
            i++;
        }
    }
}