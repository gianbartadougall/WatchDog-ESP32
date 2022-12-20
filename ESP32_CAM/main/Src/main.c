/**
 * @file main.c
 * @author Gian Barta-Dougall, Yash Pandey
 * @brief
 * @version 0.1
 * @date 2022-12-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char* TAG = "example";
#include "camera.h"
#include "sd_card.h"
#include "wd_utils.h"
#include "rtc.h"
#include "hardware_config.h"
#include "uart_comms.h"
#include "led.h"

#include "hardware_config.h"

#define COB_LED HC_COB_LED
#define RED_LED HC_RED_LED

#define BLINK_PERIOD 100

// static const int RX_BUF_SIZE = 1024;
// #define TXD_PIN  (GPIO_NUM_1)
// #define RXD_PIN  (GPIO_NUM_3)
// #define UART_NUM UART_NUM_0

void watchdog_system_start(void) {

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
            led_toggle(RED_LED_TEST);
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


uint8_t software_config(void) {

    if (sd_card_init() != WD_SUCCESS) {
        return WD_ERROR;
    }

    return WD_SUCCESS;
}

void app_main(void) {

    /* Initialise all the hardware used */
    if ((hardware_config() == WD_SUCCESS) && (software_config() == WD_SUCCESS)) {
        watchdog_system_start();
    }

    char msg[100];
    if (sd_card_open() == WD_SUCCESS) {
        sprintf(msg, "Exited Watchdog System. Waiting for shutdown");
        sd_card_log(SYSTEM_LOG_FILE, msg);
        sd_card_close();
    }

    while (1) {
        ESP_LOGE(TAG, "Blink");
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(RED_LED, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(RED_LED, 0);
    }
}
