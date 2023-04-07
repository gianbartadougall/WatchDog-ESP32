/* Personal Includes */
#include "stm32_uart.h"

int uart_transmit_string(const char* msg, ...) {

    // Transmit over uart if using a micrcontroller
    uint16_t i = 0;

    // Transmit until end of message reached
    while (msg[i] != '\0') {
        while ((USART2->ISR & USART_ISR_TXE) == 0) {};

        USART2->TDR = msg[i];
        i++;
    }

    return 1;
}

char uart_getc(void) {

    while (!(USART2->ISR & USART_ISR_RXNE)) {};
    return USART2->RDR;
}

void uart_transmit_data(uint8_t* data, uint8_t length) {

    // Transmit until end of message reached
    for (int i = 0; i < length; i++) {
        while ((USART2->ISR & USART_ISR_TXE) == 0) {};
        USART2->TDR = data[i];
    }
}