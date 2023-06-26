/**
 * @file stm32l4xx_it.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-06-24
 *
 * @copyright Copyright (c) 2023
 *
 */

/* STM32 Includes */
#include "stm32l4xx_hal.h"
#include "stm32l4xx_it.h"

/* Personal Includes */
#include "log.h"
#include "main.h"

extern PCD_HandleTypeDef hpcd_USB_FS;

/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void) {
    log_error("NMI Error\r\n");
    while (1) {}
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void) {

    log_error("HardFault Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void) {
    log_error("MemManage Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles Prefetch fault, memory access fault.
 */
void BusFault_Handler(void) {
    log_error("Bus Fault Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void) {

    log_error("Usage Fault Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void) {
    log_error("System Service Call\r\n");
}

/**
 * @brief This function handles log monitor.
 */
void logMon_Handler(void) {
    log_error("log monitor called\r\n");
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void) {
    log_error("Pendable request\r\n");
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void) {
    // Incrememnt sys tick timer
    HAL_IncTick();
}

/**
 * @brief This function handles USB event interrupt through EXTI line 17.
 */
void USB_IRQHandler(void) {
    /* USER CODE BEGIN USB_IRQn 0 */

    /* USER CODE END USB_IRQn 0 */
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
    /* USER CODE BEGIN USB_IRQn 1 */

    /* USER CODE END USB_IRQn 1 */
}