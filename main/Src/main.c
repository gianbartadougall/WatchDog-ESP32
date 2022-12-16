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

/* TEST CODE FOR UART */
#include "esp_system.h"
#include "driver/uart.h"
/* TEST CODE FOR UART */

static const char* TAG = "example";
#include "camera.h"
#include "sd_card.h"
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

void watchdog_v1_test(void) {

    int action;
    char message[100];

    while (1) {

        // Read UART and wait for command.
        uart_comms_receive_command(&action, message);

        if (action == UC_ACTION_NONE) {
            continue;
        }

        if (action == UC_ACTION_BLINK_LED) {
            led_board_toggle();
        }

        // if (action == UC_ACTION_CAPTURE_IMAGE) {
        //     if (camera_take_image() != WD_SUCCESS) {
        //         break;
        //     }
        // }

        // Reset the action back to NONE
        action = UC_ACTION_NONE;
    }
}

void watchdog_test(void) {

    char msg[100];
    if (sd_card_open() != WD_SUCCESS) {
        return;
    }

    while (1) {

        // if (sd_card_open() != WD_SUCCESS) {
        //     return;
        // }

        // sprintf(msg, "Taking image");
        // sd_card_log(SYSTEM_LOG_FILE, msg);

        // camera_fb_t* pic = esp_camera_fb_get();

        // if (pic != NULL) {

        //     sprintf(msg, "Image captured. Size was %zu bytes", pic->len);
        //     sd_card_log(SYSTEM_LOG_FILE, msg);

        //     // Save the image
        //     sd_card_save_image(pic->buf, pic->len);
        // } else {
        //     sprintf(msg, "Camera failed to take image");
        //     sd_card_log(SYSTEM_LOG_FILE, msg);
        // }

        // esp_camera_fb_return(pic);

        // sd_card_close();
        camera_capture_and_save_photo();

        gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
        vTaskDelay(5000 / portTICK_RATE_MS);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT);
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
        watchdog_v1_test();
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
