/**
 * @file main.c
 * @author Gian Barta-Dougall
 * @brief Program File
 * @version 0.1
 * @date 2022-04-15
 *
 * @copyright Copyright (c) 2022
 *
 */

/* C Library includes */

/* Personal includes */
#include "main.h"
#include "hardware_config.h"
#include "utils.h"
#include "watchdog.h"
#include "log_usb.h"
#include "led.h"
#include "datetime.h"
#include "stm32_rtc.h"
#include "bpacket.h"
#include "stm32_uart.h"
#include "bpacket_utils.h"
#include "stm32_flash.h"
#include "gpio.h"
#include "stm32_uart.h"
#include "rtc_mcp7940n.h"

/* STM32 Includes */
#include "stm32l4xx_hal.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

/* Variable Declarations */

/* Private #defines */

/* Variable Declarations */

/* Function prototypes */
void error_handler(void);
void SystemClock_Config(void);

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

    // Reset all peripherals, initialise the flash interface and the systick
    HAL_Init();

    wd_start();

    while (1) {}

    return 0;
}

/**
 * @brief Handles initialisation errors
 *
 */
void error_handler(void) {

    // Initialisation error shown by blinking LED (LD3) in pattern
    while (1) {

        for (int i = 0; i < 5; i++) {
            HAL_GPIO_TogglePin(LED_GREEN_PORT, LED_GREEN_PIN);
            HAL_Delay(100);
        }

        HAL_Delay(500);
    }
}