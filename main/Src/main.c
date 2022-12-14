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

    vTaskDelay(1000 / portTICK_RATE_MS);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT);
}

void watchdog_test() {

    char msg[100];
    if (sd_card_open() != WD_SUCCESS) {
        return;
    }

    /* Configure the camera */
    if (ESP_OK != init_camera()) {
        sprintf(msg, "Camera failed to initialise");
        sd_card_log(SYSTEM_LOG_FILE, msg);
        sd_card_close();

        gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);
        return;
    } else {
        sprintf(msg, "Camera initialised");
        sd_card_log(SYSTEM_LOG_FILE, msg);
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
            sprintf(imageName, "img%d.jpg", count);

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
        sd_card_log(SYSTEM_LOG_FILE, msg);
        sd_card_close();

        gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
        vTaskDelay(5000 / portTICK_RATE_MS);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT);
    }
}

uint8_t hardware_config(void) {

    /* Configure all the required hardware */

    // Configure the SD card and ensure it can be used. This is done before
    // any other hardware so that everything else can be logged

    char msg[100];
    if (sd_card_open() != WD_SUCCESS) {
        return WD_ERROR;
    }

    sprintf(msg, "Configuring RTC");
    sd_card_log(SYSTEM_LOG_FILE, msg);
    // TODO: Configure RTC

    sprintf(msg, "Configuring Temperature Sensor");
    sd_card_log(SYSTEM_LOG_FILE, msg);
    // TODO: Configure Temperature sensor

    // Unmount the SD card
    sd_card_close();

    return WD_SUCCESS;
}

void app_main(void) {

    /* Configure the peripheral according to the LED type */
    configure_led();

    /* Initialise all the hardware used */
    if (hardware_config() == WD_SUCCESS) {
        watchdog_test();
    }

    char msg[100];
    if (sd_card_open() == WD_SUCCESS) {
        sprintf(msg, "Exited Watchdog System. Waiting for shutdown");
        sd_card_log(SYSTEM_LOG_FILE, msg);
        sd_card_close();
    }

    while (1) {

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT);
    }
}
