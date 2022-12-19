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

static const char* TAG = "example";
#include "wd_utils.h"
#include "rtc.h"
#include "hardware_config.h"
#include "uart_comms.h"
#include "led.h"

#define COB_LED HC_COB_LED
#define RED_LED HC_RED_LED

#define BLINK_PERIOD 100

// static const int RX_BUF_SIZE = 1024;
// #define TXD_PIN  (GPIO_NUM_1)
// #define RXD_PIN  (GPIO_NUM_3)
// #define UART_NUM UART_NUM_0

void watchdog_esp_cam(void* arg) {

    int action = UC_COMMAND_NONE;
    char message[100];

    while (1) {
        // Delay for second
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Transmit message

        // Read UART and wait for command.
        led_toggle(HC_LED_2);
        uart_comms_receive_command(&action, message);

        ESP_LOGI(TAG, "Command recieved: %d. Message: '%s'", action, message);

        if (action == UC_COMMAND_BLINK_LED) {
            led_toggle(RED_LED);
        }

        // if (action == UC_ACTION_CAPTURE_IMAGE) {
        //     if (camera_take_image() != WD_SUCCESS) {
        //         break;
        //     }
        // }

        // Reset the action back to NONE
        action = UC_COMMAND_NONE;
    }
}

void watchdog_esp(void* arg) {

    int action = UC_COMMAND_BLINK_LED;
    char message[100];
    sprintf(message, "Blink the LED!");

    while (1) {

        // Delay for second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        led_toggle(HC_LED_2);
        uart_comms_transmit_message(action, message, "");
    }
}

uint8_t software_config(void) {

    // TODO: Configure RTC

    // TODO: Configure Temperature Sensor

    return WD_SUCCESS;
}

void app_main(void) {

    /* Initialise all the hardware used */
    if ((hardware_config() == WD_SUCCESS) && (software_config() == WD_SUCCESS)) {
        // xTaskCreate(watchdog_esp, "Watchdog", 4096, (void*)1, tskIDLE_PRIORITY, NULL);
        xTaskCreate(watchdog_esp_cam, "Watchdog", 4096, (void*)1, tskIDLE_PRIORITY, NULL);
    }

    // while (1) {
    //     ESP_LOGI(TAG, "Blink");
    //     led_toggle(RED_LED);
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}
