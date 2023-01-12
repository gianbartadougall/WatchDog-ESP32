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
#define FSTR_COMMAND               "COMMAND              \t"
#define FSTR_ACTION                "ACTION"
#define FSTR_LED_RED_ON            "led red on           \t"
#define FSTR_LED_RED_OFF           "led red off          \t"
#define FSTR_LED_COB_ON            "led cob on           \t"
#define FSTR_LED_COB_OFF           "led cob off          \t"
#define FSTR_INFO_FOLDER_STRUCUTRE "info fs              \t"

#define LED_RED_ON            "led red on"
#define LED_RED_OFF           "led red off"
#define LED_COB_ON            "led cob on"
#define LED_COB_OFF           "led cob off"
#define INFO_FOLDER_STRUCUTRE "info fs"

/**
 * Cool things that could be added
 * - Be able to list the folder structure of the SD card
 * - Be able copy data across from the SD card
 * - Status query so you could number of photos, time running for, if there were any errors etc
 * - Take a photo
 * - Be able to create new folders, delete folders and files etc that way you don't have to unplug the SD card every
 * time
 */

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

uint8_t buffer1Flag = 0;

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
        buffer1Flag             = 1;
        return;
    }

    buffer1[buffer1Index] = c;
    buffer1Index++;
}

void comms_usart1_print_buffer(void) {

    if (buffer1Flag == 1) {
        log_message(buffer1);
        buffer1Flag = 0;
    }
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

void comms_create_packet(packet_t* packet, uint8_t request, char* instruction, char* data) {
    packet->request = request;
    sprintf(packet->instruction, "%s", instruction);
    sprintf(packet->data, "%s", data);
}

void comms_send_packet(packet_t* packet) {
    char msg[400];
    sprintf(msg, "%i,%s,%s", packet->request, packet->instruction, packet->data);
    comms_send_data(USART1, msg, TRUE);
}

void serial_comms_process_command(char* string) {

    if (chars_same(string, "help") == TRUE) {
        log_print_const(WATCHDOG_MANUAL);
        return;
    }

    packet_t packet;

    if (chars_same(string, LED_RED_ON) == TRUE) {
        log_message("Turning the RED led on\r\n");
        comms_create_packet(&packet, UART_REQUEST_LED_RED_ON, "\0", "\0");
        comms_send_packet(&packet);
        return;
    }

    if (chars_same(string, LED_RED_OFF) == TRUE) {
        log_message("Turning the RED led off\r\n");
        comms_create_packet(&packet, UART_REQUEST_LED_RED_OFF, "\0", "\0");
        comms_send_packet(&packet);
        return;
    }

    if (chars_same(string, LED_COB_ON) == TRUE) {
        log_message("Turning the COB led on\r\n");
        comms_create_packet(&packet, UART_REQUEST_LED_COB_ON, "\0", "\0");
        comms_send_packet(&packet);
        return;
    }

    if (chars_same(string, LED_COB_OFF) == TRUE) {
        log_message("Turning the COB led off\r\n");
        comms_create_packet(&packet, UART_REQUEST_LED_COB_OFF, "\0", "\0");
        comms_send_packet(&packet);
        return;
    }

    if (chars_same(string, INFO_FOLDER_STRUCUTRE) == TRUE) {
        log_message("Requesting folder structure of SD Card\r\n");
        comms_create_packet(&packet, UART_REQUEST_FOLDER_STRUCTURE, "\0", "\0");
        comms_send_packet(&packet);
        return;
    }

    log_error("Unknown command. Type 'help' to see the list of available commands\r\n");
}

void comms_send_data(USART_TypeDef* uart, char* msg, uint8_t sendNull) {
    uint16_t i = 0;

    // Transmit until end of message reached
    while (msg[i] != '\0') {
        while ((uart->ISR & USART_ISR_TXE) == 0) {};

        uart->TDR = msg[i];
        i++;
    }

    if (sendNull == TRUE) {
        while ((uart->ISR & USART_ISR_TXE) == 0) {};

        uart->TDR = '\0';
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