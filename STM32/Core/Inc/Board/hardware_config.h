/**
 * @file hardware_configuration.h
 * @author Gian Barta-Dougall
 * @brief This file contains all the pin and GPIO assignments for all the
 * harware peripherals on the board
 * @version 0.1
 * @date --
 *
 * @copyright Copyright (c)
 *
 */
#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

/* Public Includes */

/* Public STM Includes */
#include "stm32l4xx_hal.h"

/* Public #defines */

/* Timer Macros */
#define TIMER_FREQUENCY_1KHz 1000
#define TIMER_FREQUENCY_1MHz 1000000

// TEMPORARY MARCO: TODO IS MAKE A WHOE CLOCK CONFIG FILE. This currently
// just copies value from STM32 file
#define SYSTEM_CLOCK_CORE 4000000

// Define ports and pins for peripherals that have been enabled in
// configuration.h file

/********** Marcos for hardware related to the LEDs **********/

#define LED_GREEN_PORT GPIOB
#define LED_GREEN_PIN  3

/*************************************************************/

/******** Macros for DS18B20 **********/

#define DS18B20_PORT GPIOB
#define DS18B20_PIN  0

/**************************************/

/********** Marcos for hardware related to the DS18B20 Temperature Sensor ***********/

#define DS18B20_TIMER              TIM15
#define DS18B20_TIMER_CLK_ENABLE() __HAL_RCC_TIM15_CLK_ENABLE()
#define DS18B20_TIMER_FREQUENCY    TIMER_FREQUENCY_1MHz
#define DS18B20_TIMER_MAX_COUNT    65535
#define DS18B20_TIMER_IRQn         TIM1_BRK_TIM15_IRQn
#define DS18B20_TIMER_ISR_PRIORITY TIM15_ISR_PRIORITY

/************************************************************************************/

/********** Marcos for UART Communication with ESP32 Cam ***********/

#define UART_ESP32_GPIO_RX_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define UART_ESP32_GPIO_TX_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define UART_ESP32_CLK_ENABLE()         __HAL_RCC_USART1_CLK_ENABLE()

#define UART_ESP32           USART1
#define UART_ESP32_IRQn      USART1_IRQn
#define UART_ESP32_RX_PORT   GPIOB
#define UART_ESP32_TX_PORT   GPIOA
#define UART_ESP32_RX_PIN    7
#define UART_ESP32_TX_PIN    9
#define UART_ESP32_BUAD_RATE 115200
/*******************************************************************/

/********** Marcos for hardware related to the debug log **********/
/**
 * Configuration for UART which allows debuggiong and general information
 * transfer
 */
#define UART_LOG_GPIO_RX_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define UART_LOG_GPIO_TX_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define UART_LOG_CLK_ENABLE()         __HAL_RCC_USART2_CLK_ENABLE()

#define UART_LOG           USART2
#define UART_LOG_IRQn      USART2_IRQn
#define UART_LOG_RX_PORT   GPIOA
#define UART_LOG_TX_PORT   GPIOA
#define UART_LOG_RX_PIN    15
#define UART_LOG_TX_PIN    2
#define UART_LOG_BUAD_RATE 115200
/**************************************************************/

/* Public Structures and Enumerations */

/* Public Variable Declarations */

/* Public Function Prototypes */

/**
 * @brief Initialise the system library.
 */
void hardware_config_init(void);

#endif // HARDWARE_CONFIG_H