/**
 * @file comms.c
 * @author Gian Barta-Dougall
 * @brief Library for logging
 * @version 0.1
 * @date 2022-05-31
 *
 * @copyright Copyright (c) 2022
 *
 */

/* Private Includes */
#include "comms.h"
#include "wd_utils.h"
#include "log.h"
#include "chars.h"
#include "hardware_config.h"

/* Private Macros */
#define FSTR_COMMAND     "COMMAND              \t"
#define FSTR_ACTION      "ACTION"
#define FSTR_LED_RED_ON  "led red on           \t"
#define FSTR_LED_RED_OFF "led red off          \t"
#define FSTR_LED_COB_ON  "led cob on           \t"
#define FSTR_LED_COB_OFF "led cob off          \t"

#define LED_RED_ON  "led red on"
#define LED_RED_OFF "led red off"
#define LED_COB_ON  "led cob on"
#define LED_COB_OFF "led cob off"

#define ASCII_KEY_ENTER 0x0D
#define ASCII_KEY_ESC   0x1C
#define ASCII_KEY_SPACE 0x20

const char* WATCHDOG_MANUAL = FSTR_COMMAND FSTR_ACTION
    "\r\n" FSTR_LED_RED_ON "Turn the red LED on\r\n" FSTR_LED_RED_OFF "Turn the red LED off\r\n" FSTR_LED_COB_ON
    "Turn the COB LED on\r\n" FSTR_LED_COB_OFF "Turn the COB LED off\r\n";

/* Private Variables */
char buffer1[1024];
char buffer2[1024];
uint16_t buffer1Index = 0;
uint16_t buffer2Index = 0;

/* Private Function Declarations */
void serial_comms_process_command(char* string);
void comms_usart1_add_to_buffer(char c);
void comms_usart2_add_to_buffer(char c);

int comms_skip_char(char c) {

    if (c == ASCII_KEY_ENTER) {
        return FALSE;
    }

    if (c == ASCII_KEY_SPACE) {
        return FALSE;
    }

    if (c >= '!' || c <= '~') {
        return FALSE;
    }

    return TRUE;
}

void comms_add_to_buffer(USART_TypeDef* usart, char c) {

    if (usart == USART1) {
        comms_usart1_add_to_buffer(c);
        return;
    }

    if (usart == USART2) {
        comms_usart2_add_to_buffer(c);
        return;
    }
}

void comms_usart1_add_to_buffer(char c) {

    if (c == '\0') {
        buffer1[buffer1Index++] = '\r';
        buffer1[buffer1Index++] = '\n';
        buffer1[buffer1Index]   = '\0';
        buffer1Index            = 0;
        return;
    }

    buffer1[buffer1Index] = c;
    buffer1Index++;
}

void comms_usart1_print_buffer(void) {
    log_message(buffer1);
}

void comms_usart2_add_to_buffer(char c) {

    // Check whether the character is a letter/number or something else
    if (comms_skip_char(c) == TRUE) {
        return;
    }

    buffer2[buffer2Index] = c;
    buffer2Index++;

    if (c == ASCII_KEY_ENTER) {
        buffer2[buffer2Index++] = '\r';
        buffer2[buffer2Index++] = '\n';
        buffer2[buffer2Index++] = '\0';
        log_message(buffer2);

        // Remove \r\n for processing
        buffer2[buffer2Index - 3] = '\0';
        buffer2Index              = 0;

        serial_comms_process_command(buffer2);
    }
}

void serial_comms_process_command(char* string) {

    if (chars_same(string, "help") == TRUE) {
        log_print_const(WATCHDOG_MANUAL);
        return;
    }

    char esp32Message[100];

    if (chars_same(string, LED_RED_ON) == TRUE) {
        log_message("Turning on the RED led\r\n");
        sprintf(esp32Message, "%i,%s,%s", UART_REQUEST_LED_RED_ON, "", "");
        comms_send_data(UART_ESP32, esp32Message);
        return;
    }

    if (chars_same(string, LED_RED_OFF) == TRUE) {
        log_message("Turning off the RED led\r\n");
        sprintf(esp32Message, "%i,%s,%s", UART_REQUEST_LED_RED_OFF, "", "");
        comms_send_data(UART_ESP32, esp32Message);
        return;
    }

    if (chars_same(string, LED_COB_ON) == TRUE) {
        log_message("Turning on the COB led\r\n");
        sprintf(esp32Message, "%i,%s,%s", UART_REQUEST_LED_COB_OFF, "", "");
        comms_send_data(UART_ESP32, esp32Message);
        return;
    }

    if (chars_same(string, LED_COB_OFF) == TRUE) {
        log_message("Turning off the COB led\r\n");
        sprintf(esp32Message, "%i,%s,%s", UART_REQUEST_LED_COB_OFF, "", "");
        comms_send_data(UART_ESP32, esp32Message);
        return;
    }

    log_error("Unknown command. Type 'help' to see the list of available commands\r\n");
}

void comms_send_data(USART_TypeDef* uart, char* msg) {
    uint16_t i = 0;

    // Transmit until end of message reached
    while (msg[i] != '\0') {
        while ((uart->ISR & USART_ISR_TXE) == 0) {};

        uart->TDR = msg[i];
        i++;
    }
}

char comms_getc(USART_TypeDef* uart) {

    while (!(uart->ISR & USART_ISR_RXNE)) {};
    return uart->RDR;
}

int comms_read_data(USART_TypeDef* uart, char msg[RX_BUF_SIZE], uint32_t timeout) {

    uint32_t endTime = ((HAL_GetTick() + timeout) % HAL_MAX_DELAY);

    uint8_t i = 0;
    while (1) {

        // Wait for character to be recieved on Rx line
        if (!(uart->ISR & USART_ISR_RXNE)) {
            if (HAL_GetTick() == endTime) {
                msg[i] = '\0';
                return i;
            }

            continue;
        };

        char c = uart->RDR;

        // UART sometimes stuffs up and sends this character, you need to make sure it's
        // not written in the msg otherwise it'll stuff up
        if (c == 0xFF) {
            continue;
        }

        if (c == '\0') {
            break;
        }

        msg[i] = c;
        i++;
    }

    log_prints(msg);

    return i;
}