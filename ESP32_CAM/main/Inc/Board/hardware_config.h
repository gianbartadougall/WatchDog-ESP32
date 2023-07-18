#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

/* C Library Includes */

/* ESP32 Includes */

/* Personal Includes */

/* Public Marcos for UART */
#define HC_UART_COMMS_TX_PIN     GPIO_NUM_1
#define HC_UART_COMMS_RX_PIN     GPIO_NUM_3
#define HC_UART_COMMS_UART_NUM   UART_NUM_0
#define RX_RING_BUFFER_BYTE_SIZE 1048

/* Public Marcros for Camera */

/* Public Marcros for SD Card */

/* Public Marcos for LEDs */
#define HC_RED_LED 33
#define HC_COB_LED 4

/**
 * @brief Configures all the hardware for the system
 *
 * @return uint8_t TRUE if everything was configured
 * successfully else FALSE
 */
uint8_t hardware_config();

#endif // HARDWARE_CONFIG_H