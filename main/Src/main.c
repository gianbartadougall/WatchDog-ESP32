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
// #include "led_strip.h"
#include "sdkconfig.h"

static const char* TAG = "example";

#include "camera.h"
#include "sd_card.h"
#include "wd_utils.h"

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 33

#define BLINK_PERIOD 100

static uint8_t s_led_state = 0;

static void blink_led(void) {
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    s_led_state = 1 - s_led_state;
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void) {
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void watchdog_test() {

    char msg[100];

    /* Configure the camera */
    if (ESP_OK != init_camera()) {
        sprintf(msg, "Camera initialisation failed");
        sd_card_log(SYSTEM_LOG_FILE, msg);
        return;
    }

    int count = 0;
    while (1) {

        if (sd_card_open() != WD_SUCCESS) {
            return;
        }

        sprintf(msg, "Taking image %d", count);
        sd_card_log(SYSTEM_LOG_FILE, msg);

        camera_fb_t* pic = esp_camera_fb_get();

        if (pic != NULL) {

            sprintf(msg, "Image %d captured. Size was %zu bytes", count, pic->len);
            sd_card_log(SYSTEM_LOG_FILE, msg);

            char imageName[30];
            sprintf(imageName, "img%d-01_01_2023.jpg", count);

            sprintf(msg, "Name '%s' created for image %d", imageName, count);
            sd_card_log(SYSTEM_LOG_FILE, msg);

            // Save the image
            save_image(pic->buf, pic->len, imageName, count);

            sprintf(msg, "Image %i saved as %s", count, imageName);
            sd_card_log(SYSTEM_LOG_FILE, msg);

            count++;
        } else {
            sprintf(msg, "Camera failed to take image %d", count);
            sd_card_log(SYSTEM_LOG_FILE, msg);
        }

        esp_camera_fb_return(pic);

        sprintf(msg, "Unmounting SD card");
        sd_card_close();

        gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
        vTaskDelay(5000 / portTICK_RATE_MS);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT);
    }
}

void app_main(void) {

    /* Configure the peripheral according to the LED type */
    configure_led();

    watchdog_test();

    while (1) {

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT);
    }

    // while (1) {

    //     int returnCode = sd_card_open();
    //     if (returnCode != WD_SUCCESS) {
    //         if (returnCode == SD_CARD_ERROR_UNREADABLE_CARD) {
    //             ESP_LOGE(TAG, "Please reformat SD card");
    //         }

    //         if (returnCode == SD_CARD_ERROR_CONNECTION_FAILURE) {
    //             ESP_LOGE(TAG, "SD card could not be connected");
    //         }
    //     } else {
    //         ESP_LOGI(TAG, "Mounted SD CARD");
    //         sd_card_close();
    //         ESP_LOGI(TAG, "Unmounted SD CARD");
    //     }

    //     vTaskDelay(3000 / portTICK_PERIOD_MS);
    //     blink_led();
    // }

    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        blink_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
