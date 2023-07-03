/**
 * @file log_usb.c
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

/* STM32 Library Includes */
#include "stm32l4xx_hal.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

/* Personal Includes */
#include "log_usb.h"
#include "chars.h"

char usb_msg[160];
char usb_format[180];

/* Function Prototypes */

void log_usb_error(const char* format, ...) {

    va_list args;
    va_start(args, format);
    vsprintf(usb_msg, format, args);
    va_end(args);

    sprintf(usb_format, "%s%s%s", ASCII_COLOR_RED, usb_msg, ASCII_COLOR_WHITE);
    CDC_Transmit_FS((uint8_t*)usb_format, chars_get_num_bytes(usb_format));

    // For some reason - not sure why if I don't add a small delay at the end of a transmit, some of the messages
    // won't go through. Not sure why but this is just a temporary fix I have found for the moment
    HAL_Delay(10);
}

void log_usb_message(const char* format, ...) {

    va_list args;
    va_start(args, format);
    vsprintf(usb_msg, format, args);
    va_end(args);

    sprintf(usb_format, "%s%s", ASCII_COLOR_WHITE, usb_msg);
    CDC_Transmit_FS((uint8_t*)usb_format, chars_get_num_bytes(usb_format));

    // For some reason - not sure why if I don't add a small delay at the end of a transmit, some of the messages
    // won't go through. Not sure why but this is just a temporary fix I have found for the moment
    HAL_Delay(10);
}

void log_usb_warning(const char* format, ...) {

    va_list args;
    va_start(args, format);
    vsprintf(usb_msg, format, args);
    va_end(args);

    sprintf(usb_format, "%s%s%s", ASCII_COLOR_MAGENTA, usb_msg, ASCII_COLOR_WHITE);
    CDC_Transmit_FS((uint8_t*)usb_format, chars_get_num_bytes(usb_format));

    // For some reason - not sure why if I don't add a small delay at the end of a transmit, some of the messages
    // won't go through. Not sure why but this is just a temporary fix I have found for the moment
    HAL_Delay(10);
}

void log_usb_success(const char* format, ...) {

    sprintf(usb_format, "%s%s%s", ASCII_COLOR_GREEN, format, ASCII_COLOR_WHITE);
    va_list args;
    va_start(args, format);
    vsprintf(usb_msg, format, args);
    va_end(args);

    sprintf(usb_format, "%s%s%s", ASCII_COLOR_GREEN, usb_msg, ASCII_COLOR_WHITE);
    CDC_Transmit_FS((uint8_t*)usb_format, chars_get_num_bytes(usb_format));

    // For some reason - not sure why if I don't add a small delay at the end of a transmit, some of the messages
    // won't go through. Not sure why but this is just a temporary fix I have found for the moment
    HAL_Delay(10);
}

void log_usb_bytes(uint8_t* data, uint16_t length) {
    CDC_Transmit_FS(data, length);

    // For some reason - not sure why if I don't add a small delay at the end of a transmit, some of the messages
    // won't go through. Not sure why but this is just a temporary fix I have found for the moment
    HAL_Delay(10);
}

void log_usb_clear(void) {
    // Prints ANSI escape codes that will clear the terminal screen
    sprintf(usb_format, "%s", ASCII_CLEAR_SCREEN);
    CDC_Transmit_FS((uint8_t*)usb_format, chars_get_num_bytes(usb_format));

    // For some reason - not sure why if I don't add a small delay at the end of a transmit, some of the messages
    // won't go through. Not sure why but this is just a temporary fix I have found for the moment
    HAL_Delay(10);
}