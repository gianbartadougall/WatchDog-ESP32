/**
 * @file log.c
 * @author Gian Barta-Dougall
 * @brief Library for debugging
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

    debug_prints(msg);

    return i;
}