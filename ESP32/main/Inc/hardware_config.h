#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

/* Public Includes */
#include <stdint.h>

/* ESP32 Includes */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gptimer.h>

/* Public Macros for System */
#define SYSTEM_CLK_FREQ 160000000 // CPU operates at 160Mhz

/* Public Marcos for UART */
static const int RX_BUF_SIZE = 1024;
#define HC_UART_COMMS_TX_PIN   GPIO_NUM_1
#define HC_UART_COMMS_RX_PIN   GPIO_NUM_3
#define HC_UART_COMMS_UART_NUM UART_NUM_0

/* Public Marcros for Camera */

/* Public Marcros for SD Card */

/* Public Marcos for LEDs */
#define HC_RED_LED 2
#define HC_LED_2   23

/* Public macros for temperature sensor */
// timer_config_t ds18b20TimerConfig;

extern gptimer_handle_t gptimer;
extern gptimer_config_t timer_config;

#define HC_DS18B20_PIN       27
#define DS18B20_TIMER_GROUP  0 // Timer Group 0
#define DS18B20_TIMER        0 // Timer 0
#define DS18B20_TIMER_PERIOD (SYSTEM_CLK_FREQ / 2 / 1000000) // Get a 1us timer

/**
 * @brief Configures all the hardware for the system
 *
 * @return uint8_t WD_SUCCESS if everything was configured
 * successfully else WD_ERROR
 */
uint8_t hardware_config(void);

#endif // HARDWARE_CONFIG_H