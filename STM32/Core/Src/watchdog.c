
/* Private Includes */
#include "watchdog.h"
#include "comms.h"
#include "ds18b20.h"
#include "hardware_config.h"
#include "log.h"
#include "wd_utils.h"
#include <stdio.h>

void print_64_bit1(uint64_t number);

void watchdog_init(void) {
    log_clear();
    ds18b20_init();
}

void watchdog_update(void) {

    /* TEMPERATURE CONVERSION CODE */
    // // Make the temp sensor record the temperature
    // if (ds18b20_read_temperature(DS18B20_SENSOR_ID_2) != TRUE) {
    //     log_error("Failed to record temperature\r\n");
    //     return;
    // }

    // ds18b20_temp_t temp;

    // if (ds18b20_copy_temperature(DS18B20_SENSOR_ID_2, &temp) != TRUE) {
    //     log_error("Failed to read temperature\r\n");
    //     return;
    // }

    // char msg[40];
    // sprintf(msg, "Temperature: %s%i.%i\r\n", (temp.sign == 0) ? "" : "-", temp.decimal, temp.fraction);
    // log_message(msg);
    /* TEMPERATURE CONVERSION CODE */

    // Power test
    // Turn the BJT on
    ESP32_POWER_PORT->ODR |= (0x01 << ESP32_POWER_PIN);
    LED_GREEN_PORT->ODR |= (0x01 << LED_GREEN_PIN);
    log_message("Power on\r\n");

    char msg[50];
    // Wait for 30 seconds
    for (int i = 30; i > 0; i--) {
        HAL_Delay(1000);
        sprintf(msg, "%i seconds left before shutdown\r\n", i);
        log_message(msg);
    }

    // Turn the BJT off
    ESP32_POWER_PORT->ODR &= ~(0x01 << ESP32_POWER_PIN);
    LED_GREEN_PORT->ODR &= ~(0x01 << LED_GREEN_PIN);

    log_message("Power off\r\n");

    // Wait for 30 seconds
    for (int i = 30; i > 0; i--) {
        HAL_Delay(1000);
        sprintf(msg, "%i seconds left before Restart\r\n", i);
        log_message(msg);
    }

    // comms_usart1_print_buffer();
}