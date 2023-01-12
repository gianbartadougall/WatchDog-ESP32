/**
 * @file interrupts.c
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-12
 *
 * @copyright Copyright (c) 2023
 *
 */

/* Public Includes */
#include "stm32l432xx.h"

/* Private Includes */
#include "log.h"
#include "comms.h"

void USART1_IRQHandler(void) {

    if ((USART1->ISR & USART_ISR_RXNE) != 0) {

        char c = USART1->RDR;
        // Skip accidental noise
        if (c == 0xFF) {
            return;
        }

        // log_prints("got char\r\n");
        // Copy bit into buffer. Reading RDR automatically clears flag
        comms_add_to_buffer(USART1, c);
    }

    if ((USART1->ISR & USART_ISR_PE) != 0) {
        log_prints("Parity error UART 1\r\n");
    }

    if ((USART1->ISR & USART_ISR_ORE) != 0) {
        USART1->ICR |= USART_ICR_ORECF; // Clear flag
        log_error("USART1 overrun error\r\n");
    }
}

void USART2_IRQHandler(void) {

    if ((USART2->ISR & USART_ISR_RXNE) != 0) {

        char c = USART2->RDR;

        // Skip accidental noise
        if (c == 0xFF) {
            return;
        }

        // Print the character to the console
        USART2->TDR = c;

        // Copy bit into buffer. Reading RDR automatically clears flag
        comms_add_to_buffer(USART2, c);
    }

    if ((USART2->ISR & USART_ISR_PE) != 0) {
        log_prints("Parity error UART 2\r\n");
    }

    if ((USART2->ISR & USART_ISR_EOBF) != 0) {
        USART2->ICR |= USART_ICR_EOBCF;
        log_prints("USART 2 Full Block received\r\n");
        return;
    }

    // if ((USART2->ISR & USART_ISR_EOBF) != 0) {
    //     USART2->ICR |= USART_ICR_EOBCF;
    //     log_prints("USART 2 Full Block received\r\n");
    //     return;
    // }

    char msg[100];
    sprintf(msg, "2 %lx\r\n", USART1->ISR);
    log_message(msg);
}