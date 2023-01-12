
/* Private Includes */
#include "watchdog.h"
#include "comms.h"
#include "ds18b20.h"
#include "hardware_config.h"
#include "log.h"
#include "uart_comms.h"
#include "wd_utils.h"
#include <stdio.h>

void print_64_bit1(uint64_t number);

void watchdog_init(void) {
    log_clear();
}

void watchdog_update(void) {
    // char msg1[100];

    // while (1) {

    //     comms_send_data(USART1, "1,this is a test,this\0");
    //     // comms_read_data(USART1, msg1, 10000);

    //     // log_prints(msg1);
    //     HAL_Delay(2000);
    // }

    // Main loop: wait for a new byte, then echo it back.
    // char rxb = '0';
    // while (1) {
    //     // Wait for a byte of data to arrive.

    //     while (!(USART1->ISR & USART_ISR_RXNE)) {
    //         log_prints("stuck\r\n");
    //     };
    //     // rxb = USART1->RDR;
    //     sprintf(msg1, "%c\r\n", USART1->RDR);
    //     log_prints(msg1);

    //     while (!(USART1->ISR & USART_ISR_TXE)) {
    //         log_prints("stuck 2\r\n");
    //     };
    //     USART1->TDR = rxb++;

    //     HAL_Delay(1000);
    // }

    // Get message from console
    // uint8_t i = 0;
    // log_prints("starting\r\n");
    while (1) {
        HAL_Delay(2000);
        log_message("printing: ");
        comms_usart1_print_buffer();
        log_prints("\r\n");

        // char c = log_getc();

        // if (c == 0x0D) {
        //     msg[i++] = '\r';
        //     msg[i++] = '\n';
        //     msg[i++] = '\0';
        //     log_prints("\r\n");
        //     break;
        //     i = 0;
        // }

        // // Sometimes putty accidently sends FF on restart for some reason
        // // this stops this from happening
        // if (c != 0xFF) {
        //     msg[i] = c;
        //     char character[2];
        //     sprintf(character, "%c", c);
        //     log_prints(character);
        //     i++;
        // }
    }

    // char msg[100];
    // // Construct packet from string
    // packet_t packet;
    // int num = string_to_packet(&packet, msg);

    // switch (num) {
    //     case UART_ERROR_INVALID_REQUEST:
    //         log_error("Invalid request number\r\n");
    //         return;
    //     default:
    //         break;
    // }

    // // Turn packet back into string to send to esp32
    // char message[RX_BUF_SIZE];
    // if (packet_to_string(&packet, message) != WD_SUCCESS) {
    //     log_error("Failed to pass string\r\n");
    //     return;
    // }

    // char a[RX_BUF_SIZE + 20];
    // sprintf(a, "Sending message: %s\r\n", message);
    // log_message(a);
    // // Send packet to esp32
    // comms_send_data(UART_ESP32, message);

    // // Wait for response from ESP32
    // char esp32Response[RX_BUF_SIZE];
    // if (comms_read_data(UART_ESP32, esp32Response, 1000) == 0) {
    //     log_error("No data read from ESP32\r\n");
    //     return;
    // }

    // // Turn response into packet
    // packet_t esp32ResponsePacket;
    // if (string_to_packet(&esp32ResponsePacket, esp32Response) != WD_SUCCESS) {
    //     char l[290];
    //     sprintf(l, "Failed to pass string from ESP32: '%s'\r\n", esp32Response);
    //     log_error(l);
    //     return;
    // }

    // char m[500];
    // switch (esp32ResponsePacket.request) {
    //     case UART_ERROR_REQUEST_FAILED:
    //         sprintf(m, "UART Request Failed\r\nInstruction: %s\r\nData: %s\r\n", esp32ResponsePacket.instruction,
    //                 esp32ResponsePacket.data);
    //         log_error(m);
    //         break;
    //     case UART_REQUEST_SUCCEEDED:
    //         sprintf(m, "UART Request Succeded\r\nInstruction: %s\r\nData: %s\r\n", esp32ResponsePacket.instruction,
    //                 esp32ResponsePacket.data);
    //         log_success(m);
    //         break;
    //     case UART_ERROR_REQUEST_TRANSLATION_FAILED:
    //         sprintf(m, "UART Request Translation error\r\nInstruction: %s\r\nData: %s\r\n",
    //                 esp32ResponsePacket.instruction, esp32ResponsePacket.data);
    //         log_error(m);
    //         break;
    //     case UART_ERROR_REQUEST_UNKNOWN:
    //         sprintf(m, "UART Request Unknown 1\r\nInstruction: %s\r\nData: %s\r\n", esp32ResponsePacket.instruction,
    //                 esp32ResponsePacket.data);
    //         log_error(m);
    //         break;
    //     default:
    //         sprintf(m, "UART UNKNOWN RESPONSE %i\r\nInstruction: %s\r\nData: %s\r\n", esp32ResponsePacket.request,
    //                 esp32ResponsePacket.instruction, esp32ResponsePacket.data);
    //         log_error(m);
    //         log_error(esp32Response);
    //         return;
    // }
}