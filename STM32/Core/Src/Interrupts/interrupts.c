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
#include "log_usb.h"
#include "comms_stm32.h"
#include "watchdog.h"
#include "hardware_config.h"

void USART1_IRQHandler(void) {

    /****** START CODE BLOCK ******/
    // Description: Debuging. Can delete
    log_usb_message("Interrupt 0\r\n");
    /****** END CODE BLOCK ******/

    if ((USART1->ISR & USART_ISR_RXNE) != 0) {

        char c = USART1->RDR;
        // uint8_t c = USART1->RDR;
        // log_send_data(&c, 1);
        // log_usb_message("got char\r\n");
        // Copy bit into buffer. Reading RDR automatically clears flag
        uart_append_to_buffer(BUFFER_1_ID, (uint8_t)c);
    }

    if ((USART1->ISR & USART_ISR_PE) != 0) {
        log_usb_message("Parity error UART 1\r\n");
    }

    if ((USART1->ISR & USART_ISR_ORE) != 0) {
        USART1->ICR |= USART_ICR_ORECF; // Clear flag
        log_usb_error("USART1 overrun error\r\n");
    }
}

void USART2_IRQHandler(void) {
    /****** START CODE BLOCK ******/
    // Description: Debuging. Can delete
    log_usb_message("Interrupt 1\r\n");
    /****** END CODE BLOCK ******/
    if ((USART2->ISR & USART_ISR_RXNE) != 0) {

        char c = USART2->RDR;

        // Copy bit into buffer. Reading RDR automatically clears flag
        // uart_append_to_buffer(USART2, c);
        uart_append_to_buffer(BUFFER_2_ID, (uint8_t)c);
        return;
    }

    if ((USART2->ISR & USART_ISR_PE) != 0) {
        log_usb_message("Parity error UART 2\r\n");
    }

    if ((USART2->ISR & USART_ISR_EOBF) != 0) {
        USART2->ICR |= USART_ICR_EOBCF;
        log_usb_error("USART 2 Full Block received\r\n");
        return;
    }

    // Overrun error occurs when the STM32 is receiving data slower than the
    // UART sending to it is sending
    if ((USART2->ISR & USART_ISR_ORE) != 0) {
        USART2->ICR |= USART_ICR_ORECF;
        log_usb_error("USART 2 Overrun error\r\n");
        return;
    }

    if ((USART2->ISR & USART_ISR_IDLE) != 0) {
        USART2->ICR |= USART_ICR_IDLECF;
        log_usb_error("USART 2 Idle error\r\n");
        return;
    }
}

void RTC_Alarm_IRQHandler(void) {
    /****** START CODE BLOCK ******/
    // Description: Debuging. Can delete
    log_usb_message("Interrupt 2\r\n");
    /****** END CODE BLOCK ******/
    // Check if alarm A was triggered
    if ((STM32_RTC->ISR &= RTC_ISR_ALRAF) != 0) {
        log_usb_message("Triggered!\r\n");

        // Clear the EXTI interrupt flag
        EXTI->PR1 |= (0x01 << 18);

        // Clear the RTC Flag
        STM32_RTC->ISR &= ~(RTC_ISR_ALRAF);

        // Increment the alarm time
        // watchdog_rtc_alarm_triggered();

        return;
    }

    // Check if alarm B was triggered
    if ((STM32_RTC->ISR &= RTC_ISR_ALRBF) != 0) {
        STM32_RTC->ISR &= ~(RTC_ISR_ALRBF);
        log_usb_message("Alarm B triggered\r\n");
        return;
    }
}
