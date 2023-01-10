#include "stm32l4xx_it.h"
#include "log.h"
#include "main.h"

/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void) {
    debug_prints("NMI Error\r\n");
    while (1) {}
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void) {

    debug_prints("HardFault Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void) {
    debug_prints("MemManage Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles Prefetch fault, memory access fault.
 */
void BusFault_Handler(void) {
    debug_prints("Bus Fault Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void) {

    debug_prints("Usage Fault Error\r\n");

    while (1) {}
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void) {
    debug_prints("System Service Call\r\n");
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void) {
    debug_prints("Debug monitor called\r\n");
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void) {
    debug_prints("Pendable request\r\n");
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void) {
    // Incrememnt sys tick timer
    HAL_IncTick();
}