/**
 * @file timer_interrupts.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-03
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */

/* STM32 Library Includes */
#include "stm32l432xx.h"

/* Personal Includes */
#include "task_scheduler.h"
#include "log_usb.h"

void TIM1_BRK_TIM15_IRQHandler(void) {
    /****** START CODE BLOCK ******/
    // Description: Debuging. Can delete
    log_usb_message("Interrupt Timer 15\r\n");
    /****** END CODE BLOCK ******/
    // Check and clear overflow flag for TIM15
    if ((TIM15->SR & TIM_SR_UIF) == TIM_SR_UIF) {

        // Clear the UIF flag
        TIM15->SR = ~TIM_SR_UIF;

        /* Call required functions */
    }

    // Check and clear CCR1 flag for TIM15
    if ((TIM15->SR & TIM_SR_CC1IF) == TIM_SR_CC1IF) {

        // Clear the UIF flag
        TIM15->SR = ~TIM_SR_CC1IF;

        /* Call required functions */
        task_scheduler_isr();
    }
}

void TIM1_UP_TIM16_IRQHandler(void) {
    /****** START CODE BLOCK ******/
    // Description: Debuging. Can delete
    log_usb_message("Interrupt Timer 16\r\n");
    /****** END CODE BLOCK ******/
}

void TIM1_TRG_COM_IRQHandler(void) {
    /****** START CODE BLOCK ******/
    // Description: Debuging. Can delete
    log_usb_message("Interrupt Timer 1 trg\r\n");
    /****** END CODE BLOCK ******/
}

void TIM1_CC_IRQHandler(void) {
    /****** START CODE BLOCK ******/
    // Description: Debuging. Can delete
    log_usb_message("Interrupt Timer 1 cc\r\n");
    /****** END CODE BLOCK ******/
}

void TIM2_IRQHandler(void) {
    /****** START CODE BLOCK ******/
    // Description: Debuging. Can delete
    log_usb_message("Interrupt Timer 2\r\n");
    /****** END CODE BLOCK ******/
}

void TIM6_DAC_IRQHandler(void) { /****** START CODE BLOCK ******/
    // Description: Debuging. Can delete
    log_usb_message("Interrupt Timer 6\r\n");
    /****** END CODE BLOCK ******/}

    void TIM7_IRQHandler(void) { /****** START CODE BLOCK ******/
        // Description: Debuging. Can delete
        log_usb_message("Interrupt Timer 7\r\n");
    /****** END CODE BLOCK ******/}