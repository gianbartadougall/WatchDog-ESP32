/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "hardware_config.h"
#include "wd_utils.h"
#include "uart_comms.h"
#include "led.h"
#include "ds18b20.h"

#define COB_LED HC_COB_LED
#define RED_LED HC_RED_LED

#define BLINK_PERIOD 100

void watchdog_system_start() {

    while (1) {

        uint64_t rom = ds18b20_read_rom();
        ESP_LOGE("ROM", "%lld", rom);

        vTaskDelay(100);
    }

    int action = UC_SAVE_DATA;
    char message[100];
    char instruction[100];
    int count = 0;
    while (1) {

        sprintf(instruction, "WATCHDOG/DATA%cdata.txt", UC_DATA_DELIMETER);
        sprintf(message, "%d.125%c17.334%c1/2/2023", count, UC_DATA_DELIMETER, UC_DATA_DELIMETER);
        count++;
        // Delay for second
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        led_toggle(HC_LED_2);
        uart_comms_transmit_message(action, instruction, message);
    }
}

uint8_t software_config(void) {

    // TODO: Configure RTC

    // TODO: Configure Temperature Sensor
    ds18b20_init();

    return WD_SUCCESS;
}

void app_main(void) {

    /* Initialise all the hardware used */
    if ((hardware_config() == WD_SUCCESS) && (software_config() == WD_SUCCESS)) {
        watchdog_system_start();
        // xTaskCreate(watchdog_system_start, "Watchdog", 4096, (void*)1, tskIDLE_PRIORITY, NULL);
    } else {
        while (1) {
            ESP_LOGI("Main.c Error", "Blink");
            led_toggle(RED_LED);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
