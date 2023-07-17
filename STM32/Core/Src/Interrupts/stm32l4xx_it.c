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
#include "log_usb.h"
#include "main.h"

extern PCD_HandleTypeDef hpcd_USB_FS;

/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void) {
    log_usb_error("NMI Error\r\n");
    while (1) {}
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void) {

    log_usb_error("HardFault Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void) {
    log_usb_error("MemManage Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles Prefetch fault, memory access fault.
 */
void BusFault_Handler(void) {
    log_usb_error("Bus Fault Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void) {

    log_usb_error("Usage Fault Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void) {
    log_usb_error("System Service Call\r\n");
}

/**
 * @brief This function handles log monitor.
 */
void logMon_Handler(void) {
    log_usb_error("log monitor called\r\n");
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void) {
    log_usb_error("Pendable request\r\n");
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

void WWDG_IRQHandler(void) {
    log_usb_error("WWDG called\r\n");
}

void PVD_PVM_IRQHandler(void) {
    log_usb_error("PVD_PVM_IRQHandler called\r\n");
}

void TAMP_STAMP_IRQHandler(void) {
    log_usb_error("TAMP_STAMP_IRQHandler called\r\n");
}

void RTC_WKUP_IRQHandler(void) {
    log_usb_error("RTC_WKUP_IRQHandler called\r\n");
}

void FLASH_IRQHandler(void) {
    log_usb_error("FLASH_IRQHandler called\r\n");
}

void RCC_IRQHandler(void) {
    log_usb_error("RCC_IRQHandler called\r\n");
}

void DMA1_Channel1_IRQHandler(void) {
    log_usb_error("DMA1_Channel1_IRQHandler called\r\n");
}

void DMA1_Channel2_IRQHandler(void) {
    log_usb_error("DMA1_Channel2_IRQHandler called\r\n");
}

void DMA1_Channel3_IRQHandler(void) {
    log_usb_error("DMA1_Channel3_IRQHandler called\r\n");
}

void DMA1_Channel4_IRQHandler(void) {
    log_usb_error("DMA1_Channel4_IRQHandler called\r\n");
}

void DMA1_Channel5_IRQHandler(void) {
    log_usb_error("DMA1_Channel5_IRQHandler called\r\n");
}

void DMA1_Channel6_IRQHandler(void) {
    log_usb_error("DMA1_Channel6_IRQHandler called\r\n");
}

void DMA1_Channel7_IRQHandler(void) {
    log_usb_error("DMA1_Channel7_IRQHandler called\r\n");
}

void CAN1_TX_IRQHandler(void) {
    log_usb_error("CAN1_TX_IRQHandler called\r\n");
}

void CAN1_RX1_IRQHandler(void) {
    log_usb_error("CAN1_RX1_IRQHandler called\r\n");
}

void CAN1_SCE_IRQHandler(void) {
    log_usb_error("CAN1_SCE_IRQHandler called\r\n");
}

void I2C1_EV_IRQHandler(void) {
    log_usb_error("I2C1_EV_IRQHandler called\r\n");
}

void I2C1_ER_IRQHandler(void) {
    log_usb_error("I2C1_ER_IRQHandler called\r\n");
}

void SPI1_IRQHandler(void) {
    log_usb_error("SPI1_IRQHandler called\r\n");
}

void SPI3_IRQHandler(void) {
    log_usb_error("SPI3_IRQHandler called\r\n");
}

void TSC_IRQHandler(void) {
    log_usb_error("TSC_IRQHandler called\r\n");
}

void SWPMI1_IRQHandler(void) {
    log_usb_error("SWPMI1_IRQHandler called\r\n");
}

void RNG_IRQHandler(void) {
    log_usb_error("RNG_IRQHandler called\r\n");
}

void FPU_IRQHandler(void) {
    log_usb_error("FPU_IRQHandler called\r\n");
}

void CRS_IRQHandler(void) {
    log_usb_error("CRS_IRQHandler called\r\n");
}

void SAI1_IRQHandler(void) {
    log_usb_error("SAI1_IRQHandler called\r\n");
}
