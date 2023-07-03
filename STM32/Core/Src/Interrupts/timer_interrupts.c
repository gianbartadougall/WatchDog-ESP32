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

void TIM1_BRK_TIM15_IRQHandler(void) {

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
        ts_isr();
    }
}