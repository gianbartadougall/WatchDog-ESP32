/**
 * @file exti_interrupts.c
 * @author Gian Barta-Dougall
 * @brief File to store EXTI interrupts for GPIO pins for STM32L432KC mcu
 * @version 0.1
 * @date 2022-06-28
 *
 * @copyright Copyright (c) 2022
 *
 */
/* C Library Includes */

/* STM32 Includes */
#include "stm32l432xx.h"
#include "stm32l4xx_hal.h"

/* Personal Includes */
#include "utils.h"
#include "log.h"

/**
 * @brief Interrupt routine for EXTI1
 *
 */
void EXTI0_IRQHandler(void) {

    // Clear the pending interrupt call
    NVIC_ClearPendingIRQ(EXTI1_IRQn);

    if ((EXTI->PR1 & EXTI_PR1_PIF0) == EXTI_PR1_PIF0) {

        // Clear the interrupt flag
        EXTI->PR1 = EXTI_PR1_PIF0;

        /* Call required functions */
    }
}

/**
 * @brief Interrupt routine for EXTI1
 *
 */
void EXTI1_IRQHandler(void) {

    // Clear the pending interrupt call
    NVIC_ClearPendingIRQ(EXTI1_IRQn);

    if ((EXTI->PR1 & EXTI_PR1_PIF1) == EXTI_PR1_PIF1) {

        // Clear the interrupt flag
        EXTI->PR1 = EXTI_PR1_PIF1;

        /* Call required functions */
    }
}

/**
 * @brief Interrupt handler for EXTI2
 *
 */
void EXTI2_IRQHandler(void) {

    // Clear the pending interrupt call
    NVIC_ClearPendingIRQ(EXTI2_IRQn);

    if ((EXTI->PR1 & EXTI_PR1_PIF2) == EXTI_PR1_PIF2) {

        // Clear the interrupt flag
        EXTI->PR1 = EXTI_PR1_PIF2;

        /* Call required functions */
    }
}

/**
 * @brief Interrupt handler for EXTI3
 *
 */
void EXTI3_IRQHandler(void) {

    // // Clear the pending interrupt call
    NVIC_ClearPendingIRQ(EXTI3_IRQn);

    // Confirm pending interrupt exists on EXTI line 3
    if ((EXTI->PR1 & EXTI_PR1_PIF3) == EXTI_PR1_PIF3) {

        // Clear the interrupt flag
        EXTI->PR1 = EXTI_PR1_PIF3;

        /* Call required functions */
    }
}

/**
 * @brief Interrupt routine for EXTI4
 *
 */
void EXTI4_IRQHandler(void) {

    // Clear the pending interrupt call
    NVIC_ClearPendingIRQ(EXTI4_IRQn);

    if ((EXTI->PR1 & EXTI_PR1_PIF4) == EXTI_PR1_PIF4) {

        // Clear the interrupt flag
        EXTI->PR1 = EXTI_PR1_PIF4;

        /* Call required functions */
    }
}

/**
 * @brief Interrupt handler for EXTI5 - EXTI9
 *
 */
void EXTI9_5_IRQHandler(void) {

    // Clear the pending interrupt call
    NVIC_ClearPendingIRQ(EXTI9_5_IRQn);

    // // Confirm pending interrupt exists on EXTI line 6
    // if ((EXTI->PR1 & EXTI_PR1_PIF5) == (EXTI_PR1_PIF5)) {

    //     // Clear the pending interrupt
    //     EXTI->PR1 = EXTI_PR1_PIF5;

    //     /* Call required functions */
    // }

    // // Confirm pending interrupt exists on EXTI line 6
    // if ((EXTI->PR1 & EXTI_PR1_PIF6) == EXTI_PR1_PIF6) {

    //     // Clear the interrupt flag
    //     EXTI->PR1 |= EXTI_PR1_PIF6;

    //     /* Call required functions */

    // }

    // // Confirm pending interrupt exists on EXTI line 7
    // if ((EXTI->PR1 & EXTI_PR1_PIF7) == EXTI_PR1_PIF7) {

    //     // Clear the interrupt flag
    //     EXTI->PR1 |= EXTI_PR1_PIF7;

    //     /* Call required functions */

    // }

    // Confirm pending interrupt exists on EXTI line 8
    if ((EXTI->PR1 & EXTI_PR1_PIF8) == EXTI_PR1_PIF8) {

        // Clear the interrupt flag
        EXTI->PR1 |= EXTI_PR1_PIF8;

        /* Call required functions */
        log_message("Interrupt PA8\r\n");
    }

    // // Confirm pending interrupt exists on EXTI line 9
    // if ((EXTI->PR1 & EXTI_PR1_PIF9) == EXTI_PR1_PIF9) {

    //     // Clear the interrupt flag
    //     EXTI->PR1 |= EXTI_PR1_PIF9;

    //     /* Call required functions */

    // }
}

/**
 * @brief Interrupt handler for EXTI10 - EXTI15
 *
 */
void EXTI15_10_IRQHandler(void) {

    // Clear the pending interrupt call
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);

    // // Confirm pending interrupt exists on EXTI line 10
    // if ((EXTI->PR1 & EXTI_PR1_PIF10) == EXTI_PR1_PIF10) {

    //     // Clear the interrupt flag
    //     EXTI->PR1 |= EXTI_PR1_PIF10;

    //     /* Call required functions */

    // }

    // // Confirm pending interrupt exists on EXTI line 11
    // if ((EXTI->PR1 & EXTI_PR1_PIF11) == EXTI_PR1_PIF11) {

    //     // Clear the interrupt flag
    //     EXTI->PR1 |= EXTI_PR1_PIF11;

    //     /* Call required functions */

    // }

    // // Confirm pending interrupt exists on EXTI line 12
    // if ((EXTI->PR1 & EXTI_PR1_PIF12) == EXTI_PR1_PIF12) {

    //     // Clear the interrupt flag
    //     EXTI->PR1 |= EXTI_PR1_PIF12;

    //     /* Call required functions */

    // }

    // // Confirm pending interrupt exists on EXTI line 13
    // if ((EXTI->PR1 & EXTI_PR1_PIF13) == EXTI_PR1_PIF13) {

    //     // Clear the interrupt flag
    //     EXTI->PR1 |= EXTI_PR1_PIF13;

    //     /* Call required functions */

    // }

    // // Confirm pending interrupt exists on EXTI line 14
    // if ((EXTI->PR1 & EXTI_PR1_PIF14) == EXTI_PR1_PIF14) {

    //     // Clear the interrupt flag
    //     EXTI->PR1 |= EXTI_PR1_PIF14;

    //     /* Call required functions */

    // }

    // // Confirm pending interrupt exists on EXTI line 15
    // if ((EXTI->PR1 & EXTI_PR1_PIF15) == EXTI_PR1_PIF15) {

    //     // Clear the interrupt flag
    //     EXTI->PR1 |= EXTI_PR1_PIF15;

    //     /* Call required functions */

    // }
}

void COMP_IRQHandler(void) {

    // Clear the pending interrupt call
    NVIC_ClearPendingIRQ(COMP_IRQn);

    // Check if comparator 1 triggered an interrupt
    if ((EXTI->PR1 & EXTI_PR1_PIF21) == EXTI_PR1_PIF21) {

        // Clear the interrupt flag
        EXTI->PR1 = EXTI_PR1_PIF21;

        /* Call required functions */
    }

    // Check if comparator 2 triggered an interrupt
    if ((EXTI->PR1 & EXTI_PR1_PIF22) == EXTI_PR1_PIF22) {

        // Clear the interrupt flag
        EXTI->PR1 = EXTI_PR1_PIF22;

        /* Call required functions */
    }
}