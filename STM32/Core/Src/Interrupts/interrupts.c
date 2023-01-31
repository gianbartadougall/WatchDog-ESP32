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
#include "comms_stm32.h"
#include "hardware_config.h"

void USART1_IRQHandler(void) {

    if ((USART1->ISR & USART_ISR_RXNE) != 0) {

        char c = USART1->RDR;
        // Skip accidental noise
        if (c == 0xFF) {
            return;
        }

        // log_prints("got char\r\n");
        // Copy bit into buffer. Reading RDR automatically clears flag
        comms_add_to_buffer(BUFFER_1_ID, (uint8_t)c);
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

        // Copy bit into buffer. Reading RDR automatically clears flag
        // comms_add_to_buffer(USART2, c);
        comms_add_to_buffer(BUFFER_2_ID, (uint8_t)c);
        return;
    }

    if ((USART2->ISR & USART_ISR_PE) != 0) {
        log_prints("Parity error UART 2\r\n");
    }

    if ((USART2->ISR & USART_ISR_EOBF) != 0) {
        USART2->ICR |= USART_ICR_EOBCF;
        log_error("USART 2 Full Block received\r\n");
        return;
    }

    // Overrun error occurs when the STM32 is receiving data slower than the
    // UART sending to it is sending
    if ((USART2->ISR & USART_ISR_ORE) != 0) {
        USART2->ICR |= USART_ICR_ORECF;
        log_error("USART 2 Overrun error\r\n");
        return;
    }

    if ((USART2->ISR & USART_ISR_IDLE) != 0) {
        USART2->ICR |= USART_ICR_IDLECF;
        log_error("USART 2 Idle error\r\n");
        return;
    }

    char msg[50];
    sprintf(msg, "%lx\r\n", USART2->ISR);
    log_message(msg);
}