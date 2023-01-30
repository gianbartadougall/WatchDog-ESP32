
/* Private Includes */
#include "watchdog.h"
#include "ds18b20.h"
#include "hardware_config.h"
#include "log.h"
#include <stdio.h>

#include "comms_stm32.h"
#include "utilities.h"

void print_64_bit1(uint64_t number);

void watchdog_init(void) {
    log_clear();
    ds18b20_init();

    uint32_t flags;
    comms_stm32_init(&flags);
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

    /* TURN ESP32 ON AND OFF USING BJT */
    // Power test
    // Turn the BJT on
    // ESP32_POWER_PORT->ODR |= (0x01 << ESP32_POWER_PIN);
    // LED_GREEN_PORT->ODR |= (0x01 << LED_GREEN_PIN);
    // log_message("Power on\r\n");

    // char msg[50];
    // // Wait for 30 seconds
    // for (int i = 30; i > 0; i--) {
    //     HAL_Delay(1000);
    //     sprintf(msg, "%i seconds left before shutdown\r\n", i);
    //     log_message(msg);
    // }

    // // Turn the BJT off
    // ESP32_POWER_PORT->ODR &= ~(0x01 << ESP32_POWER_PIN);
    // LED_GREEN_PORT->ODR &= ~(0x01 << LED_GREEN_PIN);

    // log_message("Power off\r\n");

    // // Wait for 30 seconds
    // for (int i = 30; i > 0; i--) {
    //     HAL_Delay(1000);
    //     sprintf(msg, "%i seconds left before Restart\r\n", i);
    //     log_message(msg);
    // }
    /* TURN ESP32 ON AND OFF USING BJT */
    char msg[50];
    bpacket_t bpacket;
    while (1) {

        HAL_Delay(1000);
        comms_usart2_print_buffer();
        // comms_process_buffer();

        // if (comms_stm32_get_bpacket(&bpacket) != TRUE) {
        //     continue;
        // }

        // LED_GREEN_PORT->ODR |= (0x01 << LED_GREEN_PIN);

        // sprintf(msg, "Request: %i", bpacket.request);
        // log_message(msg);

        // for (int i = 0; i < bpacket.numBytes; i++) {

        //     sprintf(msg, "%c ", bpacket.bytes[i]);
        //     log_message(msg);
        // }

        // log_message("\r\n");
    }
}